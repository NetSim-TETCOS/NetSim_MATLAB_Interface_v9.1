/************************************************************************************
 * Copyright (C) 2016                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                     *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#include "main.h"
#include "IEEE802_11_Phy.h"

static double** dCummulativeReceivedPower=NULL;
double get_cummlativeReceivedPower(NETSIM_ID dev1,NETSIM_ID dev2)
{
	return dCummulativeReceivedPower[dev1-1][dev2-1];
}

void set_cummulativeReceivedPower(NETSIM_ID dev1,NETSIM_ID dev2,double power)
{
	dCummulativeReceivedPower[dev1-1][dev2-1]=power;
}

void propagation_model_init()
{
	if(!dCummulativeReceivedPower)
	{
		NETSIM_ID i;
		dCummulativeReceivedPower=(double**)calloc(NETWORK->nDeviceCount,sizeof* dCummulativeReceivedPower);
		for(i=0;i<NETWORK->nDeviceCount;i++)
		{
			dCummulativeReceivedPower[i]=(double*)calloc(NETWORK->nDeviceCount,sizeof* dCummulativeReceivedPower[i]);
			set_cummulativeReceivedPower(i+1,i+1,NEGATIVE_INFINITY);
		}
	}
}

void propagation_model_finish()
{
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		free(dCummulativeReceivedPower[i]);
	}
	free(dCummulativeReceivedPower);
}

/**
This function invoke the received power calculation and 
selects the path loss model based on the channel characteristics value. 
*/
static void calculate_pathloss(double nTxPower,double dDistance,double dFrequency,double dPathLossExponent,double *dReceivedPower)
{
	double fpi=3.1417f;		
	double nGainTxr=0;		
	double nGainRver=0;	
	int nDefaultDistance=1; 
	double fA1,fWaveLength=0.0; 
	double fA1dB, fA2dB;
	double fTXPowerdBm;

	// get the gain of the Transmitter
	nGainTxr=0;
	// get the gain of the receiver
	nGainRver=0;

	fTXPowerdBm = 10 * log10(nTxPower);

	if(dDistance==0.0)
	{
		*dReceivedPower=fTXPowerdBm;
		return;
	}
	
	// Calculate Lambda
	fWaveLength=(double)(300.0/(dFrequency * 1.0));
	// Calculate  (4*3.14*do)
	fA1=fWaveLength/(4*(double) fpi * nDefaultDistance );
	//Calculate  20log10[ Lambda/(4*3.14*do) ]
	fA1dB =  20 * log10(fA1);
	//Calculate  10 * n *log10 (do/d)
	fA2dB =  10 * dPathLossExponent * log10(nDefaultDistance /dDistance);
	//Calculate  [Pt]  +  [Gt]  +  [Gr]  +  20log10[ Lemda/(4*3.14*do) ] + ( 10 * n *log10 (do/d) )
	*dReceivedPower = fTXPowerdBm + nGainTxr + nGainRver + fA1dB + fA2dB;
}

/**
This Function is used to calculate the Fading Loss(Rayleigh Fading)
*/
int fnCalculateFadingLoss(unsigned long *ulSeed1, unsigned long *ulSeed2,double *dFadingPower,int fm1)
{
	double n_sign;

	if(fm1==1)// if rayleigh fading is set
	{
		n_sign = fn_NetSim_Utilities_GenerateRandomNo(ulSeed1,ulSeed2)/NETSIM_RAND_MAX;
		*dFadingPower = n_sign;
		if(*dFadingPower<0 ||*dFadingPower>1) // Sanity check -- this condition should never occur
		{
			*dFadingPower=0;
			fnNetSimError("Random Number generated while calculating fading loss is not in the range [0,1]");
			return 0;
		}
		else
		{
			//*dFadingPower = log(*dFadingPower);
			
			//*dFadingPower = 10*log10(-*dFadingPower); //Rayleigh Fading is an exponential distribution with mean 1. So, we take an inverse exponential funtion to calculate Fading loss.
			*dFadingPower = fn_netsim_matlab();
			
			n_sign = fn_NetSim_Utilities_GenerateRandomNo(ulSeed1,ulSeed2)/NETSIM_RAND_MAX;
			if(n_sign<0.5)
			{
				*dFadingPower = -*dFadingPower; //this is done to ensure constructive/destructive fading
			}

		}

	}
	else
	{
		*dFadingPower = 0;
	}
	return 0;
}

/**
This function is used to calculate shadow loss
*/
static void Calculate_ShadowLoss(unsigned long* ulSeed1, unsigned long* ulSeed2, double* dShadowReceivedPower,double dStandardDeviation)
{
	double ldGaussianDeviate=0.0;
	double ldRandomNumber = 0.0;
	static int nIset = 0;
	static double fGset=0;
	double fFac,fRsq,fV1,fV2;	
	double d_sign1;
	if(dStandardDeviation==0)
	{
		*dShadowReceivedPower=0;
		return;
	}
	if(nIset==0)
	{
		do
		{		
			// call the random number generator function
			ldRandomNumber = fn_NetSim_Utilities_GenerateRandomNo(ulSeed1,ulSeed2)/NETSIM_RAND_MAX;		
			fV1=(double)(2.0*ldRandomNumber-1.0);
			//calculate the Random number from this function
			ldRandomNumber = fn_NetSim_Utilities_GenerateRandomNo(ulSeed1,ulSeed2)/NETSIM_RAND_MAX;		
			fV2=(double)(2.0*ldRandomNumber-1.0);
			fRsq=fV1*fV1+fV2*fV2;
		}while(fRsq>=1.0 || fRsq==0.0);
		fFac=(double)(sqrt(-2.0*log(fRsq)/fRsq));
		fGset=fV1*fFac;	
		nIset=1;
		ldGaussianDeviate = fV2*fFac;
	}
	else
	{
		nIset=0;
		ldGaussianDeviate = fGset;
	}
	d_sign1 = fn_NetSim_Utilities_GenerateRandomNo(ulSeed1,ulSeed2)/NETSIM_RAND_MAX;
	if(d_sign1 <= 0.5) 
	{
		// Assign the Received power due to shadow loss.	
		*dShadowReceivedPower = -ldGaussianDeviate * sqrt(dStandardDeviation); 
		// This is done to ensure there is constructive and destructive shadowing
	}
	else
	{	
		// Assign the Received power due to shadow loss.	
		*dShadowReceivedPower = ldGaussianDeviate * sqrt(dStandardDeviation);
	}
}

/**
This function used to calculate the received from any wireless node other wireless
nodes in the network. It check is there any interference between the adjacent channel.
If any interference then it consider that effect to calculate the received power.
*/
int fn_NetSim_IEEE802_11_PropagationModel(NETSIM_ID nodeId)
{
	PIEEE802_11_PHY_VAR phy=NULL;
	NETSIM_ID* ifids;
	NETSIM_ID ifid;
	unsigned int c;
	NETSIM_ID i;

	ifids = fn_NetSim_Stack_GetInterfaceIdbyMacProtocol(nodeId,MAC_PROTOCOL_IEEE802_11,&c);
	if(ifids)
	{
		phy = IEEE802_11_PHY(nodeId,ifids[0]);
		ifid = ifids[0];
		free(ifids);
	}
	else
		return 1;

	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		double dReceivedPower;
		double dShadowReceivedPower;
		double distance;
		bool checkinterference;
		PIEEE802_11_PHY_VAR phyi;
		if(i==nodeId)
		{
			set_cummulativeReceivedPower(i,i,MW_TO_DBM(phy->nTxPower_mw));
			continue;
		}
		ifids=fn_NetSim_Stack_GetInterfaceIdbyMacProtocol(i,MAC_PROTOCOL_IEEE802_11,&c);
		if(!ifids)
		{
			set_cummulativeReceivedPower(nodeId,i,NEGATIVE_INFINITY);
			free(ifids);
			continue;
		}
		else
		{
			ifid=ifids[0];
			free(ifids);
		}
		phyi=IEEE802_11_PHY(i,ifid);

		checkinterference=CheckFrequencyInterfrence(phy->dFrequency,
			phyi->dFrequency,
			phy->dChannelBandwidth);
		
		if(!checkinterference)
		{
			set_cummulativeReceivedPower(nodeId,i,NEGATIVE_INFINITY);
			continue;
		}

		distance=fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(nodeId),DEVICE_POSITION(i));
		if(phy->ChannelCharecteristics==NO_PATHLOSS)
		{
			set_cummulativeReceivedPower(nodeId,i,10*log10(phy->nTxPower_mw));
			continue;
		}

		calculate_pathloss(phy->nTxPower_mw,
			distance,
			phy->dFrequency,
			phy->dPathLossExponent,
			&dReceivedPower);

		Calculate_ShadowLoss(&(DEVICE(nodeId)->ulSeed[0]),
			&(DEVICE(nodeId)->ulSeed[1]),
			&dShadowReceivedPower,phy->dStandardDeviation);

		set_cummulativeReceivedPower(nodeId,i,dReceivedPower+dShadowReceivedPower);
	}
	return 0;
}
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
#include "IEEE802_11.h"
#include "IEEE802_11_Phy.h"

/// Data structure for physical layer parameters
static struct stru_802_11_Phy_Parameters_DSSS
{
	int nIndex;				
	double dRxSensitivity;
	MODULATION nModulation;
	double dDataRate;
	IEEE802_11_DSSS_PLCP_SIGNAL dsssRate;
	double processingGain;
};

static struct stru_802_11_Phy_Parameters_DSSS struPhyParameters[4] =
{
	//IEEE802.11-DSSS Phy
	{1,-97,Modulation_DBPSK,1,Rate_1Mbps,11},
	{2,-66,Modulation_DQPSK,2,Rate_2Mbps,5.5},
	{3,-57,Modulation_CCK5_5,5.5,Rate_5dot5Mbps,2},
	{4,-53,Modulation_CCK11,11,Rate_11Mbps,1},
};
#define MAX_RATE_INDEX 3
#define MIN_RATE_INDEX 0
#define TARGET_BER 1.0e-5

unsigned int get_dsss_phy_max_index(int bandwidth,IEEE802_11_PROTOCOL protocol)
{
	return MAX_RATE_INDEX;
}

unsigned int get_dsss_phy_min_index(int bandwidth,IEEE802_11_PROTOCOL protocol)
{
	return MIN_RATE_INDEX;
}

void get_dsss_phy_rate(int bandwidth,IEEE802_11_PROTOCOL protocol,double* rate,int* len)
{
	int i;
	for(i=MIN_RATE_INDEX;i<=MAX_RATE_INDEX;i++)
			rate[i]=struPhyParameters[i].dDataRate;
	*len = MAX_RATE_INDEX-MIN_RATE_INDEX+1;
}

/**
This function is used to calculate the data rate for IEEE 802.11-DSSS Phy
*/
int fn_NetSim_IEEE802_11_DSSSPhy_DataRate(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NETSIM_ID nReceiverId,NetSim_PACKET* packet)
{
	PIEEE802_11_PHY_VAR pstruPhy = IEEE802_11_PHY(nDeviceId,nInterfaceId);
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(nDeviceId,nInterfaceId);

	if(mac->rate_adaptationAlgo==GENERIC)
	{
		unsigned int index = get_rate_index(nDeviceId,nInterfaceId,nReceiverId);
		if(isIEEE802_11_CtrlPacket(packet))
			goto CONTROL_RATE;
		if(nReceiverId)
		{
			pstruPhy->PHY_TYPE.dsssPhy.dDataRate = struPhyParameters[index].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[index].dRxSensitivity;
			pstruPhy->PHY_TYPE.dsssPhy.modulation = struPhyParameters[index].nModulation;
			pstruPhy->PHY_TYPE.dsssPhy.dsssrate = struPhyParameters[index].dsssRate;
			pstruPhy->PHY_TYPE.dsssPhy.dProcessingGain = struPhyParameters[index].processingGain;
		}
		else // Broadcast packets
		{
			goto BROADCAST_RATE;
		}
	}
	else if(mac->rate_adaptationAlgo==MINSTREL)
	{
		unsigned int index = get_minstrel_rate_index(nDeviceId,nInterfaceId,nReceiverId);
		if(isIEEE802_11_CtrlPacket(packet))
			goto CONTROL_RATE;
		if(nReceiverId)
		{
			pstruPhy->PHY_TYPE.dsssPhy.dDataRate = struPhyParameters[index].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[index].dRxSensitivity;
			pstruPhy->PHY_TYPE.dsssPhy.modulation = struPhyParameters[index].nModulation;
			pstruPhy->PHY_TYPE.dsssPhy.dsssrate = struPhyParameters[index].dsssRate;
			pstruPhy->PHY_TYPE.dsssPhy.dProcessingGain = struPhyParameters[index].processingGain;
		}
		else // Broadcast packets
		{
			goto BROADCAST_RATE;
		}
	}
	else
	{
		if(nReceiverId)
		{
			if(isIEEE802_11_CtrlPacket(packet))
			{
				CONTROL_RATE:
				pstruPhy->PHY_TYPE.dsssPhy.dDataRate = struPhyParameters[0].dDataRate; // 1Mbps
				pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[0].dRxSensitivity;
				pstruPhy->PHY_TYPE.dsssPhy.modulation = struPhyParameters[0].nModulation;
				pstruPhy->PHY_TYPE.dsssPhy.dsssrate = struPhyParameters[0].dsssRate;
				pstruPhy->PHY_TYPE.dsssPhy.dProcessingGain = struPhyParameters[0].processingGain;
			}
			else
			{
				double power=get_cummlativeReceivedPower(nDeviceId,nReceiverId);

				int i;
				for(i=MAX_RATE_INDEX;i>=MIN_RATE_INDEX;i--)
				{
					double ber = calculate_BER(struPhyParameters[i].nModulation,power,pstruPhy->dChannelBandwidth);
					if((ber<=TARGET_BER && power >= struPhyParameters[i].dRxSensitivity) || i==MIN_RATE_INDEX)
					//if(power >= struPhyParameters[i].dRxSensitivity)
					{
						pstruPhy->PHY_TYPE.dsssPhy.dDataRate = struPhyParameters[i].dDataRate;
						pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[i].dRxSensitivity;
						pstruPhy->PHY_TYPE.dsssPhy.modulation = struPhyParameters[i].nModulation;
						pstruPhy->PHY_TYPE.dsssPhy.dsssrate = struPhyParameters[i].dsssRate;
						pstruPhy->PHY_TYPE.dsssPhy.dProcessingGain = struPhyParameters[i].processingGain;
						break;
					}
				}
			}
		}
		else // Broadcast packets
		{
			BROADCAST_RATE:
			pstruPhy->PHY_TYPE.dsssPhy.dDataRate = struPhyParameters[0].dDataRate; // 1Mbps
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[0].dRxSensitivity;
			pstruPhy->PHY_TYPE.dsssPhy.modulation = struPhyParameters[0].nModulation;
			pstruPhy->PHY_TYPE.dsssPhy.dsssrate = struPhyParameters[0].dsssRate;
			pstruPhy->PHY_TYPE.dsssPhy.dProcessingGain = struPhyParameters[0].processingGain;		
		}
	}
	return 0;
}

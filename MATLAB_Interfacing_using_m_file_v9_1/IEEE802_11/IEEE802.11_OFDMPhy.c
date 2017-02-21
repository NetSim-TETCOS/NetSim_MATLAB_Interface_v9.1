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
#include "ErrorModel.h"

/// Data structure for physical layer parameters
static struct stru_802_11_Phy_Parameters_OFDM
{
	int nIndex;
	double dRxSensitivity;
	MODULATION nModulation;
	double dDataRate;
	IEEE802_11_CODING nCodingRate;
	int nNBPSC;
	int nNCBPS;
	int nNDBPS;
};

static struct stru_802_11_Phy_Parameters_OFDM struPhyParameters_20MHz[8] =
{
	//IEEE802.11-OFDM Phy
	{1,-82,Modulation_BPSK,6,Coding_1_2,1,48,24},
	{2,-79,Modulation_BPSK,9,Coding_3_4,1,48,36},
	{3,-77,Modulation_QPSK,12,Coding_1_2,2,96,48},
	{4,-74,Modulation_QPSK,18,Coding_3_4,2,96,72},
	{5,-70,Modulation_16_QAM,24,Coding_1_2,4,192,96},
	{6,-66,Modulation_16_QAM,36,Coding_3_4,4,192,144},
	{7,-65,Modulation_64_QAM,48,Coding_2_3,6,288,192},
	{8,-64,Modulation_64_QAM,54,Coding_3_4,6,288,216},
};

static struct stru_802_11_Phy_Parameters_OFDM struPhyParameters_10MHz[8] =
{
	//IEEE802.11-OFDM Phy
	{1,-85,Modulation_BPSK,3,Coding_1_2,1,48,24},
	{2,-84,Modulation_BPSK,4.5,Coding_3_4,1,48,36},
	{3,-82,Modulation_QPSK,6,Coding_1_2,2,96,48},
	{4,-80,Modulation_QPSK,9,Coding_3_4,2,96,72},
	{5,-77,Modulation_16_QAM,12,Coding_1_2,4,192,96},
	{6,-73,Modulation_16_QAM,18,Coding_3_4,4,192,144},
	{7,-69,Modulation_64_QAM,24,Coding_2_3,6,288,192},
	{8,-68,Modulation_64_QAM,27,Coding_3_4,6,288,216},
};
#define MAX_RATE_INDEX 7
#define MIN_RATE_INDEX 0

#define TARGET_BER 1.0e-5

unsigned int get_ofdm_phy_max_index(int bandwidth,IEEE802_11_PROTOCOL protocol)
{
	return MAX_RATE_INDEX;
}

unsigned int get_ofdm_phy_min_index(int bandwidth,IEEE802_11_PROTOCOL protocol)
{
	return MIN_RATE_INDEX;
}

void get_ofdm_phy_rate(int bandwidth,IEEE802_11_PROTOCOL protocol,double* rate,int* len)
{
	int i;
	for(i=MIN_RATE_INDEX;i<=MAX_RATE_INDEX;i++)
		if(bandwidth==10)
			rate[i]=struPhyParameters_10MHz[i].dDataRate;
		else
			rate[i]=struPhyParameters_20MHz[i].dDataRate;
	*len = MAX_RATE_INDEX-MIN_RATE_INDEX+1;
}

static struct stru_802_11_Phy_Parameters_OFDM* get_phy_parameter_OFDM(PIEEE802_11_PHY_VAR phy)
{
	switch((int)phy->dChannelBandwidth)
	{
	case 10:
		return struPhyParameters_10MHz;
	case 20:
		return struPhyParameters_20MHz;
	default:
		fnNetSimError("Unknown channel bandwidth %lf in %s\n",phy->dChannelBandwidth,__FUNCTION__);
		return NULL;
	}
}

/**
This function is used to calculate the data rate for IEEE 802.11-OFDM Phy
*/
int fn_NetSim_IEEE802_11_OFDMPhy_DataRate(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NETSIM_ID nReceiverId,NetSim_PACKET* packet)
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(nDeviceId,nInterfaceId);
	PIEEE802_11_PHY_VAR pstruPhy = IEEE802_11_PHY(nDeviceId,nInterfaceId);
	struct stru_802_11_Phy_Parameters_OFDM *struPhyParameters = get_phy_parameter_OFDM(pstruPhy);

	if(mac->rate_adaptationAlgo==GENERIC)
	{
		unsigned int index = get_rate_index(nDeviceId,nInterfaceId,nReceiverId);
		if(isIEEE802_11_CtrlPacket(packet))
			goto CONTROL_RATE;
		if(nReceiverId)
		{
			pstruPhy->PHY_TYPE.ofdmPhy.dDataRate = struPhyParameters[index].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[index].dRxSensitivity;
			pstruPhy->PHY_TYPE.ofdmPhy.modulation = struPhyParameters[index].nModulation;
			pstruPhy->PHY_TYPE.ofdmPhy.coding = struPhyParameters[index].nCodingRate;
			pstruPhy->PHY_TYPE.ofdmPhy.nNBPSC = struPhyParameters[index].nNBPSC;
			pstruPhy->PHY_TYPE.ofdmPhy.nNCBPS = struPhyParameters[index].nNCBPS;
			pstruPhy->PHY_TYPE.ofdmPhy.nNDBPS = struPhyParameters[index].nNDBPS;
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
			pstruPhy->PHY_TYPE.ofdmPhy.dDataRate = struPhyParameters[index].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[index].dRxSensitivity;
			pstruPhy->PHY_TYPE.ofdmPhy.modulation = struPhyParameters[index].nModulation;
			pstruPhy->PHY_TYPE.ofdmPhy.coding = struPhyParameters[index].nCodingRate;
			pstruPhy->PHY_TYPE.ofdmPhy.nNBPSC = struPhyParameters[index].nNBPSC;
			pstruPhy->PHY_TYPE.ofdmPhy.nNCBPS = struPhyParameters[index].nNCBPS;
			pstruPhy->PHY_TYPE.ofdmPhy.nNDBPS = struPhyParameters[index].nNDBPS;
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
				pstruPhy->PHY_TYPE.ofdmPhy.dDataRate = struPhyParameters[0].dDataRate;
				pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[0].dRxSensitivity;
				pstruPhy->PHY_TYPE.ofdmPhy.modulation = struPhyParameters[0].nModulation;
				pstruPhy->PHY_TYPE.ofdmPhy.coding = struPhyParameters[0].nCodingRate;
				pstruPhy->PHY_TYPE.ofdmPhy.nNBPSC = struPhyParameters[0].nNBPSC;
				pstruPhy->PHY_TYPE.ofdmPhy.nNCBPS = struPhyParameters[0].nNCBPS;
				pstruPhy->PHY_TYPE.ofdmPhy.nNDBPS = struPhyParameters[0].nNDBPS;
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
						pstruPhy->PHY_TYPE.ofdmPhy.dDataRate = struPhyParameters[i].dDataRate;
						pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[i].dRxSensitivity;
						pstruPhy->PHY_TYPE.ofdmPhy.modulation = struPhyParameters[i].nModulation;
						pstruPhy->PHY_TYPE.ofdmPhy.coding = struPhyParameters[i].nCodingRate;
						pstruPhy->PHY_TYPE.ofdmPhy.nNBPSC = struPhyParameters[i].nNBPSC;
						pstruPhy->PHY_TYPE.ofdmPhy.nNCBPS = struPhyParameters[i].nNCBPS;
						pstruPhy->PHY_TYPE.ofdmPhy.nNDBPS = struPhyParameters[i].nNDBPS;
						break;
					}
				}
			}
		}
		else // Broadcast packets
		{
			BROADCAST_RATE:
			pstruPhy->PHY_TYPE.ofdmPhy.dDataRate = struPhyParameters[0].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[0].dRxSensitivity;
			pstruPhy->PHY_TYPE.ofdmPhy.modulation = struPhyParameters[0].nModulation;
			pstruPhy->PHY_TYPE.ofdmPhy.coding = struPhyParameters[0].nCodingRate;
			pstruPhy->PHY_TYPE.ofdmPhy.nNBPSC = struPhyParameters[0].nNBPSC;
			pstruPhy->PHY_TYPE.ofdmPhy.nNCBPS = struPhyParameters[0].nNCBPS;
			pstruPhy->PHY_TYPE.ofdmPhy.nNDBPS = struPhyParameters[0].nNDBPS;		
		}
	}
	return 0;
}

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

IEEE802_11_RADIO_STATE get_radio_state(PIEEE802_11_PHY_VAR phy)
{
	return phy->radioState;
}

bool set_radio_state(PIEEE802_11_PHY_VAR phy,IEEE802_11_RADIO_STATE state)
{
	if(phy->radioState==RX_OFF)
		return false;
	else
		phy->radioState = state;
	return true;
}

bool is_radio_idle(PIEEE802_11_PHY_VAR phy)
{
	return (phy->radioState==RX_ON_IDLE);
}

/**
Function used to update the medium during the PHYSICAL OUT event. Check 
change in the Medium before (previous) and present when the nDevice started 
transmission. If any change then, if the device in the BACKING_OFF state, then 
pauseBackoff.  
*/
int fn_NetSim_IEEE802_11_UpdatePowerduetoTxStart(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	double dTime = pstruEventDetails->dEventTime;
	NETSIM_ID nLoop, nLoopInterface;
	PIEEE802_11_PHY_VAR pstruPhy;
	PIEEE802_11_MAC_VAR pstruMac;
	PIEEE802_11_PHY_VAR currphy=IEEE802_11_PHY(nDeviceId,nInterfaceId);

	for(nLoop = 1;nLoop <= NETWORK->nDeviceCount;nLoop++)
	{
		for(nLoopInterface = 1;nLoopInterface<= DEVICE(nLoop)->nNumOfInterface;nLoopInterface++)
		{
			if(!DEVICE_MACLAYER(nLoop,nLoopInterface) || 
				DEVICE_MACLAYER(nLoop,nLoopInterface)->nMacProtocolId != MAC_PROTOCOL_IEEE802_11)
				continue;

			pstruPhy = IEEE802_11_PHY(nLoop,nLoopInterface);
			pstruMac = IEEE802_11_MAC(nLoop,nLoopInterface);

			if(!CheckFrequencyInterfrence(currphy->dFrequency,pstruPhy->dFrequency,currphy->dChannelBandwidth))
				continue; //Different band

			pstruPhy->dReceivedPower_mw += DBM_TO_MW(get_cummlativeReceivedPower(nDeviceId,nLoop));
			pstruPhy->dReceivedPower_dbm = MW_TO_DBM(pstruPhy->dReceivedPower_mw);

			/*if(!isMediumIdle(pstruPhy))			
			{			
				if(pstruMac->currMacState == BACKING_OFF)
					fn_NetSim_IEEE802_11_CSMACA_PauseBackOff(nLoop,nLoopInterface,dTime);	
			}*/		
		}
	}
	return 0;
}

/**
This function is called when UPDATE_DEVICE_STATUS sub event is trigged.
This event is added in the PHYSICAL_OUT event at the PHYSICAL_IN_EVENT time.
Function used to update the medium at PHYSICAL IN event time. Check any change in 
the Medium before (previous) and present when the nDevice reached the receiver. 
If any change then add the CS event to start transmission by other devices.This 
event resume the BACKOFF and start transmission of other devices if they have
packet to transmit. 
*/

int	fn_NetSim_IEEE802_11_UpdatePowerduetoTxEnd(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	double dTime = pstruEventDetails->dEventTime;
	NETSIM_ID nLoop, nLoopInterface;
	PIEEE802_11_PHY_VAR pstruPhy;
	PIEEE802_11_MAC_VAR pstruMac;
	PIEEE802_11_PHY_VAR currphy=IEEE802_11_PHY(nDeviceId,nInterfaceId);
	NetSim_PACKET* p,*t;
	p=pstruEventDetails->pPacket;

	set_radio_state(IEEE802_11_CURR_PHY,RX_ON_IDLE);
	set_mac_state_after_txend(IEEE802_11_CURR_MAC);

	for(nLoop = 1;nLoop <= NETWORK->nDeviceCount;nLoop++)
	{
		for(nLoopInterface = 1;nLoopInterface<= DEVICE(nLoop)->nNumOfInterface;nLoopInterface++)
		{
			if(!DEVICE_MACLAYER(nLoop,nLoopInterface) || 
				DEVICE_MACLAYER(nLoop,nLoopInterface)->nMacProtocolId != MAC_PROTOCOL_IEEE802_11)
				continue;

			pstruPhy = IEEE802_11_PHY(nLoop,nLoopInterface);
			pstruMac = IEEE802_11_MAC(nLoop,nLoopInterface);

			if(!CheckFrequencyInterfrence(currphy->dFrequency,pstruPhy->dFrequency,currphy->dChannelBandwidth))
				continue; //Different band

			pstruPhy->dReceivedPower_mw -= DBM_TO_MW(get_cummlativeReceivedPower(nDeviceId,nLoop));
			pstruPhy->dReceivedPower_dbm = MW_TO_DBM(pstruPhy->dReceivedPower_mw);

			if(isSTAIdle(pstruMac,pstruPhy) && isMediumIdle(pstruPhy))			
			{			
				if(!pstruMac->nBackOffCounter)
					fn_NetSim_IEE802_11_MacReInit(nLoop,nLoopInterface);					
			}
		}		
	}
	while(p)
	{
		t=p;
		p=p->pstruNextPacket;
		fn_NetSim_Packet_FreePacket(t);
	}
	return 0;	
}

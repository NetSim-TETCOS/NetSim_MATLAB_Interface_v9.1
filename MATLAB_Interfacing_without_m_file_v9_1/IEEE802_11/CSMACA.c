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
#include "IEEE802_11_MAC_Frame.h"

//Function prototype
static void fn_NetSim_IEEE802_11_CSMACA_RandomBackOffTimeCalculation();
static void fn_NetSim_IEEE802_11_CSMACA_StartBackOff();
bool fn_NetSim_IEEE802_11_CSMACA_CheckRetryLimit(PIEEE802_11_MAC_VAR mac);

int fn_NetSim_IEEE802_11_CSMACA_Init()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;

	NetSim_PACKET* packet;	

	if(!isCurrSTAIdle)
		return -1; //Either mac is busy or radio is busy

	//Check buffer has packets to send	
	if(mac->currentProcessingPacket)
		packet = mac->currentProcessingPacket;
	else
	{
		// Get packet from buffer
		int n;
		packet = fn_NetSim_IEEE802_11e_GetPacketFromQueue(&mac->qosList,mac->nNumberOfAggregatedPackets,&n);
		if(!packet)
			return -2;
		mac->currentProcessingPacket=packet;
	}

	//Call RTS-CTS
	fn_NetSim_IEEE802_11_RTS_CTS_Init();

	//Call IEEE802.11e to update cw based on QoS
	fn_NetSim_IEEE802_11e_Updatecw(packet);

	// Add MAC_OUT_EVENT
	pstruEventDetails->nPacketId = packet->nPacketId;
	if(packet->pstruAppData)
	{
		pstruEventDetails->nSegmentId = packet->pstruAppData->nSegmentId;
		pstruEventDetails->nApplicationId = packet->pstruAppData->nApplicationId;
	}
	else
	{
		pstruEventDetails->nSegmentId = 0;
		pstruEventDetails->nApplicationId = 0;
	}

	if(packet->pstruNetworkData)
		pstruEventDetails->dPacketSize = packet->pstruNetworkData->dPacketSize;
	else
		pstruEventDetails->dPacketSize = packet->pstruMacData->dPacketSize;
	pstruEventDetails->pPacket = packet;
	pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_11;
	pstruEventDetails->nSubEventType = CS;
	pstruEventDetails->nEventType = MAC_OUT_EVENT;
	fnpAddEvent(pstruEventDetails);
	return 0;
}

bool fn_NetSim_IEEE802_11_CSMACA_CS()
{
	return (isCurrSTAIdle && isMediumIdle(IEEE802_11_CURR_PHY));
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function called when CHECK_NAV subevent triggered. It is called  if the Medium 
is IDLE and a packet in the Access buffer to transmit.
If NAV <=0 then change the state to Wait_DIFS and add DIFS_END subevent.
If NAV > 0 then change the state to WF_NAV and add NAV_END subevent.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_IEEE802_11_CSMACA_CheckNAV()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_CURR_PHY;

	if(pstruEventDetails->dEventTime >= mac->dNAV)
	{
		// Change the State
		IEEE802_11_Change_Mac_State(mac,Wait_DIFS);		
		// Add DIFS_END subevent
		pstruEventDetails->dEventTime += phy->DIFS;
		pstruEventDetails->nSubEventType =  DIFS_END;
		fnpAddEvent(pstruEventDetails);	
	}
	return 0;
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When the buffer has packet to transmit, the MAC LAYER sense the Medium. If it is IDLE,
then it will wait for DCF Inter Frame Space (DIFS) time before start transmission. 
At the end of DIFS time check the Medium. If Medium is IDLE, then change the State 
to BACKING_OFF, call the function to start back off. If Medium is BUSY, then change 
the state to IDLE. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void fn_NetSim_IEEE802_11_CSMACA_DIFSEnd()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	// check the medium 
	if(isMediumIdle(IEEE802_11_CURR_PHY))
	{
		// Change the state
		IEEE802_11_Change_Mac_State(mac,BACKING_OFF);
		
		// Start Back off
		fn_NetSim_IEEE802_11_CSMACA_StartBackOff();
	}
	else if(mac->currMacState==Wait_DIFS)
	{
		IEEE802_11_Change_Mac_State(mac,MAC_IDLE);		
	}
}

/**
This function start the Backoff process. If backoff counter != 0,it adds the backoff event at (event 
time + backoff time) else it generate the random backoff time and add the backoff event. 
*/
static void fn_NetSim_IEEE802_11_CSMACA_StartBackOff()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_CURR_PHY;
	mac->dBackOffStartTime = pstruEventDetails->dEventTime;
	if(mac->nBackOffCounter == 0)
		fn_NetSim_IEEE802_11_CSMACA_RandomBackOffTimeCalculation();

	mac->dBackOffStartTime = pstruEventDetails->dEventTime;
	mac->dBackoffLeftTime = mac->nBackOffCounter*
		phy->plmeCharacteristics.aSlotTime +
		mac->dBackOffStartTime;

	pstruEventDetails->dEventTime += phy->plmeCharacteristics.aSlotTime;
	pstruEventDetails->nSubEventType = BACKOFF;
	fnpAddEvent(pstruEventDetails);
}


/**
This function invokes the Back off time calculation.
BackoffTime = RandomNumber(Between CW) * SlotTime.
*/
static void fn_NetSim_IEEE802_11_CSMACA_RandomBackOffTimeCalculation()
{
	double dRandomNumber = 0.0;
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;	
	PIEEE802_11_PHY_VAR phy=IEEE802_11_CURR_PHY;	
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	
	dRandomNumber = fn_NetSim_Utilities_GenerateRandomNo(&NETWORK->ppstruDeviceList[nDeviceId-1]->ulSeed[0],&NETWORK->ppstruDeviceList[nDeviceId-1]->ulSeed[1]);
	dRandomNumber /= NETSIM_RAND_MAX;
	dRandomNumber *= mac->nCWcurrent;
	mac->nBackOffCounter = (int)dRandomNumber;
}

bool fn_NetSim_IEEE802_11_CSMACA_Backoff()
{
	double dTime = pstruEventDetails->dEventTime;
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_CURR_PHY;

	assert(mac->nBackOffCounter>=0);

	if(phy->radioState != RX_ON_IDLE)
	{
		if(mac->currMacState==BACKING_OFF)
			IEEE802_11_Change_Mac_State(mac,MAC_IDLE);					
		mac->metrics.nBackoffFailedCount++;
		mac->dBackoffLeftTime=0;
		mac->nBackOffCounter=0;
		return false;
	}

	if(isMediumIdle(phy))
	{
		mac->nBackOffCounter--;
		if(mac->nBackOffCounter>0)
		{
			pstruEventDetails->dEventTime += phy->plmeCharacteristics.aSlotTime;
			pstruEventDetails->nSubEventType = BACKOFF;
			fnpAddEvent(pstruEventDetails);
			return false;
		}
		else
		{
			mac->nBackOffCounter=0;
			mac->dBackoffLeftTime=0;
			mac->metrics.nBackoffSuccessCount++;
			return true;
		}
	}
	else
	{
		if(mac->nBackOffCounter<=1)
		{
			if(mac->currMacState==BACKING_OFF)
				IEEE802_11_Change_Mac_State(mac,MAC_IDLE);					
			mac->metrics.nBackoffFailedCount++;
			mac->dBackoffLeftTime=0;
			mac->nBackOffCounter=0;
			return false;
		}
		else
		{
			pstruEventDetails->dEventTime += phy->plmeCharacteristics.aSlotTime;
			pstruEventDetails->nSubEventType = BACKOFF;
			fnpAddEvent(pstruEventDetails);
			return false;
		}
	}
}

/**
This function is called to create and send an Ack after receiving the DATA 
from the destination or from the AP
*/
int fn_NetSim_IEEE802_11_CSMACA_SendACK()
{
	NetSim_PACKET* packet=fn_NetSim_IEEE802_11_CreateAckPacket(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId,
		pstruEventDetails->pPacket,
		pstruEventDetails->dEventTime);

	//Free the data packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);

	// Add SEND ACK subevent
	pstruEventDetails->dEventTime += IEEE802_11_CURR_PHY->SIFS;
	pstruEventDetails->dPacketSize = packet->pstruMacData->dPacketSize;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
	pstruEventDetails->pPacket = packet;
	fnpAddEvent(pstruEventDetails);	
	// Change the state
	IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,TXing_ACK);
	return 0;
}

void fn_NetSim_IEEE802_11_CSMACA_ProcessAck()
{
	NetSim_PACKET* p=IEEE802_11_CURR_MAC->currentProcessingPacket;

	//Call rate adaptation
	if(IEEE802_11_CURR_MAC->rate_adaptationAlgo==GENERIC)
	packet_recv_notify(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId,
		pstruEventDetails->pPacket->nTransmitterId);
	else if(IEEE802_11_CURR_MAC->rate_adaptationAlgo==MINSTREL)
		DoReportDataOk(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId,
		pstruEventDetails->pPacket->nTransmitterId);

	while (p)
	{
		NetSim_PACKET* t=p;
		p=p->pstruNextPacket;
		if(t->ReceiveAckNotification)
			t->ReceiveAckNotification(pstruEventDetails->pPacket);
		fn_NetSim_Packet_FreePacket(t);
	}
	IEEE802_11_CURR_MAC->currentProcessingPacket=NULL;
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,MAC_IDLE);
	fn_NetSim_IEE802_11_MacReInit(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId);

}

void fn_NetSim_IEEE802_11_CSMACA_ProcessBlockAck()
{
	UINT success=0,fail=0;
	int i=0;
	NetSim_PACKET* p=IEEE802_11_CURR_MAC->currentProcessingPacket;
	PIEEE802_11_BLOCKACK back = (PIEEE802_11_BLOCKACK)PACKET_MACPROTOCOLDATA(pstruEventDetails->pPacket);
	NetSim_PACKET* prev=NULL;
	int flag = 1;
	while (p)
	{
		if(BIT_IS_SET_64(back->BitMap,i))
		{
			NetSim_PACKET* t=p;
			success++;
			if(prev)
			{
				prev->pstruNextPacket=t->pstruNextPacket;
				p=p->pstruNextPacket;
				fn_NetSim_Packet_FreePacket(t);
				continue;
			}
			else
			{
				p=p->pstruNextPacket;
				IEEE802_11_CURR_MAC->currentProcessingPacket=p;
				fn_NetSim_Packet_FreePacket(t);
				continue;
			}
		}
		else
		{
			fail++;
			flag = 0;
			prev=p;
			p=p->pstruNextPacket;
			continue;
		}
	}
	if(IEEE802_11_CURR_MAC->rate_adaptationAlgo==GENERIC)
	{
		//Call rate adaptation
		if(flag) //All packet received.
			packet_recv_notify(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId,
			pstruEventDetails->pPacket->nTransmitterId);
		else //Some packet dropped
			packet_drop_notify(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId,
			pstruEventDetails->pPacket->nTransmitterId);
	}
	else if(IEEE802_11_CURR_MAC->rate_adaptationAlgo==MINSTREL)
	{
		DoReportAmpduStatus(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId,
			pstruEventDetails->pPacket->nTransmitterId,
			success,fail);
	}

	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,MAC_IDLE);
	fn_NetSim_IEE802_11_MacReInit(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId);
}

void fn_NetSim_IEEE802_11_CSMACA_AddAckTimeOut(NetSim_PACKET* packet,NETSIM_ID devId,NETSIM_ID devIf)
{
	NetSim_EVENTDETAILS pevent;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(devId,devIf);
	double acktime;

	acktime = ceil(packet->pstruPhyData->dStartTime
		+ phy->plmeCharacteristics.aSIFSTime
		+ get_preamble_time(phy)
		+ ((getAckSize(phy) * 8)/phy->dControlFrameDataRate));

	pevent.dEventTime=acktime+2;
	pevent.dPacketSize=0;
	pevent.nDeviceId=devId;
	pevent.nDeviceType=DEVICE_TYPE(devId);
	pevent.nEventType=TIMER_EVENT;
	pevent.nInterfaceId=devIf;
	pevent.nPacketId=packet->nPacketId;
	pevent.nProtocolId=MAC_PROTOCOL_IEEE802_11;
	if(packet->pstruAppData)
	{
		pevent.nSegmentId=packet->pstruAppData->nSegmentId;
		pevent.nApplicationId=packet->pstruAppData->nApplicationId;
	}
	else
	{
		pevent.nSegmentId=0;
		pevent.nApplicationId=0;
	}
	pevent.nSubEventType=ACK_TIMEOUT;
	pevent.pPacket=NULL;
	pevent.szOtherDetails=NULL;
	fnpAddEvent(&pevent);
}

void fn_NetSim_IEEE802_11_CSMA_AckTimeOut()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	bool isRetry=false;
	if(mac->currMacState == Wait_ACK)
	{
		//Call rate adaptation
		if(mac->rate_adaptationAlgo==GENERIC)
			packet_drop_notify(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId,
			mac->currentProcessingPacket->nReceiverId);

		IEEE802_11_Change_Mac_State(mac,MAC_IDLE);

		if(mac->rate_adaptationAlgo == MINSTREL)
			isRetry = Minstrel_DoNeedDataSend(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId,
			mac->currentProcessingPacket->nReceiverId);
		else
			isRetry = fn_NetSim_IEEE802_11_CSMACA_CheckRetryLimit(mac);

		if(isRetry)
		{
			mac->nRetryCount++;
			if(mac->rate_adaptationAlgo==MINSTREL)
				Minstrel_ReportDataFailed(pstruEventDetails->nDeviceId,
				pstruEventDetails->nInterfaceId,
				mac->currentProcessingPacket->nReceiverId);
			fn_NetSim_IEEE802_11_CSMACA_IncreaseCW(mac);
			fn_NetSim_IEEE802_11_CSMACA_Init();
		}
		else
		{
			if(mac->rate_adaptationAlgo == MINSTREL)
				Minstrel_ReportFinalDataFailed(pstruEventDetails->nDeviceId,
				pstruEventDetails->nInterfaceId,
				mac->currentProcessingPacket->nReceiverId);

			if(mac->currentProcessingPacket)
			{
				NetSim_PACKET* pstruPacket = mac->currentProcessingPacket;

				if(pstruPacket->DropNotification)
					pstruPacket->DropNotification(pstruPacket);
				fn_NetSim_Packet_FreePacket(mac->currentProcessingPacket);
				mac->currentProcessingPacket=NULL;
			}
			mac->nRetryCount = 0;
			fn_NetSim_IEEE802_11_CSMACA_Init();
		}
	}
}

/**
 This function invoke the retry limit checking. If active frame retry count is less than
 active frame retry limit, then set move flag as one to allow the frame retransmission.
 Otherwise, set move flag as zero to drop the active frame 
 */
bool fn_NetSim_IEEE802_11_CSMACA_CheckRetryLimit(PIEEE802_11_MAC_VAR mac)
{
	if(mac->nRetryCount < mac->nRetryLimit-1)
		return true;
	else
		return false;
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function is used to expand the CW. 
else if CWcurrent >= CWmax, then CWcurrent is set to CWmax the Maximum CW.
else CWcurrent = (nCWcurrent * 2) + 1.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void fn_NetSim_IEEE802_11_CSMACA_IncreaseCW(PIEEE802_11_MAC_VAR mac)
{
	mac->nCWcurrent = min((mac->nCWcurrent * 2) + 1,mac->currCwMax);
}

/**
This function is called to create and send an Ack after receiving the DATA 
from the destination or from the AP
*/
int fn_NetSim_IEEE802_11_CSMACA_SendBlockACK()
{
	NetSim_PACKET* packet;
	NetSim_PACKET* p=pstruEventDetails->pPacket;

	if(IEEE802_11_CURR_MAC->blockAckPacket)
		packet = IEEE802_11_CURR_MAC->blockAckPacket;
	else
	{
		packet=fn_NetSim_IEEE802_11_CreateBlockAckPacket(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId,
			pstruEventDetails->pPacket,
			pstruEventDetails->dEventTime);
		IEEE802_11_CURR_MAC->blockAckPacket=packet;
	}

	set_blockack_bitmap(packet,pstruEventDetails->pPacket);

	if(!is_more_fragment_coming(pstruEventDetails->pPacket))
	{
		// Add SEND ACK subevent
		pstruEventDetails->dEventTime += IEEE802_11_CURR_PHY->SIFS;
		pstruEventDetails->dPacketSize = packet->pstruMacData->dPacketSize;
		pstruEventDetails->nSubEventType = 0;
		pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
		pstruEventDetails->pPacket = packet;
		fnpAddEvent(pstruEventDetails);	
		// Change the state
		IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,TXing_ACK);
		IEEE802_11_CURR_MAC->blockAckPacket=NULL;
	}

	//Free the data packet
	fn_NetSim_Packet_FreePacket(p);
	return 0;
}
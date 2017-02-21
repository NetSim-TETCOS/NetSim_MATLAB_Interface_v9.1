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
#include "IEEE802_11_MAC_Frame.h"
#include "IEEE802_11_Phy.h"

//Function prototype
static int fn_NetSim_IEEE802_11_MacInit();
static void fn_NetSim_IEEE802_11_SendToPhy();
static void forward_to_other_interface(NetSim_PACKET* packet);
static void add_to_mac_queue(NetSim_PACKET* packet);

void fn_NetSim_IEEE802_11_MacOut()
{
	switch(pstruEventDetails->nSubEventType)
	{
	case 0:
		fn_NetSim_IEEE802_11_MacInit();
		fn_NetSim_IEEE802_11_CSMACA_Init();
		break;
	case CS:
		if(fn_NetSim_IEEE802_11_CSMACA_CS())
			fn_NetSim_IEEE802_11_CSMACA_CheckNAV();
		break;
	case DIFS_END:
		fn_NetSim_IEEE802_11_CSMACA_DIFSEnd();
		break;
	case BACKOFF:
		if(fn_NetSim_IEEE802_11_CSMACA_Backoff())
			fn_NetSim_IEEE802_11_SendToPhy();
		break;
	case SEND_ACK:
		if(IEEE802_11_CURR_MAC->macAggregationStatus)
			fn_NetSim_IEEE802_11_CSMACA_SendBlockACK();
		else
			fn_NetSim_IEEE802_11_CSMACA_SendACK();
		break;
	case SEND_CTS:
		fn_NetSim_IEEE802_11_RTS_CTS_SendCTS();
		break;
	case SEND_MPDU:
		fn_NetSim_IEEE802_11_SendToPhy();
		break;
	default:
		fnNetSimError("Unknown subevent %d for IEEE802_11 MAC OUT.",pstruEventDetails->nSubEventType);
		break;
	}
}

int fn_NetSim_IEEE802_11_MacIn()
{
	NetSim_EVENTDETAILS pevent;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NetSim_PACKET* data;
	if(isIEEE802_11_CtrlPacket(packet))
	{
		fn_NetSim_Process_CtrlPacket();
		return 0;
	}
	IEEE802_11_CURR_MAC->metrics.nReceivedFrameCount++;

	memcpy(&pevent,pstruEventDetails,sizeof pevent);
	data = fn_NetSim_Packet_CopyPacket(packet);
	if(DEVICE_NWLAYER(pstruEventDetails->nDeviceId))
	{
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetNWProtocol(pstruEventDetails->nDeviceId);
		pstruEventDetails->nEventType=NETWORK_IN_EVENT;
		fnpAddEvent(pstruEventDetails);
	}
	else
	{
		PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;

		if(mac->BSSType==INFRASTRUCTURE)
		{
			if(isPacketforsameInfrastructureBSS(mac,packet))
			{
				fn_NetSim_802_11_InfrastructureBSS_UpdateReceiver(packet);
				add_to_mac_queue(packet);
			}
			else
				forward_to_other_interface(packet);
		}
		else if(mac->BSSType==MESH)
		{
			if(isPacketforsameMeshBSS(mac,packet))
			{
				fn_NetSim_802_11_MeshBSS_UpdateReceiver(packet);
				add_to_mac_queue(packet);
			}
			else
				forward_to_other_interface(packet);
		}
		else
		{
			fnNetSimError("BSSType %d is not implemented for IEEE802.11 protocol");
			return -1;
		}
	}
	if(data->nDestinationId) //No ack for broadcast
	{
		memcpy(pstruEventDetails,&pevent,sizeof* pstruEventDetails);
		//Add event to send ack
		pstruEventDetails->nEventType=MAC_OUT_EVENT;
		pstruEventDetails->nSubEventType=SEND_ACK;
		pstruEventDetails->pPacket=data;
		fnpAddEvent(pstruEventDetails);
		IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,TXing_ACK);
	}
	return 0;
}

static int fn_NetSim_IEEE802_11_MacInit()
{
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;
	NetSim_BUFFER* pstruBuffer = DEVICE_ACCESSBUFFER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);

	while(fn_NetSim_GetBufferStatus(pstruBuffer))
	{
		NetSim_PACKET* pstruPacket = fn_NetSim_Packet_GetPacketFromBuffer(pstruBuffer,1);

		if(mac->BSSType==INFRASTRUCTURE)
		{
			if(isPacketforsameInfrastructureBSS(mac,pstruPacket))
				fn_NetSim_802_11_InfrastructureBSS_UpdateReceiver(pstruPacket);
			else
			{
				fn_NetSim_Packet_FreePacket(pstruPacket);
				continue;
			}
		}
		else if(mac->BSSType==MESH)
		{
			if(isPacketforsameMeshBSS(mac,pstruPacket))
				fn_NetSim_802_11_MeshBSS_UpdateReceiver(pstruPacket);
			else
			{
				fn_NetSim_Packet_FreePacket(pstruPacket);
				continue;
			}
		}
		else
		{
			fnNetSimError("BSSType %d is not implemented for IEEE802.11 protocol");
			return -1;
		}

		pstruPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime	;
		pstruPacket->pstruMacData->dPayload = pstruPacket->pstruNetworkData->dPacketSize;
		//Call IEEE802.11e
		fn_NetSim_IEEE802_11e_AddtoQueue(mac,pstruPacket);
	}
	return 0;
}

void IEEE802_11_Change_Mac_State(PIEEE802_11_MAC_VAR mac,IEEE802_11_MAC_STATE state)
{
	mac->prevMacState=mac->currMacState;
	mac->currMacState=state;
}

void set_mac_state_after_txend(PIEEE802_11_MAC_VAR mac)
{
	mac->prevMacState=mac->currMacState;
	switch(mac->currMacState)
	{
	case TXing_ACK:
		IEEE802_11_Change_Mac_State(mac,MAC_IDLE);
		break;
	case TXing_MPDU:
		IEEE802_11_Change_Mac_State(mac,Wait_ACK);
		break;
	case Txing_BroadCast:
		IEEE802_11_Change_Mac_State(mac,MAC_IDLE);
		break;
	case TXing_CTS:
		IEEE802_11_Change_Mac_State(mac,MAC_IDLE);
		break;
	case TXing_RTS:
		IEEE802_11_Change_Mac_State(mac,Wait_CTS);
		break;
	default:
		fnNetSimError("Unknown mac state %d is %s",mac->currMacState,__FUNCTION__);
		IEEE802_11_Change_Mac_State(mac,MAC_IDLE);
	}
}

void set_mac_state_for_tx(PIEEE802_11_MAC_VAR mac,NetSim_PACKET* p)
{
	switch(p->nControlDataType)
	{
	case WLAN_RTS:
		IEEE802_11_Change_Mac_State(mac,TXing_RTS);
		break;
	case WLAN_CTS:
		IEEE802_11_Change_Mac_State(mac,TXing_CTS);
		break;
	case WLAN_ACK:
		IEEE802_11_Change_Mac_State(mac,TXing_ACK);
		break;
	default:
		if(p->nDestinationId)
			IEEE802_11_Change_Mac_State(mac,TXing_MPDU);
		else
			IEEE802_11_Change_Mac_State(mac,Txing_BroadCast);
		break;
	}
}
static void fn_NetSim_IEEE802_11_SendToPhy()
{
	double dPacketSize=0;	
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	
	NetSim_PACKET *p=mac->currentProcessingPacket;
	NetSim_PACKET* pstruPacket;
	unsigned int i=1;

	if(!mac->currentProcessingPacket)
	{
		fnNetSimError("%s is called without mac->currentProcessingPacket",__FUNCTION__);
		return;
	}
	set_mac_state_for_tx(mac,p);

	if(mac->currentProcessingPacket->nDestinationId)
		pstruPacket = fn_NetSim_Packet_CopyPacketList(mac->currentProcessingPacket);
	else
	{
		pstruPacket = mac->currentProcessingPacket;
		mac->currentProcessingPacket = NULL;
	}
	
	pstruEventDetails->pPacket = pstruPacket;
	// Add the MAC header
	while(pstruPacket)
	{
		if(!isIEEE802_11_CtrlPacket(pstruPacket))
		{
			fn_NetSim_IEEE802_11_Add_MAC_Header(nDeviceId,nInterfaceId,pstruPacket,i);
			mac->metrics.nTransmittedFrameCount++;
		}
		dPacketSize += pstruPacket->pstruMacData->dPacketSize;
		pstruPacket=pstruPacket->pstruNextPacket;
		i++;
	}
	pstruPacket = pstruEventDetails->pPacket;

	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;	
	pstruEventDetails->nPacketId = pstruPacket->nPacketId;
	if(pstruPacket->pstruAppData)
	{
		pstruEventDetails->nSegmentId = pstruPacket->pstruAppData->nSegmentId;
		pstruEventDetails->nApplicationId = pstruPacket->pstruAppData->nApplicationId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->dPacketSize = dPacketSize;
	fnpAddEvent(pstruEventDetails);
}

static void forward_to_other_interface(NetSim_PACKET* packet)
{
	NETSIM_ID i;
	NETSIM_ID inf=pstruEventDetails->nInterfaceId;
	NETSIM_ID d=pstruEventDetails->nDeviceId;
	fn_NetSim_IEEE802_11_FreePacket(packet);
	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_NULL;
	packet->pstruMacData->Packet_MACProtocol=NULL;
	packet->pstruMacData->dPacketSize=0;
	packet->pstruMacData->dPayload=0;
	packet->pstruMacData->dOverhead=0;
	for(i=1;i<=DEVICE(d)->nNumOfInterface;i++)
	{
		NetSim_BUFFER* buf=DEVICE_ACCESSBUFFER(d,i);
		NetSim_PACKET* p;

		if(i==inf)
			continue;
		if(!fn_NetSim_GetBufferStatus(buf))
		{
			pstruEventDetails->nInterfaceId=i;
			pstruEventDetails->nEventType=MAC_OUT_EVENT;
			pstruEventDetails->nProtocolId=fn_NetSim_Stack_GetMacProtocol(d,i);
			pstruEventDetails->nSubEventType=0;
			pstruEventDetails->pPacket=NULL;
			fnpAddEvent(pstruEventDetails);
		}
		p=fn_NetSim_Packet_CopyPacket(packet);
		fn_NetSim_Packet_AddPacketToList(buf,p,0);
	}
	fn_NetSim_Packet_FreePacket(packet);
}

static void add_to_mac_queue(NetSim_PACKET* packet)
{
	NETSIM_ID inf=pstruEventDetails->nInterfaceId;
	NETSIM_ID d=pstruEventDetails->nDeviceId;
	NetSim_BUFFER* buf=DEVICE_ACCESSBUFFER(d,inf);

	fn_NetSim_IEEE802_11_FreePacket(packet);
	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_NULL;
	packet->pstruMacData->Packet_MACProtocol=NULL;
	packet->pstruMacData->dPacketSize=0;
	packet->pstruMacData->dPayload=0;
	packet->pstruMacData->dOverhead=0;

	if(!fn_NetSim_GetBufferStatus(buf))
	{
		pstruEventDetails->nInterfaceId=inf;
		pstruEventDetails->nEventType=MAC_OUT_EVENT;
		pstruEventDetails->nProtocolId=fn_NetSim_Stack_GetMacProtocol(d,inf);
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=NULL;
		fnpAddEvent(pstruEventDetails);
	}
	fn_NetSim_Packet_AddPacketToList(buf,packet,0);
}

void fn_NetSim_IEE802_11_MacReInit(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	NetSim_EVENTDETAILS pevent;
	memcpy(&pevent,pstruEventDetails,sizeof pevent);
	pstruEventDetails->nDeviceId=nDeviceId;
	pstruEventDetails->nInterfaceId=nInterfaceId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(nDeviceId);
	pstruEventDetails->dPacketSize=0;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=MAC_OUT_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=NULL;
	pstruEventDetails->szOtherDetails=NULL;
	IEEE802_11_CURR_MAC->nRetryCount = 0;
	fn_NetSim_IEEE802_11_CSMACA_Init();
	memcpy(pstruEventDetails,&pevent,sizeof pevent);
}

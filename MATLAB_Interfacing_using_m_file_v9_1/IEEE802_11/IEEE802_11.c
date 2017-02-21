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

//Function prototype
int fn_NetSim_IEEE802_11_Configure_F(void** var);
int fn_NetSim_IEEE802_11_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type);

_declspec(dllexport) int fn_NetSim_IEEE802_11_Configure(void** var)
{
	return fn_NetSim_IEEE802_11_Configure_F(var);
}

_declspec(dllexport) int fn_NetSim_IEEE802_11_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type)
{
	fn_matlab_init();
	return fn_NetSim_IEEE802_11_Init_F(NETWORK_Formal,
		pstruEventDetails_Formal,pszAppPath_Formal,
		pszWritePath_Formal,
		nVersion_Type);
}

_declspec(dllexport) int fn_NetSim_IEEE802_11_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		fn_NetSim_IEEE802_11_MacOut();
		break;
	case MAC_IN_EVENT:
		fn_NetSim_IEEE802_11_MacIn();
		break;
	case PHYSICAL_OUT_EVENT:
		fn_NetSim_IEEE802_11_PhyOut();
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_IEEE802_11_PhyIn();
		break;
	case TIMER_EVENT:
		fn_NetSim_IEEE802_11_Timer();
		break;
	default:
		fnNetSimError("Unknown event type %d for IEEE802.11 protocol",pstruEventDetails->nEventType);
		break;
	}
	return 0;
}

/**
This function is called by NetworkStack.dll, while writing the event trace 
to get the sub event as a string.
*/
_declspec(dllexport) char* fn_NetSim_IEEE802_11_Trace(int nSubEvent)
{
	return GetStringIEEE802_11_Subevent(nSubEvent);
}

/**
This function is called by NetworkStack.dll, to free the WLAN protocol
pstruMacData->Packet_MACProtocol.
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_FreePacket(NetSim_PACKET* pstruPacket)
{
	free_ieee802_11_mac_header(pstruPacket);
	free_ieee802_11_phy_header(pstruPacket);
}

/**
This function is called by NetworkStack.dll, to copy the WLAN protocol
pstruMacData->Packet_MACProtocol from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	copy_ieee802_11_mac_header(pstruDestPacket,pstruSrcPacket);
	//copy_ieee802_11_phy_header(pstruPacket);
}

/**
This function call WLAN Metrics function in lib file to write the Metrics in Metrics.txt	
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_Metrics(char* szMetrics)
{
	fn_NetSim_IEEE802_11_Metrics_F(szMetrics);	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
	print_minstrel_table(szMetrics);
	return 0;
}

/**
This function is to configure the WLAN protocol packet trace parameter. 
This function return a string which has the parameters separated by comma.
*/
_declspec(dllexport) char* fn_NetSim_IEEE802_11_ConfigPacketTrace(const void* xmlNetSimNode)
{
	return "";
}

/**
This function is called while writing the Packet trace for WLAN protocol. 
This function is called for every packet while writing the packet trace.
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 1;
}

/**
This function is called by NetworkStack.dll, once simulation end to free the 
allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_Finish()
{
	fn_netsim_matlab_Finish();
	return fn_NetSim_IEEE802_11_Finish_F();
}

void fn_NetSim_IEEE802_11_Timer()
{
	switch(pstruEventDetails->nSubEventType)
	{
	case UPDATE_DEVICE_STATUS:
		fn_NetSim_IEEE802_11_UpdatePowerduetoTxEnd(
			pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId);
		break;
	case ACK_TIMEOUT:
		fn_NetSim_IEEE802_11_CSMA_AckTimeOut();
		break;
	case CTS_TIMEOUT:
		fn_NetSim_IEEE802_11_RTS_CTS_CTSTimeOut();
		break;
	default:
		fnNetSimError("Unknown Timer event-%d subevent for IEEE802.11 protocol in %s\n",
			pstruEventDetails->nSubEventType,
			__FUNCTION__);
	}
}
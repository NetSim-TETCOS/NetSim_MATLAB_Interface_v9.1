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

int fn_NetSim_802_11_MeshBSS_Init(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	int nConnectedDeviceCount;
	NETSIM_ID nDevId, nIntefId;
	PIEEE802_11_MAC_VAR mac=IEEE802_11_MAC(nDeviceId,nInterfaceId); 
	PIEEE802_11_PHY_VAR phy=IEEE802_11_PHY(nDeviceId,nInterfaceId);

	NetSim_LINKS* pstruLink;
	pstruLink = DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks;
	nDevId=pstruLink->puniDevList.pstruMP2MP.anDevIds[0];
	nIntefId=pstruLink->puniDevList.pstruMP2MP.anDevInterfaceIds[0];

	nConnectedDeviceCount = pstruLink->puniDevList.pstruMP2MP.nConnectedDeviceCount;
	// Set the Channel characteristic Model
	if(phy->dFrequency != DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks->puniMedProp.pstruWirelessLink.dFrequency)
	{
		DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks->puniMedProp.pstruWirelessLink.dFrequency = phy->dFrequency;
	}
	phy->dPathLossExponent = DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks->puniMedProp.pstruWirelessLink.dPathLossExponent;
	phy->dFadingFigure = DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks->puniMedProp.pstruWirelessLink.dFadingFigure;
	phy->dStandardDeviation = DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks->puniMedProp.pstruWirelessLink.dStandardDeviation;

	if(phy->dPathLossExponent ==0 && phy->dFadingFigure==0 && phy->dStandardDeviation==0)
		phy->ChannelCharecteristics = NO_PATHLOSS;	
	else if(phy->dPathLossExponent!= 0 && phy->dFadingFigure==0)
		phy->ChannelCharecteristics = LINE_OF_SIGHT;	
	else if(phy->dPathLossExponent!= 0 && phy->dFadingFigure!=0 && phy->dStandardDeviation==0)
		phy->ChannelCharecteristics = FADING;	
	else
		phy->ChannelCharecteristics = FADING_AND_SHADOWING;

	mac->BSSType=MESH;
	mac->BSSId= DEVICE_HWADDRESS(nDevId,nIntefId);
	mac->nBSSId = nDevId;
	mac->nAPInterfaceId= nIntefId;
	mac->devIdsinBSS=(NETSIM_ID*)calloc(nConnectedDeviceCount,sizeof* mac->devIdsinBSS);
	mac->devIfinBSS=(NETSIM_ID*)calloc(nConnectedDeviceCount,sizeof* mac->devIfinBSS);
	mac->devIdsinBSS[0]=nDevId;
	mac->devIfinBSS[0]=nIntefId;
	memcpy(mac->devIdsinBSS,pstruLink->puniDevList.pstruMP2MP.anDevIds,
		nConnectedDeviceCount*sizeof(NETSIM_ID));
	memcpy(mac->devIfinBSS,pstruLink->puniDevList.pstruMP2MP.anDevInterfaceIds,
		nConnectedDeviceCount*sizeof(NETSIM_ID));
	return 0;
}

void fn_NetSim_802_11_MeshBSS_UpdateReceiver(NetSim_PACKET* packet)
{
	return; //These information is updated by routing protocol
}

bool isPacketforsameMeshBSS(PIEEE802_11_MAC_VAR mac,NetSim_PACKET* packet)
{
	return true; //These information is taken care by routing protocol
}

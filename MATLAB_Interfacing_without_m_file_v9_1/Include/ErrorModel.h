/************************************************************************************
* Copyright (C) 2015
* TETCOS, Bangalore. India															*

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
* Author:	Shashi Kant Suman														*
* ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_ERROR_MODEL_H_
#define _NETSIM_ERROR_MODEL_H_
//For MSVC commpiler. For GCC link via Linker command 
#pragma comment(lib,"ErrorModel.lib")

typedef enum enum_Modulation
{
	Modulation_DBPSK,
	Modulation_DQPSK,
	Modulation_CCK5_5,
	Modulation_CCK11,
	Modulation_BPSK,
	Modulation_QPSK,
	Modulation_16_QAM,
	Modulation_64_QAM,
	Modulation_128_QAM,
	Modulation_256_QAM,
}MODULATION;


typedef struct stru_ber
{
	double dSNR;
	double dBER;
}BER;

double calculate_ber(double snr,BER ber_table[],size_t table_len);
double calculate_snr(double dReceivedPower/*in dBm*/,double bandwidth/*In MHz*/);
double calculate_BER(MODULATION modulation,double dReceivedPower/*in dBm*/,double dBandwidth/*in MHz*/);

#endif //_NETSIM_ERROR_MODEL_H_
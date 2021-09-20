#pragma once
#pragma warning(disable: 4996)
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#define MISSINGUNIT 0
#define verbose 1

class c889Communicator
{
public:
	enum acquisitionType
	{
		DC_resistance,
		DC_voltage,
		DC_current,
		Impedence_radians,
		Impedence_degrees,
		AC_voltage,
		AC_current,
		Cap_QF_par,
		Cap_QF_ser,
		Cap_DF_par,
		Cap_DF,ser,
		Cap_R_par,
		Cap_R_ser,
		Ind_QF_par,
		Ind_QF_ser,
		Ind_DF_par,
		Ind_DF_ser,
		Ind_R_par,
		Ind_R_ser,
		R_Reactance_par,
		R_Reactance_ser
	};

	enum frequency
	{
		SAMPLE_100,
		SAMPLE_120,
		SAMPLE_1K,
		SAMPLE_10K,
		SAMPLE_100K,
		SAMPLE_200K
	};

	struct measurements
	{
		int values[2];
		bool twoValues;
	};

	struct configuration
	{
		acquisitionType type;
		frequency freq;
	};

	struct commQueryCommand
	{
		char* query;
		unsigned int byteCnt;
	};

	c889Communicator();
	~c889Communicator();


	bool InitComm(int freq, bool, HANDLE pipe);
	void CloseComm();
	bool ConfigDevice();
	
	bool Read();

private:
	bool AssignMeasurementType(int);
	bool AssignSupplyFrequency(int);
	bool AssignRange(int, int);
	bool AssignSupplyVoltage(int);
	bool ConfigComm(char*, unsigned int);
	void LogOp(std::string);
	void WriteParam(char*, int);

	HANDLE comport;
	std::ofstream opLogFile;
//	FILE *logFile;
	bool commInit, devInit;
	short printCnt;
	int frequency;
	bool readParamFile;
	std::fstream paramFile;
	HANDLE commPipe;


	commQueryCommand* qCommand;
};


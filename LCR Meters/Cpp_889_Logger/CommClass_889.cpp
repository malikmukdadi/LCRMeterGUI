#include "stdafx.h"
#include "CommClass_889.h"
#include <string>
#include <sstream>

c889Communicator::c889Communicator()
:comport(NULL)
,commInit(false)
,devInit(false)
,qCommand(NULL)
//,logFile(NULL)
,printCnt(0)
{

}

c889Communicator::~c889Communicator()
{
	opLogFile.close();
	if(paramFile.is_open()) paramFile.close();
}

void c889Communicator::WriteParam(char* data, int length)
{
	for(int i=0; i<length; i++)
		paramFile.put(data[i]);
	paramFile.put('\n');
}

bool c889Communicator::InitComm(int freq, bool readFile, HANDLE pipe)
{
	commPipe = pipe;
	SYSTEMTIME st;
	GetSystemTime(&st);
	std::ostringstream sstream;
	sstream << "system-logs\\889Log_" 
			<< std::setw(4) << std::setfill('0') << st.wYear 
			<< std::setw(2) << std::setfill('0') << st.wMonth 
			<< std::setw(2) << std::setfill('0') << st.wDay << "T" 
			<< std::setw(2) << std::setfill('0') << st.wHour 
			<< std::setw(2) << std::setfill('0') << st.wMinute 
			<< std::setw(2) << std::setfill('0') << st.wSecond << "." 
			<< std::setw(3) << std::setfill('0') << st.wMilliseconds << ".txt";
	opLogFile.open(sstream.str().c_str());

	frequency = freq;
	DWORD bytes_written = 0; // Number of bytes written to the port
	DCB comSettings; // Contains various port settings
	COMMTIMEOUTS CommTimeouts;
	char portName[9];
	//wchar_t portName_uni[9];
	char inputChar;
	char buffer[50];
	readParamFile = readFile;
	if(readParamFile)
	{
		paramFile.open("paramfile.txt", std::fstream::in);
		paramFile.getline(buffer, 50);
		paramFile.getline(buffer, 50);
		for(int i=0; i<8; i++)
			portName[i] = buffer[i];
		portName[8] = 0;
		//printf("Port Name read in: %s\n", buffer);
	}
	else
	{
		paramFile.open("paramfile.txt", std::fstream::out|std::fstream::app);
		portName[0] = '\\';
		portName[1] = '\\';
		portName[2] = '.';
		portName[3] = '\\';
		portName[4] = 'C';
		portName[5] = 'O';
		portName[6] = 'M';
		portName[8] = 0;
		printf("Specify Comm Port Number (1-9):  ");
		std::cin.get(inputChar);
		fflush(stdin);

		if((inputChar < 49) || (inputChar > 57))
		{
			LogOp("ERROR: COMM Init: Invalid/out of range");
			return false;
		}
		portName[7] = inputChar;

		char temp;
		for(int i=0; i<8; i++)
		{
			wctomb(&temp, portName[i]);
			paramFile.put(temp);
		}
		paramFile.put('\n');
	}
	
	comport = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (comport == INVALID_HANDLE_VALUE)
	{
		LogOp(portName);
		//LogOp("ERROR: COMM Init: port open failure because" + GetLastError());
		return false;
	}
	
	CommTimeouts.ReadIntervalTimeout = 0; 
	CommTimeouts.ReadTotalTimeoutMultiplier = 0; 
	CommTimeouts.ReadTotalTimeoutConstant = 500;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 500;
	if (SetCommTimeouts(comport,&CommTimeouts) == 0)
	{
		LogOp("ERROR: COMM Init: port timeout settings failure because" + GetLastError());
		return false;
	}
	
	GetCommState(comport, &comSettings);
	comSettings.BaudRate = 9600;
	comSettings.StopBits = ONESTOPBIT;
	comSettings.EofChar = 0x0A;
	comSettings.ByteSize = 8;
	comSettings.Parity = NOPARITY;
	comSettings.fParity = FALSE;
	if (SetCommState(comport, &comSettings) == 0)
	{
		LogOp("ERROR: COMM Init: port comm setting failure because" + GetLastError());
		return false;
	}
	commInit = true;
	LogOp("INFO: Com Port Ready ...");
	bool ret = ConfigDevice();
	paramFile.close();
	return ret;
}

void c889Communicator::LogOp(std::string data)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
#if verbose
	printf("%s\n", data.c_str());
#endif
	std::ostringstream sstream;
	sstream << std::setw(2) << std::setfill('0') << st.wHour 
			<< std::setw(2) << std::setfill('0') << st.wMinute 
			<< std::setw(2) << std::setfill('0') << st.wSecond << "." 
			<< std::setw(3) << std::setfill('0') << st.wMilliseconds;
	opLogFile << sstream.str() << ": " << data << std::endl;
}

void c889Communicator::CloseComm()
{
	CloseHandle(comport);
	commInit = false;
	devInit = false;
//	if(logFile != NULL) fclose(logFile);
	if(qCommand != NULL)
	{ 
		free(qCommand->query);
		delete qCommand;
	}
}

bool c889Communicator::ConfigComm(char* bufOut, unsigned int byteCnt)
{
	HANDLE tempHandle = (HANDLE)GetStdHandle(STD_OUTPUT_HANDLE);
	char bufIn[500];
	DWORD bytesWritten, bytesRead;
	int ret;
	LogOp("INFO: Preparing to send config: ");
	WriteFile(tempHandle, bufOut, byteCnt, &bytesWritten, NULL);
	//send mode to unit
	ret = WriteFile(comport, bufOut, byteCnt, &bytesWritten, NULL);
	fflush(stdout);
	if((ret == 0) || (bytesWritten != byteCnt))
	{
		LogOp("ERROR: Failed to Tx buffer" + GetLastError());
		return false;
	}
	//pick up unit's acknowledgement
	//for(int i=0; i<50; i++)
	//	bufIn[i]=char(0);
	bytesRead=0;
	ret = ReadFile(comport, bufIn, 49, &bytesRead, NULL);
	if(ret == 0)
	{
		LogOp("ERROR: config: missing ack");
		return false;
	}

#if MISSINGUNIT == 0
	if(bytesRead == 0)
	{
		LogOp("ERROR: Unit unresponsive, exiting\n");
		return false;
	}
#endif
	
	if(bytesRead > 1)
	{
		if((bufIn[0] == 'O') && (bufIn[1] == 'K'))
			LogOp("INFO: Unit Acknowledges: OK, raw:");
	}
	else
	{
		(bytesRead > 48) ? bufIn[49] = char(0) : bufIn[int(bytesRead)] = char(0);
		LogOp("INFO: Unknown Acknowledgement: ");
	}

	return true;
}

bool c889Communicator::AssignMeasurementType(int choice)
{
	char bufOut[500];
	unsigned int byteCnt=0;

	if(readParamFile)
	{
		paramFile.getline(bufOut, 500);
		while(bufOut[byteCnt++] != 0);
		bufOut[byteCnt-1] = '\n';
		byteCnt++;
		//for(int i=0; i<6; i++)
		  //      printf("%d ", bufOut[i]);
        //printf("\n");    
		//printf("bufOut: %s, byteCnt: %d\n",bufOut, byteCnt);
	}
	else
	{
		switch(choice)
		{
			case 1:  byteCnt = 5; strcpy(bufOut,   "DCR\n");  break;
			case 2:  byteCnt = 6; strcpy(bufOut,   "CpRp\n"); break;
			case 3:  byteCnt = 5; strcpy(bufOut,   "CpQ\n");  break;
			case 4:  byteCnt = 5; strcpy(bufOut,   "CpD\n");  break;
			case 5:  byteCnt = 6; strcpy(bufOut,   "CsRs\n"); break;
			case 6:  byteCnt = 5; strcpy(bufOut,   "CsQ\n");  break;
			case 7:  byteCnt = 5; strcpy(bufOut,   "CsD\n");  break;
			case 8:  byteCnt = 6; strcpy(bufOut,   "LpRp\n"); break;
			case 9:  byteCnt = 5; strcpy(bufOut,   "LpQ\n");  break;
			case 10: byteCnt = 5; strcpy(bufOut,   "LpD\n");  break;
			case 11: byteCnt = 6; strcpy(bufOut,   "LsRs\n"); break;
			case 12: byteCnt = 5; strcpy(bufOut,   "LsQ\n");  break;
			case 13: byteCnt = 5; strcpy(bufOut,   "LsD\n");  break;
			case 14: byteCnt = 6; strcpy(bufOut,   "RsXs\n"); break;
			case 15: byteCnt = 6; strcpy(bufOut,   "RpXp\n"); break;
			case 16: byteCnt = 5; strcpy(bufOut,   "ZTD\n");  break;
			case 17: byteCnt = 5; strcpy(bufOut,   "ZTR\n");  break;
			case 18: byteCnt = 5; strcpy(bufOut,   "DCV\n");  break;
			case 19: byteCnt = 5; strcpy(bufOut,   "ACV\n");  break;
			case 20: byteCnt = 5; strcpy(bufOut,   "DCA\n");  break;
			case 21: byteCnt = 5; strcpy(bufOut,   "ACA\n");  break;
			default: LogOp("ERROR: Invalid measurement type\n"); return false;
		}
		WriteParam(bufOut, byteCnt-2);
	}
	
	//this will be used later in the query function
	qCommand = new commQueryCommand;
	qCommand->byteCnt = byteCnt+1;
	qCommand->query = (char*)malloc(qCommand->byteCnt);
	if(qCommand->query == NULL) return false;

	memcpy(qCommand->query, bufOut, byteCnt-2); //leave out newline and null char
	strcpy(qCommand->query + (byteCnt - 2), "?\n"); //add a question mark to it
	LogOp("INFO: AssignMeasurementType called");
	
	return ConfigComm(bufOut, (byteCnt-1));
}

bool c889Communicator::AssignSupplyFrequency(int choice)
{
	char bufOut[500];
	unsigned int byteCnt=0;
	if(readParamFile)
	{
		paramFile.getline(bufOut, 500);
		while(bufOut[byteCnt++] != 0);
		bufOut[byteCnt-1] = '\n';
		byteCnt++;
	}
	else
	{
		switch(choice)
		{
			case 1:  byteCnt = 12; strcpy(bufOut,   "FREQ 100Hz\n");  break;
			case 2:  byteCnt = 12; strcpy(bufOut,   "FREQ 120Hz\n"); break;
			case 3:  byteCnt = 11; strcpy(bufOut,   "FREQ 1KHz\n");  break;
			case 4:  byteCnt = 12; strcpy(bufOut,   "FREQ 10KHz\n");  break;
			case 5:  byteCnt = 13; strcpy(bufOut,   "FREQ 100KHz\n"); break;
			case 6:  byteCnt = 13; strcpy(bufOut,   "FREQ 200KHz\n");  break;
			default: LogOp("ERROR: Invalid frequency"); return false;
		}
		WriteParam(bufOut, byteCnt-2);
	}
	LogOp("INFO: AssignSupplyFrequency called");
	return ConfigComm(bufOut, (byteCnt-1));
}

bool c889Communicator::AssignSupplyVoltage(int choice)
{
	char bufOut[500];
	unsigned int byteCnt=0;
	if(readParamFile)
	{
		paramFile.getline(bufOut, 500);
		while(bufOut[byteCnt++] != 0);
		bufOut[byteCnt-1] = '\n';
		byteCnt++;
		//for(int i=0; i<10; i++)
		//        printf("%d ", bufOut[i]);
        //printf("\n");    
		//printf("bufOut: %s, byteCnt: %d\n",bufOut, byteCnt);
		//Sleep(2500);
	}
	else
	{
		switch(choice)
		{
			case 1:  byteCnt = 10; strcpy(bufOut,   "LEV 1VDC\n");  break;
			case 2:  byteCnt = 11; strcpy(bufOut,   "LEV 1Vrms\n"); break;
			case 3:  byteCnt = 14; strcpy(bufOut,   "LEV 250mVrms\n");  break;
			case 4:  byteCnt = 13; strcpy(bufOut,   "LEV 50mVrms\n");  break;
			default: LogOp("ERROR: Invalid supply"); return false;
		}
		WriteParam(bufOut, byteCnt-2);
	}
	LogOp("INFO: AssignSupplyVoltage called");
	return ConfigComm(bufOut, (byteCnt-1));
}

bool c889Communicator::ConfigDevice()
{
	char tArray[] = "ASC ON\n";
	if(!ConfigComm(tArray,7)) return false;
	system("cls");
	int choice;
	if(!readParamFile)
	{
    	printf("Specify measurement type...\n");
    	printf("1\tDC Resistance\n");
    	printf("2\tParallel RC\n");
    	printf("3\tParallel capacitive plus QF\n");
    	printf("4\tParallel capacitive plus DF\n");
    	printf("5\tSeries RC\n");
    	printf("6\tSeries capacitive plus QF\n");
    	printf("7\tSeries capacitive plus DF\n");
    	printf("8\tParallel LC\n");
    	printf("9\tParallel inductive plus QF\n");
    	printf("10\tParallel inductive plus DF\n");
    	printf("11\tSeries LC\n");
    	printf("12\tSeries inductive plus QF\n");
    	printf("13\tSeries inductive plus DF\n");
    	printf("14\tSeries R,X\n");
    	printf("15\tParallel R,X\n");
    	printf("16\tImpedence (degrees)\n");
    	printf("17\tImpedence (Radians)\n");
    	printf("18\tDC voltage\n");
    	printf("19\tAC voltage\n");
    	printf("20\tDC current\n");
    	printf("21\tAC current\n");
    	printf("Choice: ");
    	std::cin >> choice;
    	if((choice < 1) || (choice > 21))
    	{
    		LogOp("ERROR: Device Init: out of range");
    		return false;
    	}
    }
	if(!AssignMeasurementType(choice)) return false;

	system("cls");
	if(!readParamFile)
	{
    	printf("Select Supply Level ...\n");
    	printf("1\t 1 VDC\n");
    	printf("2\t 1 Vrms\n");
    	printf("3\t 250 mVrms\n");
    	printf("4\t 50 mVrms\n");
    	printf("Choice: ");
    	std::cin >> choice;
    	if((choice < 1) || (choice > 4))
    	{
    		LogOp("ERROR: Device Init: out of range");
    		return false;
    	}
    }
	if(!AssignSupplyVoltage(choice)) return false;

	system("cls");
	if(!readParamFile)
	{
    	printf("Select Supply Frequency ...\n");
    	printf("1\t 100 Hz\n");
    	printf("2\t 120 Hz\n");
    	printf("3\t   1 KHz\n");
    	printf("4\t  10 KHz\n");
    	printf("5\t 100 KHz\n");
    	printf("6\t 200 KHz\n");
    	printf("Choice: ");
    	std::cin >> choice;
    	if((choice < 1) || (choice > 6))
    	{
    		LogOp("Device Init: out of range");
    		return false;
    	}
    } 
	if(!AssignSupplyFrequency(choice)) return false;

	system("cls");
	int index=-1;
	if(!readParamFile)
	{
    	printf("Select Range:\n");
    	printf("***Not Shown: hit 0 for any mode to leave auto ranging active\n");
    	switch(qCommand->query[0])
    	{
    		case 'C':
    			printf("1\t pF\n");
    			printf("2\t nF\n");
    			printf("3\t uF\n");
    			printf("4\t mF\n");
    			printf("5\t  F\n");
    			index = 0; 
    			break;
    		case 'L':
    			printf("1\t nH\n");
    			printf("2\t uH\n");
    			printf("3\t mH\n");
    			printf("4\t  H\n");
    			printf("5\t KH\n");
    			index = 1; 
    			break;
    		case 'R':
    		case 'Z':
    			printf("1\t mOhm\n");
    			printf("2\t  Ohm\n");
    			printf("3\t KOhm\n");
    			printf("4\t MOhm\n");
    			index = 2;
    			break;
    		case 'D':
    		case 'A':
    			if(qCommand->query[2] == 'A')
    			{
    				index = 3;
    				printf("1\t mA\n");
    				printf("2\t  A\n");
    			}
    			else if (qCommand->query[2] == 'V')
    			{
    				index = 4;
    				printf("1\t mV\n");
    				printf("2\t  V\n");
    			}
    			else
    			{
    				printf("1\t mOhm\n");
    				printf("2\t  Ohm\n");
    				printf("3\t KOhm\n");
    				printf("4\t MOhm\n");
    				index = 2;
    			}break;
    		default: LogOp("ERROR: Device Init: Invalid Option\n");
    	}
    	std::cin >> choice;
    	int choiceMax=0;
    	if(choice == 0)
    		goto SKIPRANGE;
    	switch(index)
    	{
    		case 0:
    		case 1:
    			choiceMax=5; break;
    		case 2:
    			choiceMax=4; break;
    		case 3:
    		case 4:
    			choiceMax=2; break;
    		default:
    			choiceMax=0; break;
    	}
    	if((choice < 1) || (choice > choiceMax))
    	{
    		LogOp("ERROR: Device Init: out of range");
    		return false;
    	}
    }
	if(!AssignRange(choice, index)) return false;
SKIPRANGE:


	LogOp("INFO: Device Online...");
	devInit=true;


	
/*	if((logFile = fopen(sstream.str().c_str(), "w"))==NULL)
	{
		LogOp("ERROR: Could not open log file\n");
		CloseComm();
		return false;
	}
*/
	return true;
}

bool c889Communicator::AssignRange(int choice, int index)
{
	char bufOut[1000000];
	unsigned int byteCnt=0;
	if(readParamFile)
	{
		paramFile.getline(bufOut, 1000000);
		while(bufOut[byteCnt++] != 0);
		bufOut[byteCnt-1] = '\n';
		byteCnt++;
	}
	else
	{
		switch(index)
		{
			case 0:
				switch (choice)
				{
					case 1: byteCnt = 9; strcpy(bufOut,   "RANG pF\n"); break;
					case 2: byteCnt = 9; strcpy(bufOut,   "RANG nF\n"); break;
					case 3: byteCnt = 9; strcpy(bufOut,   "RANG uF\n"); break;
					case 4: byteCnt = 9; strcpy(bufOut,   "RANG mF\n"); break;
					case 5: byteCnt = 9; strcpy(bufOut,   "RANG F\n"); break;
					default: LogOp("ERROR: Invalid Range"); break;
				} break;
			case 1:
				switch (choice)
				{
					case 1: byteCnt = 9; strcpy(bufOut,   "RANG nH\n"); break;
					case 2: byteCnt = 9; strcpy(bufOut,   "RANG uH\n"); break;
					case 3: byteCnt = 9; strcpy(bufOut,   "RANG mH\n"); break;
					case 4: byteCnt = 8; strcpy(bufOut,   "RANG H\n"); break;
					case 5: byteCnt = 9; strcpy(bufOut,   "RANG KH\n"); break;
					default: LogOp("ERROR: Invalid Range"); break;
				} break;
			case 2:
				switch (choice)
				{
					case 1: byteCnt = 11; strcpy(bufOut,   "RANG mOhm\n"); break;
					case 2: byteCnt = 10; strcpy(bufOut,   "RANG Ohm\n"); break;
					case 3: byteCnt = 11; strcpy(bufOut,   "RANG KOhm\n"); break;
					case 4: byteCnt = 11; strcpy(bufOut,   "RANG MOhm\n"); break;
					default: LogOp("ERROR: Invalid Range"); break;
				} break;
			case 3:
				switch (choice)
				{
					case 1: byteCnt = 9; strcpy(bufOut,   "RANG mA\n"); break;
					case 2: byteCnt = 8; strcpy(bufOut,   "RANG A\n"); break;
					default: LogOp("ERROR: Invalid Range"); break;
				} break;
			case 4:

				switch (choice)
				{
					case 1: byteCnt = 9; strcpy(bufOut,   "RANG mV\n"); break;
					case 2: byteCnt = 8; strcpy(bufOut,   "RANG V\n"); break;
					default: LogOp("ERROR: Invalid Range"); break;
				} break;
			default: LogOp("ERROR: Invalid range"); return false;
		}
		WriteParam(bufOut, byteCnt-2);
	}
	LogOp("INFO: AssignRange called");
	return ConfigComm(bufOut, (byteCnt-1));
}

bool c889Communicator::Read()
{
	measurements ret;
	ret.values[0] = 0;
	ret.values[1] = 0;
	ret.twoValues = false;
	if(!devInit) return false;

	char strBuf[80]="                                                                               ";


	std::time_t now;
	int seconds;
	time(&now);
	seconds = difftime(now, 1464899931);


	SYSTEMTIME time;
	std::ostringstream timeStream, valStream;
	timeStream.str("");
	valStream.str("");
	char INBUFFER[101];
	DWORD bytes_read = 0; // Number of bytes read from port
	DWORD bytes_written = 0; // Number of bytes written to the port
	int readCnt = 100;
	// Handle COM port
	int bStatus;
	for(int i=0; i<readCnt; i++)
		INBUFFER[i] = char(0);
	//printf("Writing: %d, %*s\n",qCommand->byteCnt-1,qCommand->byteCnt-1,qCommand->query);
	PurgeComm(comport, PURGE_RXABORT|PURGE_RXCLEAR);
	LogOp("");
	//bStatus = WriteFile(comport, qCommand->query, qCommand->byteCnt-1, &bytes_written, NULL);
	bStatus = WriteFile(comport, "READ?\n", 6, &bytes_written, NULL);
	if (bStatus == 0)
	{
		LogOp("ERROR: Write: Writing failed with code " + GetLastError());
	}
	//Sleep(10);
	int i=0, byteCounter=0;
	//LogOp("INFO: MR");
	while(i < readCnt)
	{
		bStatus = ReadFile(comport, (INBUFFER+i), 1, &bytes_read, NULL);
		//(bytes_read == 0) ? printf("oops ") : printf("%c ",INBUFFER[i]);
		if(bytes_read == 0) continue;
		//if(INBUFFER[i] == '\r') break;
		if(INBUFFER[i] == '\n') break;
		if(bStatus == 0) break;
		byteCounter++;
		i++;
		
	}
	//printf("\n");
	GetLocalTime(&time);
	if (bStatus == 0)
	{
		LogOp("ERROR: Read: Reading failed\n");
	}
	Sleep(10);
	timeStream <<  seconds;


	if(bytes_read == 0)
		valStream << "error";
	else
		for(int j=0; j<byteCounter; j++)
			valStream << INBUFFER[j];

	sprintf(strBuf,"%-20s",timeStream.str().c_str());
	sprintf(strBuf+20,"%20s",valStream.str().c_str());
	printf("%s\r",strBuf);

	valStream << "\r\n";

	std::ostringstream dataStream;
	dataStream << timeStream.str() << " " << valStream.str();
	DWORD bytesWritten;
	//LogOp("INFO: Pipe Send ");
	return WriteFile(commPipe, dataStream.str().c_str(), 43, &bytesWritten, NULL);
}

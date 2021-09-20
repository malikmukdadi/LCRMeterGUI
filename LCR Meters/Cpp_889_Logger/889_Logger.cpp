// 889_Logger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CommClass_889.h"

std::ofstream logFile;

void log(std::string data)
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
	logFile << sstream.str() << ": " << data << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{	
	SYSTEMTIME st;
	GetSystemTime(&st);
	std::string filename;
	std::ostringstream sstream;
	sstream << "system-logs\\SlaveLog_" 
			<< std::setw(4) << std::setfill('0') << st.wYear 
			<< std::setw(2) << std::setfill('0') << st.wMonth 
			<< std::setw(2) << std::setfill('0') << st.wDay << "T" 
			<< std::setw(2) << std::setfill('0') << st.wHour 
			<< std::setw(2) << std::setfill('0') << st.wMinute 
			<< std::setw(2) << std::setfill('0') << st.wSecond << "." 
			<< std::setw(3) << std::setfill('0') << st.wMilliseconds << ".txt";
	printf("filename: %s\n", sstream.str().c_str());
	logFile.open(sstream.str().c_str());
	HANDLE commPipe;
	log("INFO: slave online");
	log("INFO: attempting to connect to comm pipe");
	char buf[]="hello world\n";
	int ret, gle;
	if(WaitNamedPipeA(("\\.\pipe\\889_comm_pipe"), 1000)!=0) // Code stops working here
	{
		DWORD bWritten;
		log("INFO: Pipe available, connecting ... ");
		commPipe = CreateFileA(("\\\\.\\pipe\\889_comm_pipe"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(commPipe != INVALID_HANDLE_VALUE)
		{
			log("INFO: connected to server");
			
			ret = WriteFile(commPipe, buf, sizeof(buf), &bWritten, NULL);
			if(ret == 0)
			{
				gle = GetLastError();
				if(gle == ERROR_NO_DATA)
				{
					log("ERROR: ERROR_NO_DATA received, assuming server attempting shutdown, closing client");
					//break;
				}
				else
				{
					std::string error;
					error = "ERROR: Unhandled pipe error, code: " + gle;
					log(error);
				}
			}
			Sleep(500);
		}
		else
		{
			std::string error;
			error = "ERROR: invalid handle, code %d" + GetLastError();
			log(error);
		}
	}
	else
		log("ERROR: server not available");	

	double frequency = 10, period;
	c889Communicator temp;
	FILETIME ft;
	unsigned long long sysTime, previousTime;
	int sleepTime;
	std::fstream datFile;
	if(argc>1)
	{
		datFile.open("paramfile.txt",std::fstream::in);
		datFile >> frequency;
	}
	else
	{
		datFile.open("paramfile.txt",std::fstream::out);
		printf("Select a logging sample frequency (1-50 Hz)\n");
		std::cin>>frequency;
		(frequency < 1) ? frequency = 1 : ((frequency > 50) ? frequency = 50 : frequency = frequency);
		datFile << frequency << std::endl;
	}
	if(!datFile.is_open())
	{
		perror("File failed because:");
	}
	datFile.close();
	fflush(stdin);
	DWORD bWritten;
	if(temp.InitComm((int)frequency, argc>1, commPipe))
	{
		period = 1.0/frequency * 1000.0;
		system("cls");
		std::cout << "System Running, press q or Q to quit\n\n";
		printf("Current Time:","Measured Value(s):");
		printf("----------------------------------------\n");
		while(true)
		{
			GetSystemTimeAsFileTime(&ft); 
			previousTime = ((LONGLONG)ft.dwHighDateTime << 32LL) + ft.dwLowDateTime; 
			
			if(temp.Read() == false)
			                break;

			/*ret = WriteFile(commPipe, buf, sizeof(buf), &bWritten, NULL);
			if(ret == 0)
			{
				gle = GetLastError();
				if(gle == ERROR_NO_DATA)
				{
					log("ERROR: ERROR_NO_DATA received, assuming server attempting shutdown, closing client");
					break;
				}
				else
				{
					std::string error;
					error = "ERROR: Unhandled pipe error, code: " + gle;
					log(error);
				}
			}*/
			GetSystemTimeAsFileTime(&ft); 
			sysTime = ((LONGLONG)ft.dwHighDateTime << 32LL) + ft.dwLowDateTime;
			sleepTime = (int)(period - ((sysTime - previousTime) / 10000.0));
			if(sleepTime>0)
				Sleep(sleepTime);
		}
	}
	temp.CloseComm();
	log("INFO: System shutdown, press enter to close terminal");
	char tempChar;
	fflush(stdin);
	tempChar = std::getchar();
	
	CloseHandle(commPipe);
	log("INFO: Slave shutting down");
	logFile.close();
	return 0;
}


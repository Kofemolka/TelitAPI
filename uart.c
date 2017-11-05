#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define buffSize  1000

HANDLE hComm;                          // Handle to the Serial port
OVERLAPPED o;


void openComPort()
{
	BOOL  Status;                          // Status of the various operations 


	char  ComPortName[] = "\\\\.\\COM3";  // Name of the Serial port(May Change) to be opened,

	hComm = CreateFile(ComPortName,                  // Name of the Port to be Opened
		GENERIC_READ | GENERIC_WRITE, // Read/Write Access
		0,                            // No Sharing, ports cant be shared
		NULL,                         // No Security
		OPEN_EXISTING,                // Open existing port only
		/*FILE_FLAG_OVERLAPPED*/0,
		NULL);                        // Null for Comm Devices

	/*if (hComm == INVALID_HANDLE_VALUE)
		printf("\n    Error! - Port %s can't be opened\n", ComPortName);
	else
		printf("\n    Port %s Opened\n ", ComPortName);*/

	o.hEvent = CreateEvent(
		NULL,   // default security attributes 
		TRUE,   // manual-reset event 
		FALSE,  // not signaled 
		NULL    // no name
	);

	// Initialize the rest of the OVERLAPPED structure to zero.
	o.Internal = 0;
	o.InternalHigh = 0;
	o.Offset = 0;
	o.OffsetHigh = 0;

	/*------------------------------- Setting the Parameters for the SerialPort ------------------------------*/

	DCB dcbSerialParams = { 0 };                         // Initializing DCB structure
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	Status = GetCommState(hComm, &dcbSerialParams);      //retreives  the current settings

	/*if (Status == FALSE)
		printf("\n    Error! in GetCommState()");
*/
	dcbSerialParams.BaudRate = CBR_115200;      // Setting BaudRate = 115200
	dcbSerialParams.ByteSize = 8;             // Setting ByteSize = 8
	dcbSerialParams.StopBits = ONESTOPBIT;    // Setting StopBits = 1
	dcbSerialParams.Parity = NOPARITY;        // Setting Parity = None 
	dcbSerialParams.fDtrControl = 0;
	dcbSerialParams.fRtsControl = 0;


	Status = SetCommState(hComm, &dcbSerialParams);  //Configuring the port according to settings in DCB 

	SetCommMask(hComm, EV_RXCHAR);

	//if (Status == FALSE)
	//{
	//	printf("\n    Error! in Setting DCB Structure");
	//}
	//else //If Successfull display the contents of the DCB Structure
	//{
	//	printf("\n\n    Setting DCB Structure Successfull\n");
	//	printf("\n       Baudrate = %d", dcbSerialParams.BaudRate);
	//	printf("\n       ByteSize = %d", dcbSerialParams.ByteSize);
	//	printf("\n       StopBits = %d", dcbSerialParams.StopBits);
	//	printf("\n       Parity   = %d", dcbSerialParams.Parity);
	//}

	/*------------------------------------ Setting Timeouts --------------------------------------------------*/

	COMMTIMEOUTS timeouts = { 0 };	
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	SetCommTimeouts(hComm, &timeouts);

	/*	printf("\n\n    Error! in Setting Time Outs");
	else
		printf("\n\n    Setting Serial Port Timeouts Successfull");*/

	/*------------------------------------ Setting Receive Mask ----------------------------------------------*/

	Status = SetCommMask(hComm, EV_RXCHAR); //Configure Windows to Monitor the serial device for Character Reception

	/*if (Status == FALSE)
		printf("\n\n    Error! in Setting CommMask");
	else
		printf("\n\n    Setting CommMask successfull\n");*/
}

void comSend(const char* data, int len)
{
	char   lpBuffer[buffSize];			       // lpBuffer should be  char or byte array, otherwise write wil fail
	DWORD  dNoOFBytestoWrite;              // No of bytes to write into the port
	DWORD  dNoOfBytesWritten = 0;          // No of bytes written to the port

	dNoOFBytestoWrite = min(buffSize, len); // Calculating the no of bytes to write into the port
	strncpy(lpBuffer, data, dNoOFBytestoWrite);

	BOOL Status = WriteFile(hComm,               // Handle to the Serialport
		lpBuffer,            // Data to be written to the port 
		dNoOFBytestoWrite,   // No of bytes to write into the port
		&dNoOfBytesWritten,  // No of bytes written to the port
		NULL);	
}

int comRead(char *buff, int bufSize)
{	
	int bytesRead;
	ReadFile(hComm, buff, bufSize, &bytesRead, 0);

	if (bytesRead > 0 && bytesRead < bufSize)
	{
		buff[bytesRead] = 0;
	}

	return bytesRead;
}
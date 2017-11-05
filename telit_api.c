#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "telit_api.h"
#include "util.h"

extern void comSend(const char* data, int len);
extern int comRead(char *buff, int bufSize);

#define INPUT_BUFF_SIZE 256
#define MAX_RECV_TIMEOUT 5
#define CMD_RETRY_COUNT	3
#define MAX_RECV_BYTES 900

void at_cmd_send(const char* cmd);

static at_err_code_t recv_response_processor(const char* data, int len, void(*handleData)(const char*, int))
{
	const char cOK[] = "OK\r";
	const char cRING[] = "SSLSRING:";
	const char cSSLRECV[] = "#SSLRECV=1,%d";
	const char cSSLRECV_ACK[] = "#SSLRECV:";
	
	if (strncmp(data, cRING, strlen(cRING)) == 0)
	{		
		char cmd[20];
		sprintf(cmd, cSSLRECV, MAX_RECV_BYTES);

		at_cmd_send(cmd);

		return AT_NONE;
	}
	else if (strncmp(data, cSSLRECV_ACK, strlen(cSSLRECV_ACK)) == 0)
	{
		int byte2receive = atoi(data + strlen(cSSLRECV_ACK));
		char* dataStart = strstr(data + strlen(cSSLRECV_ACK), "\n\r");

		if (dataStart != 0 && handleData != 0)
		{
			handleData(dataStart, byte2receive);
		}

		return AT_OK;
	}	

	return AT_NONE;
}

at_err_code_t processResponse(char* buff, void(*cb)(const char*))
{
	const char s[] = "\n";
	const char cOK[] = "OK\r";
	const char cERROR[] = "ERROR\r";

	char *token;
		
	token = strtok(buff, s);

	at_err_code_t res = AT_NONE;
		
	while (token != 0)
	{
		if (strncmp(token, cOK, strlen(token)) == 0)
		{
			return AT_OK;
		}
		else if (strncmp(token, cERROR, strlen(token)) == 0)
		{
			return AT_ERR;
		}
		else if (cb != 0)
		{
			cb(token);
		}

		token = strtok(NULL, s);
	}

	return res;
}

void at_cmd_send(const char* cmd)
{
	char buff[INPUT_BUFF_SIZE];
	sprintf(buff, "AT%s\r\0", cmd);
	comSend(buff, strlen(buff));

	__trace_at_out(buff);
}

at_err_code_t at_cmd(const char* cmd)
{
	return at_req(cmd, 0);
}

at_err_code_t at_req(const char* cmd, void(*cb)(const char*))
{
	at_err_code_t res = AT_NONE;

	int retry = 0;

	while(retry < CMD_RETRY_COUNT && res != AT_OK)
	{
		char buff[INPUT_BUFF_SIZE];

		at_cmd_send(cmd);	

		time_t start = time(NULL);
		while ((time(NULL) - start) <= MAX_RECV_TIMEOUT)
		{
			int resLen = 0;
			resLen = comRead(buff, INPUT_BUFF_SIZE);
			if(resLen > 0)
			{
				__trace_at_in(buff, resLen);

				res = processResponse(buff, cb);
				
				__trace_log("RES=%d",res);

				if (res == AT_OK) //return result
				{
					return res;
				}
				else if(res == AT_ERR) //retry
				{
					break;
				}

				//else - wait longer
			}
		}

		if ((time(NULL) - start) > MAX_RECV_TIMEOUT)
		{
			res = AT_TIMEOUT;
		}

		retry++;
		sleep(3000);
	}	

	return res;
}

at_err_code_t at_cmd_arg(const char* cmd, ...)
{
	va_list arg;

	va_start(arg, cmd);

	char buff[256];
	_vsprintf_l(buff, cmd, NULL, arg);

	va_end(arg);

	return at_cmd(buff);	
}

at_err_code_t at_raw(const char* data, int len)
{
	comSend(data, len);

	__trace_at_out(data);

	return AT_OK;
}

at_err_code_t at_recv(void(*handleData)(const char*,int))
{
	at_err_code_t res = AT_NONE;
	char buff[INPUT_BUFF_SIZE];

	time_t start = time(NULL);
	while ((time(NULL) - start) <= MAX_RECV_TIMEOUT)
	{
		int resLen = 0;
		resLen = comRead(buff, INPUT_BUFF_SIZE);
		if (resLen > 0)
		{
			__trace_at_in(buff, resLen);

			res = recv_response_processor(buff, resLen, handleData);
			
			__trace_log("RES=%d", res);			
		}
	}

	return AT_TIMEOUT;
}
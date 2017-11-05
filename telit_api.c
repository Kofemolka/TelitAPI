#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "telit_api.h"
#include "util.h"

extern void comSend(const char* data, int len);
extern int comRead(char *buff, int bufSize);

#define INPUT_BUFF_SIZE 256
#define MAX_RECV_TIMEOUT 10
#define CMD_RETRY_COUNT	3
#define MAX_RECV_BYTES 900

void at_cmd_send(const char* cmd);

static at_err_code_t recv_response_processor(const char* data, int len, void(*handleData)(const char*, int))
{
	const char cOK[] = "OK\r";
	const char cRING[] = "SSLSRING:";
	const char cSSLRECV[] = "#SSLRECV=1,%d";
	const char cSSLRECV_ACK[] = "#SSLRECV:";
	
	static long byte2receive = 0;
	static short sslRecvFound = 0;

	/*if (strncmp(token, cRING, strlen(cRING)) == 0)
	{
		char tmp[20];
		//strncpy(tmp, token + strlen(cRING), strlen(token) - strlen(cRING));
		strtrim(token + strlen(cRING), tmp);

		if (*tmp != 0 && strlen(tmp) > 2)
		{
			char* vals = tmp + 2; //1,xxxx
			byte2receive = min(atoi(vals), MAX_RECV_BYTES);
			sslRecvFound = 0;
			byte2receive = MAX_RECV_BYTES;

			char cmd[20];
			sprintf(cmd, cSSLRECV, byte2receive);			

			at_cmd_send(cmd);			
		}

		return AT_NONE;
	}
	else if (strncmp(token, cSSLRECV_ACK, strlen(cSSLRECV_ACK)) == 0)
	{
		sslRecvFound = 1;
	}
	else
	{
		if (handleData != 0 && sslRecvFound && byte2receive > 0)
		{
			byte2receive -= strlen(token);
			cb(token);
		}
	}*/

	return AT_NONE;
}

static at_err_code_t cmd_response_parser(char* token, at_err_code_t(*cb)(const char*))
{
	const char cOK[] = "OK\r";
	const char cERROR[] = "ERROR\r";	
	
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

	return AT_NONE;
}

at_err_code_t processResponse(char* buff, at_err_code_t(*processor)(const char*, void(*cb)(const char*)), void(*cb)(const char*))
{
	const char s[2] = "\n";
	char *token;

	/* get the first token */
	token = strtok(buff, s);

	at_err_code_t res = AT_NONE;

	/* walk through other tokens */
	while (token != NULL)
	{
		res = processor(token, cb);

		if (res != AT_NONE)
		{
			return res;
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

				res = processResponse(buff, cmd_response_parser, cb);
				
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

//at_err_code_t process_response(const char* data, int len, void(*handleData)(const char*, int))
//{
//
//}

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
			//res = processResponse(buff, recv_response_processor, cb);

			__trace_log("RES=%d", res);

			if (res != AT_NONE) //return result
			{
				return res;
			}
			
			//else - wait longer
		}
	}

	return AT_TIMEOUT;
}
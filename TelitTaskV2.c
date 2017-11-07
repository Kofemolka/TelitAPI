#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "telit_api.h"
#include "util.h"
#include "azure.h"
#include "common.h"

extern void openComPort();

/////////////////////////////////////////////////////////////////////////////////
static const char cfg_APN[]			= "www.ab.kyivstar.net";
static const char cfg_BLOB[]		= "farmedge.blob.core.windows.net";
static const char cfg_HUB[]			= "farmedge.azure-devices.net";
//static const char cfg_HUB[]			= "pot.slobodyan.org";
static const char cfg_DEVICE_ID[]	= "WeatherStation";
static const char cfg_API_VER[]		= "2016-11-14";
static const char cfg_SAS[]			= "SharedAccessSignature sr=farmedge.azure-devices.net&sig=yxUw4DSR58ZRkzngZNcUfZHE9hDrs%2FB12nIeKXX5it4%3D&se=1539956606&skn=device";

/////////////////////////////////////////////////////////////////////////////////
static const char at_INIT[]			= "E0Q0V1X1&K0";
static const char at_REBOOT[]		= "#REBOOT";
static const char at_CMEE[]			= "+CMEE=2";
static const char at_CGDCONT[]		= "+CGDCONT=1,\"IP\",\"%s\"";
static const char at_SGACT[]		= "#SGACT=1,1";
static const char at_SGACT_OFF[]	= "#SGACT=1,0";
static const char at_SSLEN[]		= "#SSLEN=1,1";
static const char at_SSLEN_OFF[]	= "#SSLEN=1,0";
static const char at_SSLCFG[]		= "#SSLCFG=1,1,300,90,100,1,1";
static const char at_SSLSECCFG[]	= "#SSLSECCFG=1,0,0";
static const char at_SSLD[]			= "#SSLD=1,443,\"%s\",0,1";
static const char at_SSLH[]			= "#SSLH=1";
static const char at_SSLSEND[]		= "#SSLSEND=1";

static const char c_end[] = { (char)0x1A, (char)0x0D, 0 };
static const char c_prompt[] = ">";
static const char c_post_msg[] =
"POST /devices/%s/messages/events?api-version=%s HTTP/1.1\n"\
"Host: %s\n"\
"Authorization: %s\n"\
"Content-Type: application/json\n"\
"Content-Length: %d\n"\
"\n"\
"%s"\
"%s"; 
static const char c_msg_type_status[]		= "[Status]\n";
static const char c_msg_type_telemetry[]	= "[Telemetry]\n";


/////////////////////////////////////////////////////////////////////////////////
#define INPUT_BUFF_SIZE 3000

typedef struct
{
	unsigned int head;
	char data[INPUT_BUFF_SIZE];
} data_buffer_t;

static data_buffer_t _input_buffer;

#define OUTPUT_BUFF_SIZE 1000
static char _output_buffer[OUTPUT_BUFF_SIZE];

static device_config_t devCfg;

/////////////////////////////////////////////////////////////////////////////////
// Callbacks
void parseIP(const char* token)
{
	//"#SGACT: 10.250.64.139"
	static const char cSGACT[] = "#SGACT:";

	if (strncmp(token, cSGACT, strlen(cSGACT)) == 0)
	{
		token += strlen(cSGACT);

		char ip[20];
		strtrim(token, ip);

		__trace_log("[IP]:%s", ip);
	}	
}

void requestDownloadFirmware(const char* token)
{
	if (strncmp(token, c_prompt, strlen(c_prompt)) == 0)
	{
		const char getFirm[] = "GET /firmware/baltimore.cer HTTP/1.1\n"\
			"Host: farmedge.blob.core.windows.net\n"\
			"\n";
		
		at_raw(getFirm, strlen(getFirm));
		at_raw(c_end, strlen(c_end));
	}
}

void onProcessResponse(const char* data, int len)
{
	if ((_input_buffer.head + len) < INPUT_BUFF_SIZE)
	{
		memcpy(_input_buffer.data+ _input_buffer.head, data, len);
		_input_buffer.head += len;
	}
}

void requestGetConfig(const char* token)
{	
	if (strncmp(token, c_prompt, strlen(c_prompt)) == 0)
	{		
		const char cGET_MSG[] = "GET /devices/%s/messages/devicebound?api-version=%s HTTP/1.1\n"\
			"Host: %s\n"\
			"Authorization: %s\n\n";
				
		sprintf(_output_buffer,
			cGET_MSG,
			cfg_DEVICE_ID,
			cfg_API_VER,
			cfg_HUB,
			cfg_SAS);

		at_raw(_output_buffer, strlen(_output_buffer));
		at_raw(c_end, strlen(c_end));
	}
}

void requestDeleteConfig(const char* token)
{	
	const char cDEL_MSG[] =
		"DELETE /devices/%s/messages/devicebound/%s?api-version=%s HTTP/1.1\n"\
		"Host: %s\n"\
		"Authorization: %s\n\n";		
	
	if (strncmp(token, c_prompt, strlen(c_prompt)) == 0)
	{
		sprintf(_output_buffer,
			cDEL_MSG,
			cfg_DEVICE_ID,
			devCfg.etag,
			cfg_API_VER,
			cfg_HUB,
			cfg_SAS);

		at_raw(_output_buffer, strlen(_output_buffer));
		at_raw(c_end, strlen(c_end));
	}
}

void requestSendStatus(const char* token)
{		
	if (strncmp(token, c_prompt, strlen(c_prompt)) == 0)
	{
		const char cTEST_STATUS[] = "WX0000001\n"\
			"Version: 1.1.7\n"\
			"Interface : 601\n"\
			"Model : Telit HE910 - EUD 12.00.225\n"\
			"Strength : 10\n"\
			"Quality : 3\n"\
			"Retries : 37\n"\
			"Maxtries : 17\n"\
			"Update : -4\n"\
			"Upload : 255";
		
		sprintf(_output_buffer,
			c_post_msg,
			cfg_DEVICE_ID,
			cfg_API_VER,
			cfg_HUB,
			cfg_SAS,
			strlen(cTEST_STATUS) + strlen(c_msg_type_status),
			c_msg_type_status,
			cTEST_STATUS);;

		at_raw(_output_buffer, strlen(_output_buffer));
		at_raw(c_end, strlen(c_end));
	}
}

void requestSendTelemetry(const char* token)
{
	if (strncmp(token, c_prompt, strlen(c_prompt)) == 0)
	{
		const char cTEST_TELEMETRY[] =
			"2700147848000                                        15M2017 - 11 - 07 12:180303030303030503092930311014\n"\
			"201711071215                                                    64.1    15.4    00.0    170     003     000\n"\
			"201711071200                                                    66.4    15.6    00.0    169     003     001\n"\
			"201711071145                                                    57.7    17.1    00.0    159     001     000";

		sprintf(_output_buffer,
			c_post_msg,
			cfg_DEVICE_ID,
			cfg_API_VER,
			cfg_HUB,
			cfg_SAS,
			strlen(cTEST_TELEMETRY) + strlen(c_msg_type_telemetry),
			c_msg_type_telemetry,
			cTEST_TELEMETRY);;

		at_raw(_output_buffer, strlen(_output_buffer));
		at_raw(c_end, strlen(c_end));
	}
}

/////////////////////////////////////////////////////////////////////////////////

int initModem()
{	
	at_cmd(at_INIT);	
	
	at_cmd_arg(at_CGDCONT, cfg_APN);
	if (at_req(at_SGACT, parseIP) == AT_OK)
	{
		if (at_cmd(at_SSLEN) == AT_OK)
		{
			at_cmd(at_SSLCFG);
			at_cmd(at_SSLSECCFG);

			return 1;
		}
	}

	return 0;
}

void downloadFirmware()
{
	at_cmd_arg(at_SSLD, cfg_BLOB);
	at_req(at_SSLSEND, requestDownloadFirmware);

	_input_buffer.head = 0;
	at_recv(onProcessResponse);

	at_cmd(at_SSLH);

	printf("\n\r[Response]:\n\r%s\n\r",_input_buffer.data);
}

void acknowledgeConfig()
{
	at_req(at_SSLSEND, requestDeleteConfig);
	at_recv(0);
}

void retrieveConfig()
{	
	device_config_status_t status;
	do
	{
		at_req(at_SSLSEND, requestGetConfig);

		_input_buffer.head = 0;
		at_recv(onProcessResponse);

		status = parseDeviceConfig(_input_buffer.data, _input_buffer.head, &devCfg);
		if (status == DEV_CFG_OK)
		{
			acknowledgeConfig();
		}
	} while (status == DEV_CFG_OK); //loop until there are no config to be pulled	
}

void sendStatus()
{	
	at_req(at_SSLSEND, requestSendStatus);
	at_recv(0);	
}

void sendTelemetry()
{	
	at_req(at_SSLSEND, requestSendTelemetry);
	at_recv(0);	
}
/////////////////////////////////////////////////////////////////////////////////

int main()
{
	openComPort();

	//at_cmd_send(at_REBOOT);
	//sleep(8000);
	//printf("Press any key...\n\r");
	//_getch();

	if (initModem())
	{		
		if (at_cmd_arg(at_SSLD, cfg_HUB) == AT_OK)
		{
			retrieveConfig();
			
			sendTelemetry();
			
			sendStatus();			

			at_cmd(at_SSLH);
		}

		at_cmd(at_SSLEN_OFF);
		at_cmd(at_SGACT_OFF);		
	}

	printf("Press any key to exit\r\n");
	getchar();

    return 0;
}


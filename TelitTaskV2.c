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
static const char cfg_DEVICE_ID[]	= "WeatherStation";
static const char cfg_API_VER[]		= "2016-11-14";
static const char cfg_SAS[]			= "SharedAccessSignature sr=farmedge.azure-devices.net&sig=yxUw4DSR58ZRkzngZNcUfZHE9hDrs%2FB12nIeKXX5it4%3D&se=1539956606&skn=device";

/////////////////////////////////////////////////////////////////////////////////
static const char at_INIT[]			= "E0Q0V1X1&K0";
static const char at_REBOOT[]		= "#REBOOT";
static const char at_CMEE[]			= "+CMEE=2";
static const char at_CGDCONT[]		= "+CGDCONT=1,\"IP\",\"%s\"";
static const char at_SGACT[]		= "#SGACT=1,1";
static const char at_SSLEN[]		= "#SSLEN=1,1";
static const char at_SSLCFG[]		= "#SSLCFG=1,1,300,90,100,1,1";
static const char at_SSLSECCFG[]	= "#SSLSECCFG=1,0,0";
static const char at_SSLD[]			= "#SSLD=1,443,\"%s\",0,1";
static const char at_SSLH[]			= "#SSLH=1";
static const char at_SSLSEND[]		= "#SSLSEND=1";

/////////////////////////////////////////////////////////////////////////////////
#define INPUT_BUFF_SIZE 3000
static char _input_buffer[INPUT_BUFF_SIZE];

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
	//>
	//HTTP request
	//Ctrl+Z
	//OK
	static const char cPrompt[] = ">";
	if (strncmp(token, cPrompt, strlen(cPrompt)) == 0)
	{
		const char getFirm[] = "GET /firmware/baltimore.cer HTTP/1.1\n"\
			"Host: farmedge.blob.core.windows.net\n"\
			"\n";
		const char end[] = { (char)0x1A, (char)0x0D, 0 };

		at_raw(getFirm, strlen(getFirm));
		at_raw(end, strlen(end));
	}
}

void onProcessResponse(const char* data, int len)
{
	static int i = 0;

	if ((i + len) < sizeof(INPUT_BUFF_SIZE))
	{
		memcpy(_input_buffer, data, len);
		i += len;
	}
}

void requestGetConfig(const char* token)
{
	//>
	//HTTP request
	//Ctrl+Z
	//OK
	static const char cPrompt[] = ">";
	if (strncmp(token, cPrompt, strlen(cPrompt)) == 0)
	{
		const char end[] = { (char)0x1A, (char)0x0D, 0 };
		const char cGET_MSG[] = "GET /devices/%s/messages/devicebound?api-version=%s HTTP/1.1\n"\
			"Host: %s\n"\
			"Authorization: %s\n\n";
		
		char buff[200];
		sprintf(buff, cGET_MSG,
			cfg_DEVICE_ID,
			cfg_API_VER,
			cfg_HUB,
			cfg_SAS);

		at_raw(buff, strlen(buff));
		at_raw(end, strlen(end));		
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
	at_recv(onProcessResponse);
	at_cmd(at_SSLH);

	printf("\n\r[Response]:\n\r%s\n\r",_input_buffer);
}

void retrieveConfig()
{
	at_cmd_arg(at_SSLD, cfg_HUB);
		
	device_config_t devCfg;
	
	int res = 0;
	do
	{
		at_req(at_SSLSEND, requestGetConfig);
		at_recv(onProcessResponse);
		int res = parseDeviceConfig(_input_buffer, strlen(_input_buffer), &devCfg);
	} while (res == 1);

	at_cmd(at_SSLH);
}

void sendStatus()
{
	at_cmd_arg(at_SSLD, cfg_HUB);

	at_cmd(at_SSLH);
}

void sendTelemetry()
{
	at_cmd_arg(at_SSLD, cfg_HUB);

	at_cmd(at_SSLH);
}
/////////////////////////////////////////////////////////////////////////////////

int main()
{
	openComPort();

	at_cmd_send(at_REBOOT);
	sleep(5000);
	//printf("Press any key...\n\r");
	//_getch();

	if (initModem())
	{		
		retrieveConfig();		
		sendStatus();
		sendTelemetry();
		downloadFirmware();
	}

	printf("Press any key to exit\r\n");
	getchar();

    return 0;
}


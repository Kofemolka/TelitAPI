#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "telit_api.h"
#include "util.h"

extern void openComPort();

/////////////////////////////////////////////////////////////////////////////////
static const char cfg_APN[]			= "www.ab.kyivstar.net";
static const char cfg_BLOB[]		= "farmedge.blob.core.windows.net";
static const char cfg_HUB[]			= "farmedge.azure-devices.net";

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

static char _firm[3000];
void onDownloadFirmResponse(const char* data)
{
	//static int i = 0;

	strncat(_firm, data, strlen(data));
	//i += strlen(data);

	//__trace_log(":%s", data);
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
		const char end[] = { (char)0x1A, (char)0x0D };

		at_raw("abcdef12345", 11);
		at_raw(end, 2);
		/*at_cmd
		at = $"GET /firmware/baltimore.cer HTTP/1.1\n" +
		"Host: farmedge.blob.core.windows.net\n" +
		"\n" + (char)0x1A + (char)0x0D,*/
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
	at_recv(onDownloadFirmResponse);
	at_cmd(at_SSLH);

	printf("\n\r\[Response]:\n\r%s\n\r",_firm);
}

void retrieveConfig()
{
	at_cmd_arg(at_SSLD, cfg_HUB);
	at_req(at_SSLSEND, requestGetConfig);
	at_cmd(at_SSLH);
}

int main()
{
	openComPort();

	//at_cmd_send(at_REBOOT);
	//sleep(7000);
	//printf("Press any key...\n\r");
	//_getch();

	if (initModem())
	{
		downloadFirmware();
		//retrieveConfig();		
	}

	printf("Press any key to exit\r\n");
	_getch();

    return 0;
}


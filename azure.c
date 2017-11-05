#include <string.h>
#include <stdlib.h>

#include "azure.h"
#include "util.h"
#include "common.h"

int parseDeviceConfig(const char* data, int len, device_config_t* cfg)
{
	if (cfg == 0)
		return -1;

	const char s[2] = "\n";
#define _IOT_HUB_PREF "iothub-app"	
	const char cSAS[] = _IOT_HUB_PREF"SAS:";
	const char cFIRMWARE[] = _IOT_HUB_PREF"Firmware:";
	const char cPOLL_INTERVAL[] = _IOT_HUB_PREF"PollPeriod:";
	const char cLAST_TELEMETRY[] = _IOT_HUB_PREF"LastTelemetry:";
#undef _IOT_HUB_PREF

	char *token;

	token = strtok((char*)(data), s);
	
	while (token != 0)
	{
		if (strncmp(token, cSAS, strlen(cSAS)) == 0)
		{
			strtrim(token + strlen(cSAS), cfg->sas);			
		}
		else if (strncmp(token, cFIRMWARE, strlen(cFIRMWARE)) == 0)
		{
			strtrim(token + strlen(cFIRMWARE), cfg->firmware_url);
		}
		else if (strncmp(token, cPOLL_INTERVAL, strlen(cPOLL_INTERVAL)) == 0)
		{
			cfg->poll_period = atoi(token + strlen(cPOLL_INTERVAL));
			if (cfg->poll_period < 0 || cfg->poll_period > 120)
			{
				cfg->poll_period = -1;
			}			
		}
		else if (strncmp(token, cLAST_TELEMETRY, strlen(cLAST_TELEMETRY)) == 0)
		{
			cfg->last_telemetry = atoi(token + strlen(cLAST_TELEMETRY));			
		}

		token = strtok(0, s);
	}
	return 0;
}
#include <string.h>
#include <stdlib.h>

#include "azure.h"
#include "util.h"
#include "common.h"

device_config_status_t parseDeviceConfig(const char* data, int len, device_config_t* cfg)
{
	if (cfg == 0)
		return DEV_CFG_NONE;
	
	cfg->etag[0] = 0;
	cfg->firmware_url[0] = 0;
	cfg->sas[0] = 0;
	cfg->last_telemetry = -1;
	cfg->poll_period = -1;

	const char s[2] = "\n";
	const char cETag[] = "ETag:";

#define _IOT_HUB_PREF "iothub-app-"	
	const char cSAS[] = _IOT_HUB_PREF"SAS:";
	const char cFIRMWARE[] = _IOT_HUB_PREF"Firmware:";
	const char cPOLL_INTERVAL[] = _IOT_HUB_PREF"PollInterval:";
	const char cLAST_TELEMETRY[] = _IOT_HUB_PREF"LastTelemetry:";
#undef _IOT_HUB_PREF

	char *token;

	device_config_status_t status = DEV_CFG_NONE;

	token = strtok((char*)(data), s);
	
	while (token != 0)
	{
		if (strncmp(token, cETag, strlen(cETag)) == 0)
		{
			//ETag: "3da1f730-589c-467a-9fd9-fcf8b14c7687"
			char* begin = strstr(token, "\"");
			if (begin != 0)
			{
				begin += 1;
				char* end = strstr(begin, "\"");
				if (end != 0)
				{
					unsigned int tagLen = min(sizeof(cfg->etag), end - begin);
					__trace_log("ETag=%.*s", tagLen, begin);
					strncpy(cfg->etag, begin, tagLen);

					status = DEV_CFG_OK;
				}
			}			
		}
		else if (strncmp(token, cSAS, strlen(cSAS)) == 0)
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

	return status;
}
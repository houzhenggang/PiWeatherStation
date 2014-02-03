/*
 * PiWeatherStation
 * configurator.c
 * Copyright (c) 2013  Goce Boshkovski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "configurator.h"

int readConfiguration(char *confFile,configuration *config, char *errorMsg)
{
	config_t *cfg;
	const char *tlocation;
	const char *tdb;
	const char *ti2cBus;

	cfg=malloc(sizeof(config_t));
	config_init(cfg);

	if (!config_read_file(cfg,confFile))
	{
		sprintf(errorMsg,"%s:%d - %s\n",
				config_error_file(cfg),
				config_error_line(cfg),
				config_error_text(cfg));
		config_destroy(cfg);
		free(cfg);
		return (-1);
	}

	config_lookup_string(cfg,"db",&tdb);
	config_lookup_string(cfg,"sensor.i2cBus",&ti2cBus);
	config_lookup_int(cfg,"sensor.sensorAddr",&config->sensorAddress);
	config_lookup_int(cfg,"sensor.sensorMode",&config->sensorMode);
	config_lookup_string(cfg,"measurement.location",&tlocation);
	config_lookup_int(cfg,"measurement.semplingInterval",&config->interval);

	config->db=malloc(sizeof(char)*(strlen(tdb)+1));
	config->location=malloc(sizeof(char)*(strlen(tlocation)+1));
	config->i2cBus=malloc(sizeof(char)*(strlen(ti2cBus)+1));

	strcpy(config->db,tdb);
	strcpy(config->location,tlocation);
	strcpy(config->i2cBus,ti2cBus);

	config_destroy(cfg);
	free(cfg);

	return(0);
}

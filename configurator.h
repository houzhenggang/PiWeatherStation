/*
 * PiWeatherStation
 * configurator.h
 *
 * Copyright (c) 2013  Goce Boshkovski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */


#ifndef CONFIGURATOR_H_
#define CONFIGURATOR_H_
#endif /* CONFIGURATOR_H_ */

#include "libconfig.h"

typedef struct configuration {
	char *db;
	char *location;
	int interval;
	char *i2cBus;
	int sensorAddress;
	int sensorMode;
} configuration;

int readConfiguration(char *,configuration *,char *);

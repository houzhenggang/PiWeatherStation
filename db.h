/*
 * PiWeatherStation
 * db.h
 *
 * Copyright (c) 2013  Goce Boshkovski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */


#ifndef DB_H_
#define DB_H_
#endif /* DB_H_ */

#include "sqlite3.h"

typedef struct databaseObjects {
	sqlite3 *db;
	sqlite3_stmt *insertMeasurement;
	sqlite3_stmt *insertMeasuredValues;
	sqlite3_int64 currentMeasurementID;
} dbobjects;

int openDatabase(const char *,dbobjects *,char *);
int prepareSQLCommands(dbobjects *,char *);
int createNewMeasurement(dbobjects *,const char *,int,char *);
int writeMeasuredValue(dbobjects *,float,float,char *);
void getCurrentTime(char *);
int closeDatabaseConnection(dbobjects *dbo,char *);





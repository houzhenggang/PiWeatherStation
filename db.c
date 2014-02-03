/*
 * PiWeatherStation
 * db.c
 *
 * Copyright (c) 2013  Goce Boshkovski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */


#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

const char enableForeignKeySupportSQL[]="PRAGMA FOREIGN_KEYS=ON";
const char defineNewMeasurementSQL[]="insert into measurements (location,StartDateTime,samplingInterval) values (?,?,?)";
const char writeMeasuredValuesSQL[]="insert into measuredValues (measurementID,madeOn,pressure,temperature) values (?,?,?,?)";

int openDatabase(const char *dbfile,dbobjects *dbo,char *errMsg)
{
	int returnCode;
	char *terrorMessage;

	returnCode = sqlite3_open(dbfile,&(dbo->db));
	if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg,"Can't open database %s (%i): %s\n", dbfile, returnCode, sqlite3_errmsg(dbo->db));
		return (1);
	}

	//Enable ForeignKey Constraint support.
	returnCode=sqlite3_exec(dbo->db,enableForeignKeySupportSQL,NULL,NULL,&terrorMessage);
	if (returnCode!=SQLITE_OK)
	{
		strcpy(errMsg,terrorMessage);
		sqlite3_free(terrorMessage);
		return (1);
	}
	sqlite3_free(terrorMessage);
	return (0);
}

int prepareSQLCommands(dbobjects *dbo,char *errMsg)
{
	int returnCode;

	returnCode = sqlite3_prepare_v2(dbo->db,defineNewMeasurementSQL,-1,&(dbo->insertMeasurement),NULL);
	if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg, "Can't prepare insert statement %s (%i): %s\n", defineNewMeasurementSQL, returnCode, sqlite3_errmsg(dbo->db));
	    return (1);
	}

	returnCode = sqlite3_prepare_v2(dbo->db,writeMeasuredValuesSQL,-1,&(dbo->insertMeasuredValues),NULL);
	if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg, "Can't prepare insert statement %s (%i): %s\n", writeMeasuredValuesSQL, returnCode, sqlite3_errmsg(dbo->db));
	    return (1);
	}

	return (0);
}

int createNewMeasurement(dbobjects *dbo,const char *location,int interval,char *errMsg)
{
	int returnCode;
    char datetime[20];

	returnCode=sqlite3_bind_text(dbo->insertMeasurement,1,location,-1,NULL);
	if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
		return(1);
	}

	returnCode=sqlite3_bind_int(dbo->insertMeasurement,3,interval);
	if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
		return(1);
	}

	getCurrentTime(datetime);
	returnCode=sqlite3_bind_text(dbo->insertMeasurement,2,datetime,-1,NULL);
    if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
		return(1);
	}

	returnCode=sqlite3_step(dbo->insertMeasurement);
	if (returnCode!=SQLITE_DONE)
	{
		sprintf(errMsg,"Cannot insert a new measurement: %s\n",sqlite3_errmsg(dbo->db));
		return(1);
	}

	dbo->currentMeasurementID=sqlite3_last_insert_rowid(dbo->db);
	sqlite3_reset(dbo->insertMeasurement);
	return (0);
}

void getCurrentTime(char *currentTime)
{
	   time_t cTime;
	   struct tm *timeinfo;

	   time(&cTime);
	   timeinfo = localtime (&cTime );
	   strftime(currentTime, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

int writeMeasuredValue(dbobjects *dbo,float pressureValue, float temperatureValue,char *errMsg)
{
	int returnCode;
    char datetime[20];

    returnCode=sqlite3_bind_int64(dbo->insertMeasuredValues,1,dbo->currentMeasurementID);
    if (returnCode!=SQLITE_OK)
    {
    	sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
    	return(1);
    }

    getCurrentTime(datetime);
    returnCode=sqlite3_bind_text(dbo->insertMeasuredValues,2,datetime,sizeof(datetime),NULL);
    if (returnCode!=SQLITE_OK)
    {
        sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
        return(1);
    }

    returnCode=sqlite3_bind_double(dbo->insertMeasuredValues,3,pressureValue);
    if (returnCode!=SQLITE_OK)
    {
        sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
        return(1);
    }

    returnCode=sqlite3_bind_double(dbo->insertMeasuredValues,4,temperatureValue);
    if (returnCode!=SQLITE_OK)
    {
        sprintf(errMsg,"Error binding value %s\n",sqlite3_errmsg(dbo->db));
        return(1);
    }

	returnCode=sqlite3_step(dbo->insertMeasuredValues);
	if (returnCode!=SQLITE_DONE)
	{
		sprintf(errMsg,"Cannot insert a new measurement: %s\n",sqlite3_errmsg(dbo->db));
		return(1);
	}

	sqlite3_reset(dbo->insertMeasuredValues);

	return(0);
}


int closeDatabaseConnection(dbobjects *dbo,char *errMsg)
{
	int returnCode;
	sqlite3_finalize(dbo->insertMeasurement);
	sqlite3_finalize(dbo->insertMeasuredValues);

	returnCode=sqlite3_close(dbo->db);

	if (returnCode!=SQLITE_OK)
	{
		sprintf(errMsg,"Error closing database connection: %s\n",sqlite3_errmsg(dbo->db));
	    return(1);
	}

	return(0);

}

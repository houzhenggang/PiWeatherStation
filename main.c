/*
 * PiWeatherStation
 * main.c
 *
 * Copyright (c) 2013  Goce Boshkovski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include "db.h"
#include "configurator.h"
#include "bmp085.h"

#define SIGTIMER 35

static volatile sig_atomic_t loopFlag = 1;
static volatile sig_atomic_t timerExpired = 0;

void SIG_handler(int);
void terminate(dbobjects *,configuration *,char *);

int main(int argc, char **argv)
{
	//Daemon related variables
	pid_t pid, sid;
    	int lockFileDes;
    	char buffer[25];

    	openlog("Weather station daemon",LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    	syslog(LOG_INFO,"Starting a new weather station daemon.");

	//Create child
    	pid=fork();

    	if(pid<0)
    		exit(EXIT_FAILURE);
    	if(pid>0)
    		exit(EXIT_SUCCESS);

    	umask(0);
   	 sid=setsid();
    	if (sid<0)
    		exit(EXIT_FAILURE);
    	if ((chdir(".")) < 0)
    		exit(EXIT_FAILURE);
    	close(STDIN_FILENO);
    	close(STDOUT_FILENO);
    	close(STDERR_FILENO);

    	//Create a lock file to prevent starting several daemons in parallel
    	syslog(LOG_INFO,"Creating a lock file.");
    	lockFileDes=open("daemon.lock",O_RDWR|O_CREAT,0640);
    	if (lockFileDes<0)
   	{
    		syslog(LOG_INFO,"Cannot create a lock file.");
        	exit(1);
    	}
    	if(lockf(lockFileDes,F_TLOCK,0)<0)
    	{
      		syslog(LOG_INFO,"There is an instance of the daemon already running. Exiting.");
      		exit(1);
    	}
    	syslog(LOG_INFO,"Lock file created.");
    	sprintf(buffer,"%d\n",getpid());
    	if (write(lockFileDes,buffer,strlen(buffer))==0)
    	{
    		syslog(LOG_INFO,"Cannot write to the lock file.");
    		exit(1);
	 }

	//Timer related variables
	timer_t timerid;
	struct sigevent sevent;
	struct itimerspec its;
	configuration conf;

	//Signal definition and handling
	struct sigaction signalAction;
	//sigset_t signalMask;

	//Define SQLite3 object
	dbobjects dbo;

	char errorMessage[255];

        BMP085 *sensor;
        int sfd;

	//Read configuration file
	if (readConfiguration("./conf/weatherStation.cfg",&conf,errorMessage)<0)
	{
		syslog(LOG_INFO,"Failed to fetch configuration.");
		syslog(LOG_INFO,"%s",errorMessage);
		exit(1);
	}

	sensor = (BMP085 *) malloc(sizeof(BMP085));
	sensor->oss=conf.sensorMode;
	syslog(LOG_INFO,"Raspberry Pi i2c device interface: %s",conf.i2cBus);
        syslog(LOG_INFO,"Sensor address: 0x%02X",conf.sensorAddress);
        sensor->i2cAddress=(char)conf.sensorAddress;


	if (connect2BMP085(&sfd,conf.i2cBus,sensor->i2cAddress))
	{
		syslog(LOG_INFO,"Failed to connect to the sensor.");
		free(sensor);
		exit(1);
	}
	syslog(LOG_INFO,"Reading BMP085 calibration table.");
        readCalibrationTable(sfd,sensor);

	timerExpired=conf.interval;

	//Define signal handling
	signalAction.sa_flags=0;
	signalAction.sa_handler=SIG_handler; //timerExpired;
	sigemptyset(&(signalAction.sa_mask));
	//sigaction(SIGRTMIN,&signalAction,NULL);
	sigaction(SIGTIMER,&signalAction,NULL);
	sigaction(SIGINT,&signalAction,NULL);
	sigaction(SIGTERM,&signalAction,NULL);

	//Create new timer
	sevent.sigev_notify=SIGEV_SIGNAL;
	sevent.sigev_signo=SIGTIMER;
	sevent.sigev_value.sival_ptr=&timerid;
	timer_create(CLOCK_REALTIME,&sevent,&timerid);

	//Set timer interval
	its.it_interval.tv_sec=60;
	its.it_interval.tv_nsec=0;
	its.it_value.tv_sec=60;
	its.it_value.tv_nsec=0;

	if (openDatabase(conf.db,&dbo,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
		terminate(&dbo,&conf,errorMessage);
		exit(1);
	}

	if (prepareSQLCommands(&dbo,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
		terminate(&dbo,&conf,errorMessage);
		exit(1);
	}

	if (createNewMeasurement(&dbo,conf.location,conf.interval,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
		terminate(&dbo,&conf,errorMessage);
		exit(1);
	}

	syslog(LOG_INFO,"Current measurement id: %d\n",(int)dbo.currentMeasurementID);

	//Activate the timer
	timer_settime(timerid,0,&its,NULL);

	while(loopFlag)
	{
		if(timerExpired==0)
		{
			timerExpired=conf.interval;
			syslog(LOG_INFO,"%dmin timer expired.",conf.interval);
			makeMeasurement(sfd,sensor);
			if(writeMeasuredValue(&dbo,sensor->pressure/100.0,sensor->temperature,errorMessage))
			{
				syslog(LOG_INFO,"%s",errorMessage);
				break;
			}
		}
		sleep(60);
	}

	syslog(LOG_INFO,"Exiting...\n");
	terminate(&dbo,&conf,errorMessage);
	free (sensor);

	return (0);
}

void SIG_handler(int sig)
{
	switch(sig)
	{
		case SIGINT:
		case SIGTERM:
			loopFlag = 0;
			break;
		case SIGTIMER:
			timerExpired--;
			break;
	}
	return;
}

void terminate(dbobjects *dbo,configuration *conf,char *errorMessage)
{

	if (closeDatabaseConnection(dbo,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
	}
	free(conf->db);
	free(conf->location);
}

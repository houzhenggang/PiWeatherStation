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
#include "libbmp085.h"

#define SIGTIMER 35

//Exit switch 
static volatile sig_atomic_t loopFlag = 1;
//Timer has expired switch 
static volatile sig_atomic_t timerExpired = 0;
//Controls the config reload 
static volatile sig_atomic_t reloadConfig = 0;

//Function that handles the signals
void SIG_handler(int);
//Close database connection, free the memory resources
void terminate(dbobjects *,configuration *,BMP085 *,char *);
//Read the daemon configuration file, prepare the database and define sensor parameters
int init(dbobjects *,configuration *,BMP085 **,int *,char *);
//Start timer for 60s
void startTimer(timer_t *,struct itimerspec *);
//Stop the timer
void stopTimer(timer_t *,struct itimerspec *);

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
	struct itimerspec itsNULL;
	configuration conf;

	//Signal definition and handling
	struct sigaction signalAction;
	//sigset_t signalMask;

	//Define SQLite3 object
	dbobjects dbo;	
	dbo.db=NULL;

	char errorMessage[255];

	BMP085 *sensor;
    	int sfd;

	syslog(LOG_INFO,"Starting the init process.");
	if (init(&dbo,&conf,&sensor,&sfd,errorMessage))
	{
		syslog(LOG_INFO,"Faild to finish the initialisation process. Execution terminated.");
		terminate(&dbo,&conf,sensor,errorMessage);
		exit(1);
	}	
	timerExpired=conf.interval;

	//Define signal handling
	signalAction.sa_flags=0;
	signalAction.sa_handler=SIG_handler; //timerExpired;
	sigemptyset(&(signalAction.sa_mask));
	sigaction(SIGTIMER,&signalAction,NULL);
	sigaction(SIGINT,&signalAction,NULL);
	sigaction(SIGTERM,&signalAction,NULL);
	sigaction(SIGHUP,&signalAction,NULL);

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

	//Set structure to stop the timer
	itsNULL.it_interval.tv_sec=0;
        itsNULL.it_interval.tv_nsec=0;
        itsNULL.it_value.tv_sec=0;
        itsNULL.it_value.tv_nsec=0;

	//Activate the timer
	//timer_settime(timerid,0,&its,NULL);
	startTimer(&timerid,&its);

	while(loopFlag)
	{
		if(timerExpired==0)
		{
			timerExpired=conf.interval;
			syslog(LOG_INFO,"%dmin timer expired.",conf.interval);
			BMP085takeMeasurement(sfd,sensor);
			if(writeMeasuredValue(&dbo,sensor->pressure/100.0,sensor->temperature,errorMessage))
			{
				syslog(LOG_INFO,"%s",errorMessage);
				break;
			}
		}
		if(reloadConfig==1)
		{
			reloadConfig=0;
			syslog(LOG_INFO,"SIGHUP signal received. Reload configuration file.");
			stopTimer(&timerid,&itsNULL);
			terminate(&dbo,&conf,sensor,errorMessage);
		        if (init(&dbo,&conf,&sensor,&sfd,errorMessage))
		        {
                		syslog(LOG_INFO,"Faild to finish the initialisation process. Execution terminated.");
                		terminate(&dbo,&conf,sensor,errorMessage);
                		exit(1);
		        }
			timerExpired=conf.interval;
			startTimer(&timerid,&its);
		}	
		sleep(60);
	}

	syslog(LOG_INFO,"Exiting...\n");
	stopTimer(&timerid,&itsNULL);
	terminate(&dbo,&conf,sensor,errorMessage);

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
		case SIGHUP:
			reloadConfig = 1;
			break;
		case SIGTIMER:
			timerExpired--;
			break;
	}
	return;
}

void terminate(dbobjects *dbo,configuration *conf,BMP085 *sensor,char *errorMessage)
{

	if (dbo->db)
	{
		if (closeDatabaseConnection(dbo,errorMessage))
		{
			syslog(LOG_INFO,"%s",errorMessage);
		}
	}
	free(conf->db);
	free(conf->location);
	free(sensor);
}

int init(dbobjects *dbo,configuration *conf,BMP085 **sensor,int *sfd,char *errorMessage)
{
	//Read configuration file
	if (readConfiguration("./conf/weatherStation.cfg",conf,errorMessage)<0)
	{
		syslog(LOG_INFO,"Failed to fetch configuration.");
		syslog(LOG_INFO,"%s",errorMessage);
		return 1;
	}	

	*sensor = (BMP085 *) malloc(sizeof(BMP085));
    	//Update sensor connection parameters
    	(*sensor)->oss=conf->sensorMode;
    	syslog(LOG_INFO,"Raspberry Pi i2c device interface: %s",conf->i2cBus);
    	syslog(LOG_INFO,"Sensor address: 0x%02X",conf->sensorAddress);
    	(*sensor)->i2cAddress=(char)conf->sensorAddress;

	//Connect to the sensor
	if (connect2BMP085(sfd,conf->i2cBus,(*sensor)->i2cAddress))
	{
		syslog(LOG_INFO,"Failed to connect to the sensor.");
		return 1;
	}
	//Read calibration table of the sensor
	syslog(LOG_INFO,"Reading BMP085 calibration table.");
	if (readBMP085CalibrationTable(*sfd,*sensor)==1)
		return 1;
	
	//Define a new measurement in the database
    	if (openDatabase(conf->db,dbo,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
		return 1;
	}

	if (prepareSQLCommands(dbo,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
		return 1;
	}

	if (createNewMeasurement(dbo,conf->location,conf->interval,errorMessage))
	{
		syslog(LOG_INFO,"%s",errorMessage);
		return 1;
	}

	syslog(LOG_INFO,"Current measurement id: %d\n",(int)dbo->currentMeasurementID);
	
	return 0;
}

void startTimer(timer_t *timerid,struct itimerspec *its)
{
	timer_settime(*timerid,0,its,NULL);
	return;
}

void stopTimer(timer_t *timerid,struct itimerspec *itsNULL)
{
	timer_settime(*timerid,0,itsNULL,NULL);
	return;
}



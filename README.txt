PiWeatherStation

PiWeatherStation is Linux daemon written in C for Raspberry Pi that use Bosch BMP085 I2C sensor module from Adafruit (product ID 391) 
to record the barometric pressure and temperature in a database on a regular sampling intervals. The log messages generate by the daemon 
are written in /var/log/syslog by syslogd integration.

Usage

1. Load the I2C kernel modules to activate the I2C interface on Raspberry Pi.
2. Connect the BMP085 sensor module to Raspberry Pi as described on Adafruit web site.
3. Check the device connectivity and the I2C address of the sensor by using i2cdetect command
which is part of i2c-tools:

 #sudo i2cdetect -y -1

4. Update the configuration file to define the sensor parameters, sqlite3 database file, sampling interval(in min) and specify the location.
5. Run the PiWeatherStation executable to start the Linux daemon and begin recording the sensors readings for temperature and
atmospheric pressure. 
6. All actions done by the daemon are registered in the /var/log/syslog and can be monitored with:

 #tail -f /var/log/syslog

7. PiWeatherStation accepts 3 signals: SIGTERM, SIGINT to exit from the running state and SIGHUP to reload the configuration file if it was updated
while the daemon is still in the running state. There is no need to restart the daemon in order to start a new measurement with different settings then 
the ongoing one. 

Foe example, to stop the daemon use:

 #kill -SIGTERM <PiWeatherStation PID>

or to reload the configuration file:

 #kill -SIGHUP <PiWeatherStation PID>

where the  PID number can be faound in the daemon.lock file located in the PiWeatherStation working folder.



WARNING: The program is tested on Raspberry Pi Model B Revision 1 board.
Before use, always check the GPIO and I2C manuals for your Raspberry Pi board to
take possible hardware changes on Pi board in consideration.

The source is provided as it is without any warranty. Use it on your own risk!
The author does not take any responsibility for the damage caused while using this software.

For more references how to connect the sensor to Raspberry Pi check the official guide available
on Adafruit web site.
     

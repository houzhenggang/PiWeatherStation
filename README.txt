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

7. PiWeatherStation accepts two signals SIGTERM and SIGINT to exit form the running state. To stop the daemon use:

 #kill -SIGTERM <PiWeatherStation PID>

The PID is written int the daemon.lock file in the PiWeatherStation working folder.



WARNING: The program is tested on Raspberry Pi Model B Revision 1 board.
Before use, always check the GPIO and I2C manuals for your Raspberry Pi board to
take possible hardware changes on Pi board in consideration.

The source is provided as it is without any warranty. Use it on your own risk!
The author does not take any responsibility for the damage caused while using this software.

For more references how to connect the sensor to Raspberry Pi check the official guide available
on Adafruit web site.
     

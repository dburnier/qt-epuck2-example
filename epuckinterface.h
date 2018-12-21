#ifndef 	EPUCKINTERFACE_H
#define 	EPUCKINTERFACE_H

#include "serialport/serialport.h"
#include <string.h>

#define NUMBER_OF_PROX	8
#define NUMBER_OF_LEDS	8
#define MAXIMUM_SPEED	1000

class EPuckInterface
{
public:
	EPuckInterface(std::string port);	// port is the serial port to be used (i.e. "COM3:" or "/dev/rfcomm0")
	~EPuckInterface();

	// set left and right wheels speed
	void setSpeed(int left, int right);
	void setSpeed(int speed);

	// go straight on a short distance
	void go_straight();

	// output specified state (0, 1, 2) to specified LED (0-8)
	void setLed(int number, int state = 2);

	// write in an array the 8 proximity sensors values
	void readProximitySensor(int intensities[NUMBER_OF_PROX]);
	
	// simple wrapper around serialPort.sleep()
	void sleep(unsigned long ms);

protected:
	SerialPort serialPort;

private:
	// Sercom version, written when first connected
	std::string epuckVersion;

	std::string serialRequest(std::string request);
};

#endif

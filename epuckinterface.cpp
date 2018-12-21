#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "epuckinterface.h"


#define LIMIT_MIN_MAX(x, min, max) \
	do { \
		if (x > (max)) \
			x = (max); \
		else if (x < (min)) \
			x = (min); \
	} while(0)

#define LIMIT(x, y)	LIMIT_MIN_MAX(x, -y, y)


EPuckInterface::EPuckInterface(std::string port)
{
	serialPort.setCommPort(port);

	// connect to the serial port. quit on error
	if (serialPort.connect() != 0)
		exit(1);

	// send this dummy command twice, the first time's not working
	epuckVersion = serialRequest("V\n");
	epuckVersion = serialRequest("V\n");

	std::cout << "SerCom version: " << epuckVersion << std::endl;

	// blink LEDs to show correct behavior
	setLed(8, 1);
	serialPort.sleep(500);
	setLed(8, 0);
}


EPuckInterface::~EPuckInterface()
{
	setSpeed(0, 0);
}


void EPuckInterface::setLed(int number, int state)
{
	// number has to be between 0 and 8
	LIMIT_MIN_MAX(number, 0, NUMBER_OF_LEDS + 1);
	//valid states: 0 (off) 1 (on) 2 (change state)
	LIMIT_MIN_MAX(state, 0, 2);

	//writing command string
	std::ostringstream cmd;
	cmd << "L," << number << "," << state << "\n";
	
	//conversion from ostringstream to string needed to send command
	std::string command = cmd.str();

	//sending the command string to the robot
	serialRequest(command);
}


void EPuckInterface::setSpeed(int left, int right)
{
	// 1000 is the maximum speed
	LIMIT(left, MAXIMUM_SPEED);
	LIMIT(right, MAXIMUM_SPEED);

	//writing command string
	std::ostringstream cmd;
	cmd << "D," << left << "," << right << "\n";

	//conversion from ostringstream to string needed to send command
	std::string command = cmd.str();

	//sending the command string to the robot
	serialRequest(command);
}

void EPuckInterface::setSpeed(int speed)
{
	setSpeed(speed, speed);
}

void EPuckInterface::go_straight()
{
	setSpeed(MAXIMUM_SPEED, MAXIMUM_SPEED);

	serialPort.sleep(1000);

	setSpeed(0,0);
}


void EPuckInterface::readProximitySensor(int intensities[NUMBER_OF_PROX])
{
	char garbage;

	std::string sensor;

	sensor = serialRequest("N\n");

	std::istringstream decode(sensor); // "n,0,1,2,3,4,5,6,7"
	decode >> garbage; // remove first character 'n'

	for (int i = 0; i < NUMBER_OF_PROX; i++) {
		// remove comma character
		decode >> garbage;
		// save useful sensor value
		decode >> intensities[i];
	}
}


void EPuckInterface::sleep(unsigned long ms)
{
	serialPort.sleep(ms);
}


// using this function avoids garabage characters in the input buffers
// due to unmanaged answers
std::string EPuckInterface::serialRequest(std::string request)
{
	std::string result;

	serialPort.sendString(request);
	serialPort.receiveString(result);

	return result;
}


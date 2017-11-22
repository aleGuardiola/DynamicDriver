/*
* Arduino Control Library
* Arduino library to call functions via serial
* based on Christophe Diericx's library
* https://github.com/christophediericx/ArduinoDriver/blob/master/Source/ArduinoDriver/ArduinoListener/ArduinoListener.ino
*
* Name: DynamicDriver
* Author: Alejandro Guardiola
* License: MIT
* Special Thanks to Christophe Diericx
*/

#ifndef _DynamicDriver_h
#define _DynamicDriver_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h" 
#endif

#define dataMaxSize 128
#define dataRealSize 128 + 4

enum FunctionType
{
	BOOLEANT=0, 
	CHART,
	UCHART,
	BYTET,
	INTT,
	UINTT,
	WORD,
	LONG,
	ULONG,
	SHORT,	
	VOIDT
};

//represent a method to call via serial
class Function
{
public:
	Function();
	~Function();

	//Get the function info
	virtual FunctionType getReturnType() {};
	virtual int getArgumentsCount() {};
	virtual FunctionType* getArgumentTypes() {};

	//InvokingTheFunction
	virtual void execute(void** arguments, void* returnValue) = 0;

private:
	
};


class DynamicDriver
{
public:
	DynamicDriver();
	~DynamicDriver();

	virtual Function* getFunction(byte commandByte) {};

	void setup();
	void loop();

private:
	int functionsSize;
	Function** functions; //The functions
	byte data[dataRealSize]; //the data
	byte commandByte, lengthByte, syncByte, fletcherByte1, fletcherByte2;
	unsigned int fletcher16, f0, f1, c0, c1;	
	void* argumentsData[15];	
};


#endif


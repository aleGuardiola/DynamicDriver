# DynamicDriver
An Arduino library to call functions via Serial

# Getting Started
Download the the tow files, DynamicDriver.h and DynamicDriver.cpp
Include the DynamicDriver.h in your project.

# How to use it
First you need to create your custom functions for example:

```c++
class SetPinModeFunction : public Function //Function class need to be the base class
{
public:
	FunctionType getReturnType() //override getReturnType() and return your returnType,
  {                            //the types supported are in from FunctionType
	   return VOIDT;
	}

	int getArgumentsCount() //override getArgumentsCount() and return the quantity arguments of your function
	{
		return 2;
	}

	FunctionType* getArgumentTypes() //override getArgumentTypes() an array with the types of your arguments
	{
		return argumentTypes;
	}
	
	void execute(void** arguments, void *returnValue) //override exectute(), execute is called when the function is called via Serial
	{
		byte pin = *(byte*)arguments[0]; //get the first argument
		byte mode = *(byte*)arguments[1]; //get the second argument
		pinMode(pin, mode); //do the work
	}

private:
	FunctionType argumentTypes[2]{ FunctionType::BYTET, FunctionType::BYTET };
};
```

## Creating your listener
After you have created your customs functions create your listener
```c++

class MyDynamicDriver : public DynamicDriver //Dynamic Driver need to be the base class
{

public:
	MyDynamicDriver()
	{
		
	}

	Function* getFunction(byte commandByte) //only you need to override getFunction and return the function that correspond by id
	{
  // dont use 0
		switch (commandByte)
		{
		case 1:
			return &pinModeFunc;
			break;

		case 2:
			return &digitalWriteFunc;
			break;
		default:
			break;
		}
	}

private:
	SetPinModeFunction pinModeFunc;
	DigitalWriteFunction digitalWriteFunc;
		
};
```
After you have your listener you need to to call driver.setup() and driver.loop():

```c++
MyDynamicDriver driver;

void setup()
{
   driver.setup();
}

void loop()
{
   driver.loop();
}

```


## How to comunicate with the arduino
You need to use a client for now the only one that you can use is the C# client that is: https://github.com/aleGuardiola/ArduinoDynamicDriver
but you are welcome to provide a new one :).

# Special Thanks to Christophe Diericx

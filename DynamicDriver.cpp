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

#include "DynamicDriver.h"
#include "arduino.h"

//prototypes
bool expectNumberOfBytesToArrive(byte numberOfBytes, unsigned long timeout);
void WriteError();
unsigned int Fletcher16(byte data[], int count);
size_t getSize(FunctionType type);

const long BAUD_RATE = 115200;
const unsigned int SYNC_TIMEOUT = 30000;//250;
const unsigned int READ_REQUESTPAYLOAD_TIMEOUT = 60000;//1000
const unsigned int SYNC_REQUEST_LENGTH = 4;
const unsigned int CMD_HANDSHAKE_LENGTH = 3;

const byte START_OF_REQUEST_MARKER = 0xfb;
const byte ALL_BYTES_WRITTEN = 0xfa;
const byte START_OF_RESPONSE_MARKER = 0xf9;
const byte ERROR_MARKER = 0xef;
const byte VOID_RETURN_RESULT=0xfc;

const unsigned int PROTOCOL_VERSION_MAJOR = 1;
const unsigned int PROTOCOL_VERSION_MINOR = 2;

const byte CMD_HANDSHAKE_INITIATE = 0x01;
const byte ACK_HANDSHAKE_INITIATE = 0x02;

DynamicDriver::DynamicDriver()
{

}

DynamicDriver::~DynamicDriver()
{	
}

void DynamicDriver::setup()
{
	Serial.begin(BAUD_RATE);
	while (!Serial) { ; }
}

void DynamicDriver::loop()
{
	// Reset variables
	for (int i = 0; i < 128; i++) { data[i] = 0x00; }

	// Try to acquire SYNC. Try to read up to four bytes, and only advance if the pattern matches ff fe fd fc. 
	// Blocking (the client must retry if getting sync fails).
	while (Serial.available() < SYNC_REQUEST_LENGTH) { ; }
	if ((syncByte = Serial.read()) != 0xff) return;
	if ((syncByte = Serial.read()) != 0xfe) return;
	if ((syncByte = Serial.read()) != 0xfd) return;
	if ((syncByte = Serial.read()) != 0xfc) return;

	// Write out SYNC ACK (fc fd fe ff).
	Serial.write(0xfc);
	Serial.write(0xfd);
	Serial.write(0xfe);
	Serial.write(0xff);
	Serial.flush();

	// Now expect the START_OF_REQUEST_MARKER (0xfb), followed by our command byte, and a length byte
	// To acknowledge, we will write out the sequence in reverse (length byte, command byte, START_OF_REQUEST_MARKER)
	// We cannot be blocking as the client expects an answer. 
	if (!expectNumberOfBytesToArrive(CMD_HANDSHAKE_LENGTH, SYNC_TIMEOUT)) return;

	if (Serial.read() != START_OF_REQUEST_MARKER) return;
	commandByte = Serial.read();
	lengthByte = Serial.read();

	// Write out acknowledgement.
	Serial.write(lengthByte);
	Serial.write(commandByte);
	Serial.write(START_OF_REQUEST_MARKER);
	Serial.flush();

	// Read length bytes + 4 (first two bytes are commandByte + length repeated, last two bytes are fletcher16 checksums)
	if (!expectNumberOfBytesToArrive(lengthByte + 4, READ_REQUESTPAYLOAD_TIMEOUT)) return;
	for (int i = 0; i < lengthByte + 4; i++) { data[i] = Serial.read(); }

	fletcherByte1 = data[lengthByte + 2];
	fletcherByte2 = data[lengthByte + 3];

	// Expect all bytes written package to come in (0xfa), non blocking
	if (!expectNumberOfBytesToArrive(1, READ_REQUESTPAYLOAD_TIMEOUT)) return;
	if (Serial.read() != ALL_BYTES_WRITTEN) return;

	// Packet checks: do fletcher16 checksum!
	fletcher16 = Fletcher16(data, lengthByte + 2);
	f0 = fletcher16 & 0xff;
	f1 = (fletcher16 >> 8) & 0xff;
	c0 = 0xff - ((f0 + f1) % 0xff);
	c1 = 0xff - ((f0 + c0) % 0xff);

	// Sanity check of checksum + command and length values, so that we can trust the entire packet.
	if (c0 != fletcherByte1 || c1 != fletcherByte2 || commandByte != data[0] || lengthByte != data[1]) {
		WriteError();
		return;
	}

	//Work with the data------------------------------------------------------------------------------------------------------

	if (commandByte == CMD_HANDSHAKE_INITIATE)
	{
		Serial.write(START_OF_RESPONSE_MARKER);
		Serial.write(3);
		Serial.write(ACK_HANDSHAKE_INITIATE);
		Serial.write(PROTOCOL_VERSION_MAJOR);
		Serial.write(PROTOCOL_VERSION_MINOR);
		Serial.flush();
		return;
	}
	
	//get The correspond function
	Function* function = getFunction(commandByte);
	//Serial.write(START_OF_RESPONSE_MARKER);
	
	//Obtain the function parameters
	FunctionType returnType = function->getReturnType();	
	int argumentsCount = function->getArgumentsCount();
	FunctionType* arguments = function->getArgumentTypes();
	
	int pos = 2;
	
	//get all arguments
	for (int i = 0; i < argumentsCount; i++)
	{
		argumentsData[i] = &data[pos];
		FunctionType argument = arguments[i];
		pos += getSize(argument);
	}
		
	void* result;
	size_t returnTypeSize;

	if (returnType == VOIDT)
		result = NULL;
	else	
		result = malloc(returnTypeSize=getSize(returnType));
		
	function->execute(argumentsData, result);

	//free the result
	if (result != NULL)
	{
		Serial.write(START_OF_RESPONSE_MARKER);
		Serial.write(returnTypeSize);

		byte buf[4];
		int16_t int16;
		uint16_t uint16;
		switch (returnType)
		{
		case BOOLEANT:
			Serial.write(*(byte*)result);
			break;
		case CHART:
			Serial.write(*(byte*)result);
			break;
		case UCHART:
			Serial.write(*(byte*)result);
			break;
		case BYTET:
			Serial.write(*(byte*)result);
			break;
		case INTT:
			int16 = *(int16_t*)result;
			buf[0] = int16 & 255;
			buf[1] = (int16 >> 8) & 255;
			break;
		case UINTT:
			uint16 = *(uint16_t*)result;
			buf[0] = uint16 & 255;
			buf[1] = (uint16 >> 8) & 255;
			break;
		case WORD:
			uint16 = *(uint16_t*)result;
			buf[0] = uint16 & 255;
			buf[1] = (uint16 >> 8) & 255;
			break;
		case LONG:
			int16 = *(int16_t*)result;
			buf[0] = int16 & 255;
			buf[1] = (int16 >> 8) & 255;
			break;
		case ULONG:
			uint16 = *(uint16_t*)result;
			buf[0] = uint16 & 255;
			buf[1] = (uint16 >> 8) & 255;
			break;
		case SHORT:
			int16 = *(int16_t*)result;
			buf[0] = int16 & 255;
			buf[1] = (int16 >> 8) & 255;
			break;		
		default:
			break;
		}

		Serial.write(buf, returnTypeSize);
		free(result);
	}	
	else
	{
		Serial.write(START_OF_RESPONSE_MARKER);
		Serial.write(VOID_RETURN_RESULT);
	}
	Serial.flush();
}

size_t getSize(FunctionType type)
{
	size_t resultSize;

	switch (type)
	{
	case BOOLEANT:
		resultSize = sizeof(boolean);
		break;
	case CHART:
		resultSize = sizeof(char);
		break;
	case UCHART:
		resultSize = sizeof(unsigned char);
		break;
	case BYTET:
		resultSize = sizeof(byte);
		break;
	case INTT:
		resultSize = sizeof(int16_t);
		break;
	case UINTT:
		resultSize = sizeof(uint16_t);
		break;
	case WORD:
		resultSize = sizeof(word);
		break;
	case LONG:
		resultSize = sizeof(long long);
		break;
	case ULONG:
		resultSize = sizeof(unsigned long long);
		break;
	case SHORT:
		resultSize = sizeof(unsigned short);
		break;	
	}

	return resultSize;
}

bool expectNumberOfBytesToArrive(byte numberOfBytes, unsigned long timeout)
{
	unsigned long timeoutStartTicks = millis();
	while ((Serial.available() < numberOfBytes) && ((millis() - timeoutStartTicks) < timeout)) { ; }
	if (Serial.available() < numberOfBytes) {
		// Unable to get sufficient bytes, perhaps one was lost in transportation? Write out three error marker bytes.
		WriteError();
		return false;
	}
	return true;
}

void WriteError()
{
	for (int i = 0; i < 3; i++) { Serial.write(ERROR_MARKER); }
	Serial.flush();
}

unsigned int Fletcher16(byte data[], int count)
{
	unsigned int sum1 = 0;
	unsigned int sum2 = 0;
	unsigned int idx;
	for (idx = 0; idx < count; ++idx) {
		sum1 = (sum1 + data[idx]) % 255;
		sum2 = (sum2 + sum1) % 255;
	}
	return (sum2 << 8) | sum1;
}


//Function class
Function::Function()
{
}

Function::~Function()
{
}

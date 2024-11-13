#ifndef I2C_RW_H
#define I2C_RW_H

// Includes
#include "Arduino.h"
#include <Wire.h>    // I2C library

class I2C_RW
{
    public:
        // Members
        static int i2cVcnl4040Address;  // Will only be changed in testing to confirm address
        static int i2cSht40Address; 
        static int i2cFrequency;
        static int i2cSdaPin;
        static int i2cSclPin;

        // Initialization and debugging methods
        static void initI2C(int i2cVcnl4040Addr, int i2cSht40ddr, int i2cFreq, int pinSda, int pinScl);
        static void scanI2cLinesForAddresses(bool verboseConnectionFailures);
        static void printI2cReturnStatus(byte returnStatus, int bytesWritten, const char action[]);
        
        // 8-bit register methods
        static void getSht40Data(byte regAddr, float * temperature, float * humidity);
        static uint16_t readReg8Addr16Data(byte regAddr, int numBytesToRead, String action, bool verbose);
        static bool writeReg8Addr16DataWithProof(byte regAddr, int numBytesToWrite, uint16_t data, String action, bool verbose);
        static void writeReg8Addr16Data(byte regAddr, uint16_t data, String action, bool verbose);
};

#endif
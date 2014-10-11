/*
 * PCA9546 Library for Arduino
 *
 * MIT License
 *
 * Copyright Jeff Whitlatch - ko7m - 2014
 */

#include <Arduino.h>
#include <Wire.h>
#include "PCA9546.h"
#include "debug.h"


#define DEBUG(x ...)  // Default to NO debug    
//#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug

// Initialize the PCA9546 and disable all channels
PCA9546::PCA9546(uint8_t PCA9546_address)
{ 
  i2c_address = PCA9546_address;

  Wire.begin();
  selectChannel(PCA9546_NOCHANNEL);
}

// Initialize the PCA9546 and enable the channel(s) indicated
PCA9546::PCA9546(uint8_t PCA9546_address, uint8_t channel)
{ 
  i2c_address = PCA9546_address;

  Wire.begin();
  selectChannel(channel);
}

// Send a channel selection word to the PCA9546
bool PCA9546::selectChannel(uint8_t channel)
{
  // Sanity check value passed.  Only least significant 4 bits valid
  if (channel <= 0xf)
  {
    i2c_write(channel);
    DEBUG(P("Successfully selected PCA9546 channel"));
    status = PCA9546_SUCCESS;
  }
  else
  {
    DEBUG(P("PCA9546 channel selection failed"));
    status = PCA9546_ERROR;
  }
  return (PCA9546_SUCCESS == status);
}

// Write a byte to I2C device.  There is only a single register.  If multiple bytes written, last one wins.
void PCA9546::i2c_write(uint8_t data)
{
  Wire.beginTransmission(i2c_address);
  Wire.write(data);
  Wire.endTransmission();
}

// Read the one byte register from the I2C device
uint8_t PCA9546::i2c_read()
{
  uint8_t rdata = 0xFF;
  Wire.beginTransmission(i2c_address);
  Wire.requestFrom(i2c_address, (uint8_t)1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

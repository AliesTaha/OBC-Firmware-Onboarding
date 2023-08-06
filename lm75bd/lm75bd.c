#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config)
{
  error_code_t errCode;

  if (config == NULL)
    return ERR_CODE_INVALID_ARG;

  errCode = writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                              config->osOperationMode, config->devOperationMode);

  if (errCode != ERR_CODE_SUCCESS)
    return errCode;

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp)
{
  uint8_t accessBuffer = TEMP_REGISTER;
  uint8_t readData[READ_BYTES];
  float answer;

  /* Implement this driver function */
  /**
   * to read the current temperature from the temperature sensor. To implement this, you’ll need to select the
   * sensor's internal temperature register (since that's the register we're interested in reading) using the pointer register.
   * Once you select it, you can read the temperature from the sensor.
   * Look at section 7.4.1 for a description of the pointer register.
   *
   *
   * Look at Figure 11 on the datasheet for the timing diagrams of interest. The top diagram shows an I2C write that sets the pointer
   * register. The bottom diagram shows an I2C read that gets the data from the register you selected via the pointer register.
   *
   * Note that both the I2C write and read start by transmitting the I2C device address. Once you get the data from the register,
   * you'll need to perform a conversion to get the temperature in degrees Celsius. This conversion is described in section 7.4.3
   */
  i2cSendTo(devAddr, accessBuffer, WRITE_BYTES);
  i2cReceiveFrom(devAddr, readData, READ_BYTES);

  int16_t processedTemp = ((uint16_t)readData[0] << 3) | ((uint16_t)(readData[1]) >> 5);

  if (readData[0] >> 7 == 0)
  { // positive temp
    answer = processedTemp * 0.125;
    *temp = answer;
  }
  else
  { // negative temp
    processedTemp = ~processedTemp + 1;
    // Negate everything, good, except now have 1111 at the front which i don't want
    // Use bitwise or to remove those
    processedTemp ^= 0xF800;
    answer = processedTemp * 0.125;
    *temp = answer;
    // two's complement
    //- that * 0.125
  }

  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                               uint8_t osOperationMode, uint8_t devOperationMode)
{
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize)
  {
  case 1:
    osFaltQueueRegData = 0;
    break;
  case 2:
    osFaltQueueRegData = 1;
    break;
  case 4:
    osFaltQueueRegData = 2;
    break;
  case 6:
    osFaltQueueRegData = 3;
    break;
  default:
    return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS)
    return errCode;

  return ERR_CODE_SUCCESS;
}

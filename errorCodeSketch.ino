#include "msp430g2553.h"
#include <pfatfsEdited.h>
#include <pffconfEdited.h>

#include "SPI.h" 
#include "pfatfsEdited.h"

void setup()
{
  Serial.begin(9600);
  //0
  Serial.print("FR_OK: ");
  Serial.println(FR_OK);
  //1
  Serial.print("FR_DISK_ERR: ");
  Serial.println(FR_DISK_ERR);
  
  //Serial.print("FR_INT_ERR: ");
  //Serial.println(FR_INT_ERR);
  
  //2
  Serial.print("FR_NOT_READY: ");
  Serial.println(FR_NOT_READY);
  
  //3
  Serial.print("FR_NO_FILE: ");
  Serial.println(FR_NO_FILE);
  
  //4
  Serial.print("FR_NO_PATH: ");
  Serial.println(FR_NO_PATH);
  
  //Serial.print("FR_INVALID_NAME: ");
  //Serial.println(FR_INVALID_NAME);
  
  //Serial.print("FR_DENIED: ");
  //Serial.println(FR_DENIED);
  
  //Serial.print("FR_EXIST: ");
  //Serial.println(FR_EXIST);
  
 // Serial.print("FR_INVALID_OBJECT: ");
 // Serial.println(FR_INVALID_OBJECT);
  
  //Serial.print("FR_WRITE_PROTECTED: ");
  //Serial.println(FR_WRITE_PROTECTED);
  
  //Serial.print("FR_INVALID_DRIVE: ");
  //Serial.println(FR_INVALID_DRIVE);
  
  
  //6
  Serial.print("FR_NOT_ENABLED: ");
  Serial.println(FR_NOT_ENABLED);
  
  //7
  Serial.print("FR_NO_FILESYSTEM: ");
  Serial.println(FR_NO_FILESYSTEM);
  
  //Serial.print("FR_MKFS_ABORTED: ");
  //Serial.println(FR_MKFS_ABORTED);
  
  //Serial.print("FR_TIMEOUT: ");
  //Serial.println(FR_TIMEOUT);
  
  //Serial.print("FR_LOCKED: ");
  //Serial.println(FR_LOCKED);
  
  //Serial.print("FR_NOT_ENOUGH_CORE: ");
  //Serial.println(FR_NOT_ENOUGH_CORE);
  
  //Serial.print("FR_TOO_MANY_OPEN_FILES: ");
  //Serial.println(FR_TOO_MANY_OPEN_FILES);
  
  //Serial.print("FR_INVALID_PARAMETER: ");
  //Serial.println(FR_INVALID_PARAMETER);
}

void loop()
{
  // put your main code here, to run repeatedly:
  
}

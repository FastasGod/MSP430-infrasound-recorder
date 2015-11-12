/* This is the config file for the MSP432 INFRASOUND project */

#define ADC_SAMPLE_FREQUENCY            1000                      //Measured in Hz
#define BUFFER_RAM                      192                        //Measured in bytes
#define BUFFER_COUNT                    2
#define BUFFER_SIZE                     BUFFER_RAM/(BUFFER_COUNT+1)  //In current setup there will be recording buffers and
//a separate
#define HEADER_SIZE                     512    

#define NUM_SECTOR_WRITES               5
#define LOG_FILES_LOCATION             "LOGFILES"

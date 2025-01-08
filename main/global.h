#include <M5Core2.h>
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
#include "BME280I2C.h"
//#define BME_ENABLE
extern bool catchupActive;
extern TimerHandle_t threadTimer;
extern QueueHandle_t networkQueue;
extern QueueHandle_t storageQueue;

extern SemaphoreHandle_t displayMutex;
extern SemaphoreHandle_t storageMutex;
extern SFEWeatherMeterKit weatherMeterKit;
extern BME280I2C bme;
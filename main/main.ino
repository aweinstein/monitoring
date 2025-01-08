/*

Muestra los datos del pluviometro

SparkFun Weather Meter Library:
https://github.com/sparkfun/SparkFun_Weather_Meter_Kit_Arduino_Library

*/

#include <freertos/semphr.h>
#include <M5Core2.h>
#include <WiFi.h>
#include "time.h"
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
#include "esp_console.h"

// Enable BME280
//#define BME_ENABLE
#include "BME280I2C.h"

// Send data without sensors
#define DEBUG

#include "helper.h"
#include "network.h"
#include "console.h"
#include "storage.h"
#include "screen.h"

// Timer periods in milliseconds
#define TIMER_PERIOD 10 * 1000 

// Queue size for threads
#define STORAGE_QUEUE 50
#define NETWORK_QUEUE 200

// Mutexes
SemaphoreHandle_t displayMutex = NULL;
SemaphoreHandle_t storageMutex = NULL;

// Weather station definitions
int rain_fall_pin = 27;
int wind_direction_pin = 35; 
int wind_speed_pin = 19; 

// Global variables
SFEWeatherMeterKit weatherMeterKit(wind_direction_pin, wind_speed_pin, rain_fall_pin);

BME280I2C::Settings bmeSettings(
  BME280::OSR_X1, 
  BME280::OSR_X1, 
  BME280::OSR_X1, 
  BME280::Mode_Forced, 
  BME280::StandbyTime_1000ms, 
  BME280::Filter_16, 
  BME280::SpiEnable_False, 
  BME280I2C::I2CAddr_0x76); 
BME280I2C bme(bmeSettings);

bool catchupActive = false;
TimerHandle_t threadTimer;
TaskHandle_t networkThread;
TaskHandle_t storageThread;
QueueHandle_t storageQueue;
QueueHandle_t networkQueue;

#ifdef DEBUG
extern float temp;
extern float hum;
extern float pres;
#endif

/* 
  Callback function for timer to read and queue data to other threads
  TODO: Use this callback function to check thread status if necessary
*/
void timer_pushData(TimerHandle_t timer) {
  time_t cur_time = getUnixTimestamp();
  #ifndef DEBUG
  float temp = bme.temp();
  float hum = bme.hum();
  float pres = bme.pres();
  if(isnan(temp)) 
    temp = 0;
  if(isnan(hum)) 
    hum = 0;
  if(isnan(pres)) 
    pres = 0;
  sensor_data data = {
    .timestamp = getUnixTimestamp(),
    .rain_fall = weatherMeterKit.getTotalRainfall(),
    .wind_speed = weatherMeterKit.getWindSpeed(),
    .wind_direction = weatherMeterKit.getWindDirection(),
    .temperature = temp,
    .humidity = hum,
    .pressure = pres,
    .init = true,
  };
  #endif
  #ifdef DEBUG
  sensor_data data = {
    .timestamp = getUnixTimestamp(),
    .rain_fall = weatherMeterKit.getTotalRainfall(),
    .wind_speed = weatherMeterKit.getWindSpeed(),
    .wind_direction = weatherMeterKit.getWindDirection(),
    .temperature = temp,
    .humidity = hum,
    .pressure = pres,
    //.temperature = 25 + ((rand() % 20)/10.0)-1,
    //.humidity = 50 + (rand() % 4)-2,
    //.pressure = 1010 + (rand() % 8) - 4,
    .init = true,
  };
  #endif
  xQueueSend(networkQueue, &data, 0);
  xQueueSend(storageQueue, &data, 0);
}

void setup() {
  configTime(-4 * 3600, 3600, NULL); // Not a good idea, but doing this temporarily
  M5.begin(true, true, false, true); //Init M5Core2.
  init_screen();
  
  // Initialize display mutex
  displayMutex = xSemaphoreCreateMutex();
  if(displayMutex == NULL) {
    printf("Failed to initialize display mutex! Aborting...\n");
    return;
  }
  xSemaphoreGive(displayMutex);

  // Initialize storage mutex
  storageMutex = xSemaphoreCreateMutex();
  if(storageMutex == NULL) {
    printf("Failed to initialize storage mutex! Aborting...\n");
    return;
  }
  xSemaphoreGive(storageMutex);

  init_console();
  int err = 0;
  esp_console_run("setWifi ap m5core2 password1234", &err);
  if(err) {
    writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "Couldn't start AP", RED, BLACK, right);
  };
  esp_console_run("setDB 192.168.4.2 8086", &err);

  #ifdef SFE_WMK_PLAFTORM_UNKNOWN
    weatherMeterKit.setADCResolutionBits(10);
    printf(F("Unknown platform! Please edit the code with your ADC resolution!\n"));
  #endif
  weatherMeterKit.begin();

  #ifdef BME_ENABLE
    while(!bme.begin())
    {
      printf("Could not find BME280 sensor!\n");
      delay(1000);
    }

    switch(bme.chipModel())
    {
      case BME280::ChipModel_BME280:
        printf("Found BME280 sensor! Success.\n");
        break;
      case BME280::ChipModel_BMP280:
        printf("Found BMP280 sensor! No Humidity available.\n");
        break;
      default:
        printf("Found UNKNOWN sensor! Error!\n");
    }
  #endif

  // Initialize queues for storage and network threads
  storageQueue = xQueueCreate(STORAGE_QUEUE, sizeof(sensor_data));
  networkQueue = xQueueCreate(NETWORK_QUEUE, sizeof(sensor_data));

  if(storageQueue == NULL || networkQueue == NULL) {
    printf("CRITICAL: Failed to initialize queue, not enough memory!\n");
  }
  xTaskCreate(display_screen, "display_screen", 4096, NULL, 1, NULL); // Render screen
  xTaskCreate(store_data, "store_data", 4096, NULL, 2, &storageThread); // Write to SD
  xTaskCreate(upload_data, "upload_data", 4096*2, NULL, 3, &networkThread); // Write to web server

  threadTimer = xTimerCreate("threadTimer", pdMS_TO_TICKS(TIMER_PERIOD), pdTRUE, (void*) 0, timer_pushData);
  if(xTimerStart(threadTimer, 100) == pdFAIL) {
    printf("CRITICAL: Failed to start thread timer!\n");
  }
}

void loop() {
}

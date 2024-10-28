/*

Muestra los datos del pluviometro

SparkFun Weather Meter Library:
https://github.com/sparkfun/SparkFun_Weather_Meter_Kit_Arduino_Library

*/

#define font_size 1

// Text distribution on screen
#define title_row 1 * font_size * 10
#define time_row 2 * font_size * 10
#define rain_row 3 * font_size * 10
#define anemometer_row 4 * font_size * 10
#define vane_row 5 * font_size * 10
#define temp_row 6 * font_size * 10
#define hum_row 7 * font_size * 10
#define pres_row 8 * font_size * 10

// Timer periods in milliseconds
#define TIMER_PERIOD 10 * 1000 

// Queue size for threads
#define STORAGE_QUEUE 50
#define NETWORK_QUEUE 200

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
//#define DEBUG

#include "helper.h"
#include "network.h"
#include "console.h"

// RTC definitions
RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;
char timeStrbuff[64];
char weatherStrbuff[64];
char SDStrbuff[64];
struct tm timeinfo;
SemaphoreHandle_t displaySemaphore = NULL;

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
sensor_data storageData;

// Functions
void print_time() {
  M5.Rtc.GetTime(&RTCtime);
  M5.Rtc.GetDate(&RTCDate);
  sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
          RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
          RTCtime.Seconds);
  writeToScreen(10, time_row, timeStrbuff);
}

void print_weatherkit_data() {
  // Rainfall
  sprintf(weatherStrbuff, "Rainfall: %f [mm]\n", weatherMeterKit.getTotalRainfall());
  writeToScreen(10, rain_row, weatherStrbuff);

  // Anemometer
  sprintf(weatherStrbuff, "Wind Speed: %0.5f [km/h]\n", weatherMeterKit.getWindSpeed());
  writeToScreen(10, anemometer_row, weatherStrbuff);

  // Wind vane
  // Could display via directional arrow graphic alongside numerical value?
  sprintf(weatherStrbuff, "Wind Vane: %0.1f [deg]\n", weatherMeterKit.getWindDirection());
  writeToScreen(10, vane_row, weatherStrbuff);

  #ifdef BME_ENABLE
    // Temperature (BME does not provide a precise reading)
    sprintf(weatherStrbuff, "Temperature: %0.1f [C]]\n", bme.temp());
    writeToScreen(10, temp_row, weatherStrbuff);

    // Humidity
    sprintf(weatherStrbuff, "Humidity: %0.1f [deg]\n", bme.hum());
    writeToScreen(10, hum_row, weatherStrbuff);

    // Pressure
    sprintf(weatherStrbuff, "Pressure: %0.1f [deg]\n", bme.pres());
    writeToScreen(10, pres_row, weatherStrbuff);
  #endif
}

void store_data(void* _) {
  while(1) {
    // Wait to receive data from queue
    xQueueReceive(storageQueue, &storageData, portMAX_DELAY);
    writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "                      ");
    // Create new log every day
    M5.Rtc.GetDate(&RTCDate);
    sprintf(SDStrbuff, "/weather-data_%d-%02d-%02d.csv", RTCDate.Year, 
          RTCDate.Month, RTCDate.Date);
    File file = SD.open(SDStrbuff, FILE_APPEND);
    if (file) {
      sprintf(SDStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
          RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
          RTCtime.Seconds);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.rain_fall);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.wind_speed);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.wind_direction);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.temperature);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.humidity);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.pressure);
      file.printf(SDStrbuff);

      file.printf("\n");
      file.close();
      writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "Wrote to SD");
    } 
    else {
      writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "Error writing to file", RED, BLACK);
    }
    sleep(2000);
    writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "                            ");
    sleep(1000);
  }
}

void display_screen(void* _) {
  while(1) {
    print_time();
    print_weatherkit_data();
    delay(100);
  }
}

/* 
  Callback function for timer to read and queue data to other threads
  TODO: Use this callback function to check thread status if necessary
*/
void timer_pushData(TimerHandle_t timer) {
  time_t cur_time = getUnixTimestamp();
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
    .rain_fall = weatherMeterKit.getTotalRainfall(),
    .wind_speed = weatherMeterKit.getWindSpeed(),
    .wind_direction = weatherMeterKit.getWindDirection(),
    .temperature = temp,
    .humidity = hum,
    .pressure = pres,
    .timestamp = getUnixTimestamp(),
  };
  xQueueSend(networkQueue, &data, 100);
  xQueueSend(storageQueue, &data, 100);
}

void setup() {
  M5.begin(true, true, false, true); //Init M5Core2.
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(10, title_row);
  M5.Lcd.setTextSize(font_size);
  M5.Lcd.print("SparkFun Weather Kit");
  displaySemaphore = xSemaphoreCreateBinary();
  if(displaySemaphore == NULL) {
    printf("Failed to initialize display semaphore! Aborting...\n");
    return;
  }
  xSemaphoreGive(displaySemaphore);
  init_console();
  int err = 0;
  esp_console_run("wificonf ap m5core2 password1234", &err);
  if(err) {
    writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "Couldn't start AP", RED, BLACK, right);
  };
  esp_console_run("dbconf 192.168.4.2 8086", &err);
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

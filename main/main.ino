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
#define atm_row 7 * font_size * 10
#define humidity_row 8 * font_size * 10

#include <freertos/semphr.h>
#include <M5Core2.h>
#include <WiFi.h>
#include "time.h"
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
#include "BME280I2C.h"

#include "helper.h"
#include "network.h"

// RTC definitions
RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;
char timeStrbuff[64];
char weatherStrbuff[64];
char SDStrbuff[64];
struct tm timeinfo;
SemaphoreHandle_t displaySemaphore = NULL;

// Weather station definitions
int rain_fall_pin = 27;  // TODO: Find the right value
int wind_direction_pin = 35; // TODO: Find the right value
int wind_speed_pin = 14; // TODO: Find the right value
int temperature_pin = 6; // TODO: Find the right value
int pressure_pin =  7; // TODO: Find the right value
int humidity_pin = 8; // TODO: Find the right value
SFEWeatherMeterKit weatherMeterKit(wind_direction_pin, wind_speed_pin, rain_fall_pin);

//BME280I2C bme(bme_settings);

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
  sprintf(weatherStrbuff, "Wind Speed: %0.5f [k/h]\n", weatherMeterKit.getWindSpeed());
  writeToScreen(10, anemometer_row, weatherStrbuff);

  // Wind vane
  // Could display via directional arrow graphic alongside numerical value?
  sprintf(weatherStrbuff, "Wind Vane: %0.1f [deg]\n", weatherMeterKit.getWindDirection());
  writeToScreen(10, vane_row, weatherStrbuff);

  // Implement temp displays via GPIO
  // ...
}

void store_weatherkit_data(void* _) {
  while(1) {
    // Create new log every day
    M5.Rtc.GetTime(&RTCtime);
    M5.Rtc.GetDate(&RTCDate);
    sprintf(SDStrbuff, "/weather-data_%d-%02d-%02d.csv", RTCDate.Year, 
          RTCDate.Month, RTCDate.Date);
    File file = SD.open(SDStrbuff, FILE_APPEND);
    if (file) {
      sprintf(SDStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
          RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
          RTCtime.Seconds);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", weatherMeterKit.getTotalRainfall());
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", weatherMeterKit.getWindSpeed());
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", weatherMeterKit.getWindDirection());
      file.printf(SDStrbuff);
      file.println("");
      file.close();
      writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "Wrote to SD");
    } 
    else {
      writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "Error al abrir el archivo", RED, BLACK);
    }
    delay(3000);
    writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "                ");
    delay(7000);
  }
}

void display_screen(void* _) {
  while(1) {
    print_time();
    print_weatherkit_data();
    delay(100);
  }
}


void setup() {
  M5.begin(); //Init M5Core2.
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(10, title_row);
  M5.Lcd.setTextSize(font_size);
  M5.Lcd.print("SparkFun Weather Kit");
  BaseType_t wiFiTask;
  TaskHandle_t wiFiHandle = NULL;
  xTaskCreate(start_wifi, "start_wifi", 4096, NULL, 1, &wiFiHandle);
  Serial.begin(115200);
  Serial.print("\n");
  Serial.println(F("Testing the rain fall thingy"));
  #ifdef SFE_WMK_PLAFTORM_UNKNOWN
    weatherMeterKit.setADCResolutionBits(10);
    Serial.println(F("Unknown platform! Please edit the code with your ADC resolution!"));
    Serial.println();
  #endif
  weatherMeterKit.begin();
  //bme.begin();
  displaySemaphore = xSemaphoreCreateBinary();
  if(displaySemaphore == NULL) {
    Serial.println("Failed to initialize display semaphore! Aborting...");
    return;
  }
  xSemaphoreGive(displaySemaphore);
  xTaskCreate(display_screen, "display_screen", 4096, NULL, 1, NULL); // Render screen
  xTaskCreate(store_weatherkit_data, "store_weatherkit_data", 4096, NULL, 2, NULL); // Write to SD
  xTaskCreate(upload_data, "upload_data", 4096*2, NULL, 3, NULL); // Write to web server
}

void loop() {
}

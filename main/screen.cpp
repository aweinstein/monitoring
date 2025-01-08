#include <M5Core2.h>
#include "screen.h"
#include "helper.h"
#include "global.h"

#define font_size 1
#define DEBUG
// Constants used for screen placement
#define title_row 1 * font_size * 10
#define time_row 2 * font_size * 10
#define rain_row 3 * font_size * 10
#define anemometer_row 4 * font_size * 10
#define vane_row 5 * font_size * 10
#define temp_row 6 * font_size * 10
#define hum_row 7 * font_size * 10
#define pres_row 8 * font_size * 10

// Globals
RTC_TimeTypeDef RTCTime_screen;
RTC_DateTypeDef RTCDate_screen;
char timeStrbuff[64];
char weatherStrbuff[64];
char directionBuff[4];
#ifdef DEBUG
float temp;
float hum;
float pres;
#endif
// Functions
void print_time() {
  M5.Rtc.GetTime(&RTCTime_screen);
  M5.Rtc.GetDate(&RTCDate_screen);
  sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate_screen.Year,
          RTCDate_screen.Month, RTCDate_screen.Date, RTCTime_screen.Hours, RTCTime_screen.Minutes,
          RTCTime_screen.Seconds);
  writeToScreen(10, time_row, timeStrbuff);
}

void print_weatherkit_data() {
  // Rainfall
  sprintf(weatherStrbuff, "Rainfall: %f [mm]       \n", weatherMeterKit.getTotalRainfall());
  writeToScreen(10, rain_row, weatherStrbuff);

  // Anemometer
  sprintf(weatherStrbuff, "Wind Speed: %0.5f [km/h]      \n", weatherMeterKit.getWindSpeed());
  writeToScreen(10, anemometer_row, weatherStrbuff);
  // Wind vane
  // Could display via directional arrow graphic alongside numerical value?
  angleToDirection(weatherMeterKit.getWindDirection(), directionBuff);
  sprintf(weatherStrbuff, "Wind Vane: %0.4s    \n", directionBuff);
  writeToScreen(10, vane_row, weatherStrbuff);

  #if defined(BME_ENABLE)
    // Temperature (BME does not provide a precise reading)
    sprintf(weatherStrbuff, "Temperature: %0.1f [C]      \n", bme.temp());
    writeToScreen(10, temp_row, weatherStrbuff);

    // Humidity
    sprintf(weatherStrbuff, "Humidity: %0.1f% [RH]     \n", bme.hum());
    writeToScreen(10, hum_row, weatherStrbuff);

    // Pressure
    sprintf(weatherStrbuff, "Pressure: %0.1f [hPa]     \n", bme.pres());
    writeToScreen(10, pres_row, weatherStrbuff);

  #elif defined(DEBUG)
    // Temperature (BME does not provide a precise reading)
    if(rand() % 100 > 90) {
      temp = fmax(24.6, temp + ((rand() % 3)/10.0)-0.1);
      temp = fmin(temp, 25.5);
    }
    sprintf(weatherStrbuff, "Temperature: %0.1f [C]      \n", temp);
    writeToScreen(10, temp_row, weatherStrbuff);

    // Humidity
    if(rand() % 100 > 90) {
      hum = fmax(49, hum + ((rand() % 3)/10.0)-0.1);
      hum = fmin(hum, 51);
    }
    sprintf(weatherStrbuff, "Humidity: %0.1f [RH]     \n", hum);
    writeToScreen(10, hum_row, weatherStrbuff);

    // Pressure
    if(rand() % 100 > 90) {
      pres = fmax(1009, pres + ((rand() % 3)/10.0)-0.1);
      pres = fmin(pres, 1011);
    }
    sprintf(weatherStrbuff, "Pressure: %0.1f [hPa]     \n", pres);
    writeToScreen(10, pres_row, weatherStrbuff);

  #endif
}

void display_screen(void* _) {
  while(1) {
    print_time();
    print_weatherkit_data();
    delay(100);
  }
}

void init_screen() {
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(10, title_row);
  M5.Lcd.setTextSize(font_size);
  M5.Lcd.print("SparkFun Weather Kit");
}
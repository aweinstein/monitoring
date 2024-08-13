/*

Muestra los datos del pluviometro

SparkFun Weather Meter Library:
https://github.com/sparkfun/SparkFun_Weather_Meter_Kit_Arduino_Library

*/
#include <M5Core2.h>
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"

// RTC definitions
RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;
char timeStrbuff[64];

// Weather station definitions
int rain_fall_pin = 27;  // TODO: Find the right value
int wind_direction_pin = 35; // TODO: Find the right value
int wind_speed_pin = 14; // TODO: Find the right value
SFEWeatherMeterKit weatherMeterKit(wind_direction_pin, wind_speed_pin, rain_fall_pin);

void show_time() {
  M5.Rtc.GetTime(&RTCtime);  // Gets the time in the real-time clock.
  M5.Rtc.GetDate(&RTCDate);
  sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
          RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
          RTCtime.Seconds);
  M5.lcd.setCursor(10, 100);
  M5.Lcd.println(timeStrbuff);
}

void show_battery() {
  sprintf(timeStrbuff, "Bat Voltage:%f\n", M5.Axp.GetBatVoltage());
  M5.lcd.setCursor(10, 120);
  M5.Lcd.println(timeStrbuff);
}

void show_rain_gauge() {
  sprintf(timeStrbuff, "Rainfall: %f [mm]\n", weatherMeterKit.getTotalRainfall());
  Serial.print(timeStrbuff);
  M5.lcd.setCursor(10, 120);
  M5.Lcd.println(timeStrbuff);
  //Serial.print(F("Total rainfall (mm): "));
  //Serial.println(weatherMeterKit.getTotalRainfall(), 1);
}

void setup() {
  M5.begin(); //Init M5Core2. Initialize M5Core2
  M5.lcd.setCursor(10, 80);
  M5.lcd.setTextSize(2);
  M5.Lcd.print("Pluviometro");

  Serial.begin(115200);
  Serial.print("\n");
  Serial.println(F("Testing the rain fall thingy"));
  #ifdef SFE_WMK_PLAFTORM_UNKNOWN
    weatherMeterKit.setADCResolutionBits(10);
    Serial.println(F("Unknown platform! Please edit the code with your ADC resolution!"));
    Serial.println();
  #endif
    weatherMeterKit.begin();

}

void loop() {
    show_time();
    // show_battery();
    show_rain_gauge();
    delay(1000);
}

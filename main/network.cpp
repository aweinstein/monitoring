#include <time.h>
#include <M5Core2.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "network.h"
#include "helper.h"

const char* ssid = "m5core2";
const char* password = "password1234";
const char* ntpServer = "ntp.shoa.cl";
const char* httpServer = "192.168.187.2";
char dataBuf[400];
char debugBuf[50];
extern SemaphoreHandle_t displaySemaphore;

#include "BME280I2C.h"
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
extern BME280I2C bme;
extern SFEWeatherMeterKit weatherMeterKit;
HTTPClient Http;

void configRTCLocalTime() {
  struct tm timeinfo;
  RTC_DateTypeDef RTC_DateStruct;
  RTC_TimeTypeDef RTC_TimeStruct;
  
  configTime(-4 * 3600, 3600, ntpServer);
  while(!getLocalTime(&timeinfo)) {
    delay(500); // Wait until the local time is obtained
  }
  RTC_DateStruct.WeekDay = timeinfo.tm_wday;
  RTC_DateStruct.Date = timeinfo.tm_mday;
  RTC_DateStruct.Month = timeinfo.tm_mon + 1;
  RTC_DateStruct.Year = timeinfo.tm_year + 1900;
  RTC_TimeStruct.Seconds = timeinfo.tm_sec;
  RTC_TimeStruct.Minutes = timeinfo.tm_min;
  RTC_TimeStruct.Hours = timeinfo.tm_hour;
  M5.Rtc.SetDate(&RTC_DateStruct);
  M5.Rtc.SetTime(&RTC_TimeStruct);
}

void start_wifi(void* _) {
  //WiFi.softAP(ssid, password)
  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED) {
    writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "Couldn't connect to network", RED, BLACK, right);
    delay(2000); // Wait 2s before checking connection again
  }
  writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "                              ", WHITE, BLACK, right);
  writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "Connected to network", WHITE, BLACK, right);
  Serial.println(WiFi.localIP());
  configRTCLocalTime();
  vTaskDelete(NULL);
}

void upload_data(void* _) {
  RTC_DateTypeDef RTC_DateStruct;
  RTC_TimeTypeDef RTC_TimeStruct;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000); // Check for internet connection periodically
  }
  writeToScreen(0, M5.Lcd.height()-10, "Connecting...", RED, BLACK);

  while(Http.begin(httpServer, 8086, "/api/v2/write?org=weather-station-group&bucket=weather-records&precision=s") == 0) {
    writeToScreen(0, M5.Lcd.height()-10, "Failed to connect to server    ");
    delay(6000); // Attempt to connect periodically
  }

  Http.addHeader("Content-Type", "text/plain; charset=utf-8");
  Http.addHeader("Authorization", "Token fhXh88keqv2kLUkhEsgDYMiyUOJcGhUebRp93gzu3v_iB-0mFIHgOWZVl__SO89bD3lH-UvBLWjsD88741tFyw==");

  while(true) {
    time_t cur_time = getUnixTimestamp();
    sprintf(dataBuf, 
    "weather,sensor_id=SFEWeatherMeterKit,location=test rain_fall=%f,wind_speed=%f,wind_direction=%f %d \n \
    weather,sensor_id=bme280,location=test temperature=%f,humidity=%f,pressure=%f %d", 
    weatherMeterKit.getTotalRainfall(), weatherMeterKit.getWindSpeed(), weatherMeterKit.getWindDirection(), cur_time, 
    bme.temp(), bme.hum(), bme.pres(), cur_time);
    writeToScreen(0, M5.Lcd.height()-10, "POSTing to db...         ");
    int httpCode = Http.POST(dataBuf);
    delay(5000);
    sprintf(debugBuf, "Returned %d               ", httpCode);
    writeToScreen(0, M5.Lcd.height()-10, debugBuf);
    delay(5000); // Send again every 10 seconds
  }
  
  vTaskDelete(NULL);
}
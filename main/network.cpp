#include <time.h>
#include <M5Core2.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "network.h"
#include "helper.h"


const char* ssid = "VTR-1829990";
const char* password = "s9qYsCtwfqbv";
const char* ntpServer = "ntp.shoa.cl";
const char* httpServer = "192.168.0.123";
char dataBuf[400];
char debugBuf[100];
// Includes user ID, not sure if important to separate 
extern SemaphoreHandle_t displaySemaphore;

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
  Http.begin(httpServer, 8086, "/api/v2/write?org=weather-station-group&bucket=weather-records&precision=s");
  Http.addHeader("Content-Type", "text/plain; charset=utf-8");
  Http.addHeader("Authorization", "Token fhXh88keqv2kLUkhEsgDYMiyUOJcGhUebRp93gzu3v_iB-0mFIHgOWZVl__SO89bD3lH-UvBLWjsD88741tFyw==");
  while(true) {
    /*
    sprintf(dataBuf, " \
    { \
      \"create_time\": \"%d/%02d/%02d %02d:%02d:%02d\", \
      \"rain_fall\": 1, \
      \"wind_speed\": 2, \
      \"wind_direction\": 3, \
      \"temperature\": 4, \
      \"pressure\": 5, \
      \"humidity\": 6 \
    } \
    ", RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date, 
      RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);
    */
    time_t cur_time = getUnixTimestamp();
    sprintf(dataBuf, 
    "weather,sensor_id=SFEWeatherMeterKit,location=test rain_fall=73.97038159354763,wind_speed=35.23103248356096,wind_direction=0.48445310567793615 %d", cur_time, cur_time);
    writeToScreen(0, M5.Lcd.height()-10, "POSTing to db...         ");
    int httpCode = Http.POST(dataBuf);
    delay(5000);
    sprintf(debugBuf, "Returned %d               ", httpCode);
    writeToScreen(0, M5.Lcd.height()-10, debugBuf);
    delay(5000); // Send again every 10 seconds
  }
  vTaskDelete(NULL);
}
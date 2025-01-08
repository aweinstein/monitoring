#include <time.h>
#include <M5Core2.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "network.h"
#include "helper.h"
#include "global.h"

#define MAX_ATTEMPTS 3

const char* ntpServer = "ntp.shoa.cl";
const char* httpServer = "192.168.4.2";
const int port = 8086; // Default InfluxDB port
char dataBuf[400];
char debugBuf[50];
sensor_data data = {0};
extern SemaphoreHandle_t displaySemaphore;

#include "BME280I2C.h"
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
extern BME280I2C bme;
extern SFEWeatherMeterKit weatherMeterKit;
HTTPClient Http;

// HTTPClient's connected method doesn't seem to work, so just track the status ourselves
bool httpActive = false;

void sink_func(void* _) {
  return;
}

void configRTCLocalTime() {
  struct tm timeinfo;
  RTC_DateTypeDef RTC_DateStruct;
  RTC_TimeTypeDef RTC_TimeStruct;
  
  configTime(0, 0, ntpServer);
  setenv("TZ", "<-04>4<-03>,M9.1.6/24,M4.1.6/24", 1);
  tzset();
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

bool start_wifi_cmd(const char* ssid, const char* password, bool isAP) {
  if(isAP) {
    return WiFi.softAP(ssid, password);
  }
  TimerHandle_t timer = xTimerCreate("wifi_timer", pdMS_TO_TICKS(10000), pdFALSE, NULL, &sink_func);
  xTimerStart(timer, pdMS_TO_TICKS(10));
  WiFi.disconnect(true, true);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "Couldn't connect to network", RED, BLACK, right);
    delay(1000); // Wait 1s before checking connection again
    writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "                              ", WHITE, BLACK, right);
    if(!xTimerIsTimerActive(timer)) {
      printf("Gave up on connecting to network\n");
      return false;
    }
  }
  writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "                              ", WHITE, BLACK, right);
  writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "Connected to network", WHITE, BLACK, right);
  printf("IP address obtained: %s\n", WiFi.localIP().toString().c_str());
  configRTCLocalTime();
  delay(1000);
  writeToScreen(M5.Lcd.width(), M5.Lcd.height()-10, "                              ", WHITE, BLACK, right);
  return true;
}

void start_http_server(const char* httpServer, const int port) {
  if(httpActive = true) {
    Http.end();
    httpActive = false;
  }
  if(!WiFi.isConnected() && (WiFi.softAPSSID() == NULL)) {
    printf("Failed to start HTTP connection: WiFi is not connected\n");
    return;
  }
  if(!Http.begin(httpServer, port, "/api/v2/write?org=weather-station-group&bucket=weather-records&precision=s")) {
    printf("Failed to connect to server\n");
    return;
  }
  printf("Connection was successful\n");
  Http.addHeader("Content-Type", "text/plain; charset=utf-8");
  Http.addHeader("Authorization", "Token pmSEgLNxbcXsM5r0M2foylcUYPna-M3uz2v5oCmAMlCHihqJaCXsb-4Ehy5cP84UjeMUXbN5K2Y-p0boAxVs7w==");
  httpActive = true;
  return;
}

void upload_data(void* _) {
  while(true) {
    xQueueReceive(networkQueue, &data, portMAX_DELAY);
    while(!httpActive) {
      sleep(5000);
    }
    if(isnan(data.humidity))
      data.humidity = 0;
    if(isnan(data.pressure))
      data.pressure = 0;
    if(isnan(data.temperature))
      data.temperature = 0;
    sprintf(dataBuf, 
      "weather,sensor_id=SFEWeatherMeterKit,location=test rain_fall=%f,wind_speed=%f,wind_direction=%f %d \n \
      weather,sensor_id=bme280,location=test temperature=%f,humidity=%f,pressure=%f %d", 
      data.rain_fall, data.wind_speed, data.wind_direction, data.timestamp, 
      data.temperature, data.humidity, data.pressure, data.timestamp);

    while(true) {
      int httpCode = Http.POST(dataBuf);
      clearRegion(0, M5.Lcd.height()-10, 30);
      if(httpCode == 204) {
        writeToScreen(0, M5.Lcd.height()-10, "Sent data successfully");
        break;
      }
      // Handle POST failures
      if(httpCode == -1) {
        int i = 0;
        while(i < MAX_ATTEMPTS) {
          writeToScreen(0, M5.Lcd.height()-10, "Failed to send data");
          delay(3000);
          clearRegion(0, M5.Lcd.height()-10, 30);
          delay(1000);
          i++;
        }
      }
      //printf("Returned %d, dropping packet\n", httpCode);
      sprintf(debugBuf, "Returned %d, dropping packet", httpCode);
      writeToScreen(0, M5.Lcd.height()-10, debugBuf);
      delay(3000);
      clearRegion(0, M5.Lcd.height()-10, strlen(debugBuf));
    }
  }
  vTaskDelete(NULL);
}

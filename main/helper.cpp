/*
  Helper functions, primarily to avoid race conditions between tasks

  TODO: Add screen write function that clears after certain time has passed
*/
#include <M5Core2.h>
#include <time.h>
#include "helper.h"
#include "global.h"
#define screen_width 320
#define screen_height 240
#define char_width 8

char* directionArray[8] = {"N", "NE", 
                           "E", "SE", 
                           "S", "SW",
                           "W", "NW"};
char unixStrBuff[40];

// Writes to screen with thread safety. Left/Right specifies anchor point location
void writeToScreen(int x, int y, char* str, uint16_t textColor, uint16_t bgColor, enum Direction direction) {
  int len = 0;
  if(direction == right) {
    len = strlen(str) * 6;
  }
  xSemaphoreTake(displayMutex, portMAX_DELAY);
  M5.Lcd.setTextColor(textColor, bgColor);
  M5.Lcd.setCursor(x - len, y);
  M5.Lcd.println(str);
  M5.Lcd.setTextColor(WHITE, BLACK);
  xSemaphoreGive(displayMutex);
}

void clearRegion(int x, int y, int len) {
  xSemaphoreTake(displayMutex, portMAX_DELAY);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(x,y);
  // Generate string with the correct amount of spaces
  char* buf = (char*) malloc(len+1);
  memset(buf, ' ', len);
  buf[len] = '\0'; 
  
  M5.Lcd.print(buf);
  xSemaphoreGive(displayMutex);
  free(buf);
}

// Retrieves unix time from the RTC
time_t getUnixTimestamp() {
  RTC_DateTypeDef RTCDate;
  RTC_TimeTypeDef RTCTime;
  M5.Rtc.GetTime(&RTCTime);
  M5.Rtc.GetDate(&RTCDate);
  sprintf(unixStrBuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
          RTCDate.Month, RTCDate.Date, RTCTime.Hours, RTCTime.Minutes,
          RTCTime.Seconds);
  struct tm time = {0};
  if(strptime(unixStrBuff, "%Y/%m/%d %H:%M:%S", &time) != NULL) {
    time.tm_isdst = _daylight;
    return mktime(&time);
  }
  else {
    return -1;
  }
}

sensor_data deserializeSensorData(char* str) {
  sensor_data data = { .init = false };
  char* token;
  char* tokenArray[7] = {0};
  int i = 0;
  char* buf; // For strtok_r thread safety
  // Split our string into 7 tokens (matching our struct)
  token = strtok_r(str, ",", &buf);
  while(token != NULL && i < 7) {
    tokenArray[i] = token;
    i++;
    token = strtok_r(str, ",", &buf); 
  }
  // Not enough tokens for struct, malformed file? Return just incase
  if(i != 7) { 
    printf("Wrong amount of tokens\n");
    return data;
  }
  struct tm time = {0};
  time_t str_timestamp = 0;
  // Malformed date, so return without initializing.
  if(strptime(tokenArray[0], "%Y/%m/%d %H:%M:%S", &time) == NULL) {
    printf("Malformed date %s\n", tokenArray[0]);
    return data;
  }
  time.tm_isdst = _daylight;
  str_timestamp = mktime(&time);

  data.timestamp = str_timestamp;
  data.rain_fall = atof(tokenArray[1]);
  data.wind_speed = atof(tokenArray[2]);
  data.wind_direction = atof(tokenArray[3]);
  data.temperature = atof(tokenArray[4]);
  data.humidity = atof(tokenArray[5]);
  data.pressure = atof(tokenArray[6]);
  data.init = true;
  return data;
}

bool angleToDirection(float ang, char* buf) {
  int i;
  for(i = 0; i < 8; i++) {
    if(ang >= 45 * i && ang < 45 * (i+1)) {
      memset(buf, 0, 4);
      strncpy(buf, directionArray[i], 4);
      return true;
    }
  }
  return false;
}

void setRTC(tm timeinfo) {
  RTC_DateTypeDef RTC_DateStruct;
  RTC_TimeTypeDef RTC_TimeStruct;
  
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

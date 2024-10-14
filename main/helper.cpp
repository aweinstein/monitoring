/*
  Helper functions, primarily to avoid race conditions between tasks
*/
#include <M5Core2.h>
#include <time.h>
#include "helper.h"
#define screen_width 320
#define screen_height 240
#define char_width 8
char unixStrBuff[40];
extern SemaphoreHandle_t displaySemaphore;

// Writes to screen with thread safety. Left/Right specifies anchor point location
void writeToScreen(int x, int y, char* str, uint16_t textColor, uint16_t bgColor, enum Direction direction) {
  int len = 0;
  if(direction == right) {
    len = strlen(str) * 6;
  }
  xSemaphoreTake(displaySemaphore, portMAX_DELAY);
  M5.Lcd.setTextColor(textColor, bgColor);
  
  M5.Lcd.setCursor(x - len, y);
  M5.Lcd.println(str);
  M5.Lcd.setTextColor(WHITE, BLACK);
  xSemaphoreGive(displaySemaphore);
}

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
    return mktime(&time);
  }
  else {
    return -1;
  }
}
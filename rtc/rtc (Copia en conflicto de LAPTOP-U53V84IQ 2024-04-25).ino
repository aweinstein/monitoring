#include <M5Core2.h>

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

char timeStrbuff[64];

void flushTime() {
    M5.Rtc.GetTime(&RTCtime);  // Gets the time in the real-time clock.
    M5.Rtc.GetDate(&RTCDate);
    sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
            RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
            RTCtime.Seconds);
    M5.lcd.setCursor(10, 100);
    M5.Lcd.println(timeStrbuff);
}

void setupTime() {
    RTCtime.Hours   = 10;
    RTCtime.Minutes = 10;
    RTCtime.Seconds = 30;
    if (!M5.Rtc.SetTime(&RTCtime)) Serial.println("wrong time set!");
    RTCDate.Year  = 2024; 
    RTCDate.Month = 1;
    RTCDate.Date  = 25;
    if (!M5.Rtc.SetDate(&RTCDate)) Serial.println("wrong date set!");
}

void setup(){
  M5.begin(); //Init M5Core2. Initialize M5Core2
  // Remove comments to set date and time
  delay(1000);
  setupTime();
}

void loop() {
    flushTime();
    delay(1000);
}

#include <M5Core2.h>

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

char timeStrbuff[64];

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

void setupTime() {
    RTCtime.Hours   = 5;
    RTCtime.Minutes = 29;
    RTCtime.Seconds = 50;
    if (!M5.Rtc.SetTime(&RTCtime)) Serial.println("wrong time set!");
    RTCDate.Year  = 2024; 
    RTCDate.Month = 2;
    RTCDate.Date  = 28;
    if (!M5.Rtc.SetDate(&RTCDate)) Serial.println("wrong date set!");
}

void setup(){
  M5.begin(); //Init M5Core2. Initialize M5Core2
  // Remove comments of next two lines to set date and time
  //delay(1000);
}


void loop() {
    show_time();
    show_battery();
    delay(1000);
}

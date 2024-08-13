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
    //M5.lcd.setCursor(10, 100);
    M5.lcd.setCursor(0, 0);
    M5.Lcd.println(timeStrbuff);
}

void setup(){
  M5.begin(); //Init M5Core2. Initialize M5Core2
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("SD init...");
  if(!SD.begin()) {
    M5.Lcd.println("SD card error");
  } else {
    M5.Lcd.println("SD Card OK");
  }
}

void loop() {
  flushTime();
  File file = SD.open("/sd_test.csv", FILE_APPEND);
  if (file) {
    file.println(timeStrbuff);
    file.close();

    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("Line added to SD");
    delay(200);
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("                ");
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("Error al abrir el archivo");
  }
    delay(800);
}

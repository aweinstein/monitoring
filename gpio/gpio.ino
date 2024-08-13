#include <M5Core2.h>

#define PIN_INPUT 27

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
  sprintf(timeStrbuff, "Bat Voltage: %f\n", M5.Axp.GetBatVoltage());
  M5.lcd.setCursor(10, 120);
  M5.Lcd.println(timeStrbuff);
}

void show_gpio() {
  sprintf(timeStrbuff, "Value of PIN %d: %d \n", PIN_INPUT, digitalRead(PIN_INPUT));
  M5.lcd.setCursor(10, 140);
  M5.Lcd.println(timeStrbuff);
  Serial.print(timeStrbuff);
}

void setup(){
  M5.begin(); //Init M5Core2. Initialize M5Core2
  M5.begin(); //Init M5Core2. Initialize M5Core2
  M5.lcd.setCursor(10, 80);
  M5.lcd.setTextSize(2);
  M5.Lcd.print("GPIO");

  Serial.begin(115200);
  Serial.print("\n");
  Serial.println(F("Testing the GPIO"));

  pinMode(PIN_INPUT, INPUT_PULLUP);
}


void loop() {
  show_time();
  show_battery();
  show_gpio();
  delay(250);
}

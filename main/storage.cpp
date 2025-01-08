#include <M5Core2.h>
#include "storage.h"
#include "helper.h"
#include "global.h"


RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;
sensor_data storageData;
char SDStrbuff[64];

void store_data(void* _) {
  while(1) {
    // Wait to receive data from queue
    xQueueReceive(storageQueue, &storageData, portMAX_DELAY);
    clearRegion((M5.Lcd.width()-(11*6))/2, 120, 30);
    // Create new log every day
    M5.Rtc.GetDate(&RTCDate);
    M5.Rtc.GetTime(&RTCtime);
    sprintf(SDStrbuff, "/weather-data_%d-%02d-%02d.csv", RTCDate.Year, 
          RTCDate.Month, RTCDate.Date);
    File file = SD.open(SDStrbuff, FILE_APPEND);
    if (file) {
      xSemaphoreTake(storageMutex, portMAX_DELAY);
      sprintf(SDStrbuff, "%d/%02d/%02d %02d:%02d:%02d", RTCDate.Year,
          RTCDate.Month, RTCDate.Date, RTCtime.Hours, RTCtime.Minutes,
          RTCtime.Seconds);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.rain_fall);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.wind_speed);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.wind_direction);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.temperature);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.humidity);
      file.printf(SDStrbuff);
      sprintf(SDStrbuff, ",%f", storageData.pressure);
      file.printf(SDStrbuff);

      file.printf("\n");
      file.close();
      xSemaphoreGive(storageMutex);
      writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "Wrote to SD");
    } 
    else {
      writeToScreen((M5.Lcd.width()-(11*6))/2, 120, "Error writing to file", RED, BLACK);
    }
    //delay(2000);
    clearRegion((M5.Lcd.width()-(11*6))/2, 120, 30);
    //delay(1000);
  }
}

// Read data from SD card, ranging between two UNIX timestamps, and queue it
void readDataAndQueue(time_t timestamp, time_t timestamp_end) {
  double diff = difftime(timestamp_end, timestamp);
  struct tm date = *localtime(&timestamp);
  char readBuffer[256];
  size_t read = 0;

  while(diff >= 0) {
    sensor_data data;
    sprintf(readBuffer, "/weather-data_%d-%02d-%02d.csv", date.tm_year+1900, 
          date.tm_mon+1, date.tm_mday);
    File file = SD.open(readBuffer, FILE_READ);
    if(file) {
      printf("Reading file %s\n", readBuffer);
      while(true) {
        xSemaphoreTake(storageMutex, portMAX_DELAY);
        read = file.readBytesUntil('\n', readBuffer, sizeof(readBuffer));
        xSemaphoreGive(storageMutex);
        // EOL, move forward to next file
        if(read == 0)
          break;
        data = deserializeSensorData(readBuffer);
        // Malformed data, so we move on
        if (!data.init) {
          printf("Ran into malformed string while reading %s\n", file.name());
          continue;
        }
        printf("Reading data from %d\n", data.timestamp);
        // Too early, so we skip this entry
        if(data.timestamp < timestamp)
          continue;
        // Reached the last timestamp, so we exit
        if(data.timestamp > timestamp_end)
          break;
        xQueueSend(networkQueue, &data, portMAX_DELAY);
      }
      file.close();
    }
    // Advance day by 1 and set time to 0
    date.tm_mday += 1;
    date.tm_sec = 0;
    date.tm_min = 0;
    date.tm_hour = 0;
    timestamp = mktime(&date);
    diff = difftime(timestamp_end, timestamp);
  }
}
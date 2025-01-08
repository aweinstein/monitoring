#include "BME280I2C.h"
#include <time.h>

typedef struct {
  time_t timestamp;
  float rain_fall;
  float wind_speed;
  float wind_direction;
  float temperature;
  float humidity;
  float pressure;
  bool init;
} sensor_data;

enum Direction { left, right };
void writeToScreen(int x, int y, char* str, uint16_t textColor = WHITE, uint16_t bgColor = BLACK, enum Direction direction = left);
void clearRegion(int x, int y, int len);
time_t getUnixTimestamp();
sensor_data deserializeSensorData(char* str);
bool angleToDirection(float ang, char* buf);
void setRTC(tm time);
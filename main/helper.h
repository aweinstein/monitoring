#include "BME280I2C.h"

enum Direction { left, right };
void writeToScreen(int x, int y, char* str, uint16_t textColor = WHITE, uint16_t bgColor = BLACK, enum Direction direction = left);
time_t getUnixTimestamp();

typedef struct {
  float rain_fall;
  float wind_speed;
  float wind_direction;
  float temperature;
  float humidity;
  float pressure;
  int timestamp;
} sensor_data;
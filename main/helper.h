#include "BME280I2C.h"

enum Direction { left, right };
void writeToScreen(int x, int y, char* str, uint16_t textColor = WHITE, uint16_t bgColor = BLACK, enum Direction direction = left);
time_t getUnixTimestamp();


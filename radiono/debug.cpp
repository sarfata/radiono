
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include "debug.h"

void debug(char const *fmt, ... ) {
  char tmp[64]; // resulting string limited to 64 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, sizeof(tmp), fmt, args);
  va_end (args);
  Serial.println(tmp);
}


/*
void debug2(const __FlashStringHelper *c) {
    Serial.println(c);
}
*/


void debugUnique(char const *fmt, ... ) {
    static byte checkSumPrev = 0;
    byte checkSum = 0;
    char tmp[64+2]; // resulting string limited to 64 chars
    byte *pb = (byte *)&tmp;
  
    va_list args;
    va_start (args, fmt );
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end (args);
  
    // Compute new Checksum for tmp
    for (int i = 0; *pb && i < sizeof(tmp); i++) checkSum += *pb++;
    if ((int)checkSum == (int)checkSumPrev) return;
    checkSumPrev = checkSum;
    Serial.println(tmp);
}

// End

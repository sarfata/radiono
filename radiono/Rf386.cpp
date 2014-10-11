//Rf386.cpp
// Support for RF386 Filter Selection

#include <Arduino.h>
#include "A1Main.h"
#include "debug.h"

#define DEBUG(x ...)
//#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug

void setRf386BandSignal(unsigned long freq){
  // This setup is compatable with the Minima RF386 RF Power Amplifier
  // See: http://www.hfsignals.org/index.php/RF386

  // Bitbang Clock Pulses to Change PA Band Filter
  int band;
  static int prevBand;
  static unsigned long prevFreq;

  if (freq == prevFreq) return;
  prevFreq = freq;
   
  if      (freq <  4000000UL) band = 4; //   3.5 MHz
  else if (freq < 10200000UL) band = 3; //  7-10 MHz
  else if (freq < 18200000UL) band = 2; // 14-18 MHz
  else if (freq < 30000000UL) band = 1; // 21-28 MHz
  else band = 1;

  //debug("Band Index = %d", band);
  
  if (band == prevBand) return;
  prevBand = band;
  
  DEBUG(P("BandIndex = %d"), band);
  pinMode(PA_BAND_CLK, OUTPUT);
  
  digitalWrite(PA_BAND_CLK, 1);  // Output Reset Pulse for PA Band Filter
  delay(10);
  digitalWrite(PA_BAND_CLK, 0);

  while (band > 1) { // Output Selector Pulses to Change PA Band Filter
     delay(2);
     digitalWrite(PA_BAND_CLK, 1);
     band--;
     digitalWrite(PA_BAND_CLK, 0);
  }
}

//End

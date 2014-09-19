/*
 * Radiono - The Minima's Main Arduino Sketch
 * Copyright (C) 2013 Ashar Farhan
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Modified by: Jeff Witlatch - KO7M - Copyright (C) 2014
 *   Added 1Hz VFO Resolution
 *   Added Support for PCA9546 I2C Mux
 *   Added Support for I2C LCD
 *
 * Modified by: Eldon R. Brown (ERB) - WA0UWH - Copyright (C) 2014
 *   Added An Alternate Tuning Method, with Cursor and POT
 *   Added Multiple Button Support
 *   Added RIT Function
 *   Added Manual Sideband Selection 
 *   Added Band Selection UP/DOWN
 *   Added Band Selection Memories for both VFO_A and VFO_B
 *   Added Band Limits for Transmit
 *   Added Alternate VFO for Transmit, Split Operation
 *   Added Rf386 Band Selecter Encoder
 *   Added Edit and Set IF Frequency for Dial Calibration for USB and LSB
 *   Added Support for 2500Hz Tuning Mode
 *   Added Non-Volatile Memory Storage
 *   Added Morse Code Functions, MACROs and Beacon 
 *   Added New Cursor Blink Strategy
 *   Added Some new LCD Display Format Functions
 *   Added Idle Timeout for Blinking Cursor
 *   Added Sideband Toggle while in Edit-IF-Mode
 *
 */

void setup(); // # A Hack, An Arduino IED Compiler Preprocessor Fix


//#define RADIONO_VERSION "0.4"
#define RADIONO_VERSION "0.4.erb" // Modifications by: Eldon R. Brown - WA0UWH
#define INC_REV "ko7m-AC"         // Incremental Rev Code
#define INC_REV "ERB_FR"          // Incremental Rev Code

//#define USE_PCA9546	1         // Define this symbol to include PCA9546 support
//#define USE_I2C_LCD	1         // Define this symbol to include i2c LCD support
//#define USE_MULTIPLEXED_BUTTONS 1 // Define this symbol to use a series of cascaded buttons+pullup
//#define USE_ALTERNATE_TUNING 1  // Define this to use a classic radio tuning where buttons are used to move active digit
                                  // You need the USE_MULTIPLEXED_BUTTONS and multiple buttons for this
/*
 * Wire is only used from the Si570 module but we need to list it here so that
 * the Arduino environment knows we need it.
 */
#include <Wire.h>
#ifndef USE_I2C_LCD
  #include <LiquidCrystal.h>
#else
  #include <LiquidTWI.h>
#endif

#define LCD_COL (16)
#define LCD_ROW (2)
//#define LCD_COL (20)
//#define LCD_ROW (4)
#define LCD_STR_CEL "%-16.16s"
//#define LCD_STR_CEL "%-20.20s"  // For 20 Character LCD Display


#include <avr/io.h>
#include "A1Main.h"
#ifdef USE_PCA9546
  #include "PCA9546.h"
#endif
#include "Si570.h"
#include "debug.h"
#include "NonVol.h"
#include "Rf386.h"
#include "MorseCode.h"
#include "Macro.h"

#ifdef USE_PCA9546
  #define PCA9546_I2C_ADDRESS 0x70
#endif
#define SI570_I2C_ADDRESS   0x55

// USB and LSB IF frequencies
#define IF_FREQ_USB   (19997000L)
#define IF_FREQ_LSB   (19992000L)
//#define IF_FREQ_USB   (0)  // FOR debug ONLY
//#define IF_FREQ_LSB   (0)  // FOR debug ONLY

#define CW_TIMEOUT (600L) // in milliseconds, this is the parameter that determines how long the tx will hold between cw key downs

#define MAX_FREQ (30000000UL)
#define DEAD_ZONE (40)

enum ButtonPressModes { // Button Press Modes
    MOMENTARY_PRESS = 1,
    DOUBLE_PRESS,
    LONG_PRESS,
    ALT_PRESS_FN,
    ALT_PRESS_LT,
    ALT_PRESS_RT,
};

enum Buttons { // Button Numbers
    FN_BTN = 1,
    LT_CUR_BTN,
    RT_CUR_BTN,
    LT_BTN,
    UP_BTN,
    DN_BTN,
    RT_BTN,
};

enum SidebandModes { // Sideband Modes
    AUTO_SIDEBAND_MODE = 0,
    UPPER_SIDEBAND_MODE,
    LOWER_SIDEBAND_MODE,
};

enum VFOs { // Available VFOs
    VFO_A = 0,
    VFO_B,
};

// Pin Number for the digital output controls
#define LSB (2)
#define TX_RX (3)
#define CW_KEY (4)

// Pin Numbers for analog inputs
#define FN_PIN (A3)
#define ANALOG_TUNING (A2)
#define ANALOG_KEYER (A1)

#ifdef USE_PCA9546
PCA9546 *mux;
#endif
Si570 *vfo;
#ifndef USE_I2C_LCD
  LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
#else
  LiquidTWI lcd(0);   // I2C backpack display on 20x4 or 16x2 LCD display
#endif

unsigned long frequency = 14285000UL; //  20m - QRP SSB Calling Freq
unsigned long iFreqUSB = IF_FREQ_USB;
unsigned long iFreqLSB = IF_FREQ_LSB;

unsigned long vfoA = frequency, vfoB = frequency;
unsigned long cwTimeout = 0;
boolean editIfMode = false;

char b[LCD_COL+6], c[LCD_COL+6];  // General Buffers, used mostly for Formating message for LCD
char blinkChar[2];

/* tuning pot stuff */
byte refreshDisplay = 0;
unsigned int blinkCount = 0;

int tuningDir = 0;
int tuningPosition = 0;
int tune2500Mode = 0;
int freqUnStable = 1;
int tuningPositionDelta = 0;
int cursorDigitPosition = 0;
int tuningPositionPrevious = 0;
int cursorCol, cursorRow, cursorMode;
char* const sideBandText[] PROGMEM = {"Auto SB","USB","LSB"};
byte sideBandMode = 0;

boolean tuningLocked = 0; //the tuning can be locked: wait until Freq Stable before unlocking it
boolean inTx = 0, inPtt = 0;
boolean keyDown = 0;
boolean isLSB = 0;
boolean vfoActive = VFO_A;

/* modes */
int ritVal = 0;
boolean ritOn = 0;
boolean AltTxVFO = 0;
boolean isAltVFO = 0;

// PROGMEM is used to avoid using the small available variable space
const unsigned long bandLimits[BANDS*2] PROGMEM = {  // Lower and Upper Band Limits
      1800000UL,  2000000UL, // 160m
      3500000UL,  4000000UL, //  80m
      7000000UL,  7300000UL, //  40m
     10100000UL, 10150000UL, //  30m
     14000000UL, 14350000UL, //  20m
     18068000UL, 18168000UL, //  17m
     21000000UL, 21450000UL, //  15m
     24890000UL, 24990000UL, //  12m
     28000000UL, 29700000UL, //  10m
   //50000000UL, 54000000UL, //   6m - Will need New Low Pass Filter Support
   };

// An Array to save: A-VFO & B-VFO
unsigned long freqCache[BANDS*2] = { // Set Default Values for Cache
      1825000UL, 1825000UL,  // 160m - QRP SSB Calling Freq
      3985000UL, 3985000UL,  //  80m - QRP SSB Calling Freq
      7285000UL, 7285000UL,  //  40m - QRP SSB Calling Freq
     10138700UL, 10138700UL, //  30m - QRP QRSS, WSPR and PropNET
     14285000UL, 14285000UL, //  20m - QRP SSB Calling Freq
     18130000UL, 18130000UL, //  17m - QRP SSB Calling Freq
     21385000UL, 21385000UL, //  15m - QRP SSB Calling Freq
     24950000UL, 24950000UL, //  12m - QRP SSB Calling Freq
     28385000UL, 28385000UL, //  10m - QRP SSB Calling Freq
   //50200000UL, 50200000UL, //   6m - QRP SSB Calling Freq
   };
byte sideBandModeCache[BANDS*2] = {0};

// ERB - Buffers that Stores "const stings" to, and Reads from FLASH Memory via P()
char buf[PBUFSIZE];  // Note: PBUFSIZE must be set in A1Main.h


// ###############################################################################
// ###############################################################################
// ###############################################################################
// ###############################################################################

// ###############################################################################
/* display routines */
void printLineXY(byte col, byte row, char const *c) {   
    char lbuf[LCD_COL+2];
    
    //snprintf(lbuf, sizeof(lbuf)-col, "%s", c);
    strncpy(lbuf, c, sizeof(lbuf)-col);
    lcd.setCursor(col, row);
    lcd.print(lbuf);
}

void printLine1(char const *c){
    printLineXY(0, 0, c);
}

void printLine2(char const *c){
    printLineXY(0, 1, c);
}

// Print LCD Line1 with Clear to End of Line
void printLine1CEL(char const *c){
    char buf[16];   
    char lbuf[LCD_COL+2];
    
    sprintf(lbuf, P(LCD_STR_CEL), c);
    printLineXY(0, 0, lbuf);
}

// Print LCD Line2 with Clear to End of Line
void printLine2CEL(char const *c){
    char buf[16];  
    char lbuf[LCD_COL+2];
    
    sprintf(lbuf, P(LCD_STR_CEL), c);
    printLineXY(0, 1, lbuf);
}


// ###############################################################################
void updateDisplay(){
  char const *vfoStatus[] = { "ERR", "RDY", "BIG", "SML" };
  char d[6]; // Buffer for RIT Display Value
  char *vfoLabel;
  
  if (refreshDisplay) {
      refreshDisplay = 0; 
      blinkCount = 0;
      
      // Create Label for Displayed VFO
      vfoLabel = vfoActive == VFO_A ?  P2("A") : P2("B");
      if (AltTxVFO)  *vfoLabel += 32; // Convert to Lower Case
      if (editIfMode) vfoLabel = isLSB ? P2("L") : P2("U"); // Replace with IF Freq VFO
      
      // Top Line of LCD
      sprintf(d, P("%+03.3d"), ritVal);  
      sprintf(b, P("%08ld"), frequency);
      sprintf(c, P("%1s:%.2s.%.6s%4.4s%s"), vfoLabel,
          b,  b+2,
          inTx ? P4(" ") : ritOn ? d : P4(" "),
          tune2500Mode ? P8("*"): P8(" ")
          );
      printLine1CEL(c);
      
      saveCursor(11 - (cursorDigitPosition + (cursorDigitPosition>6) ), 0);   
      sprintf(blinkChar, "%1.1s", c+cursorCol);  // Save Character to Blink
      
      sprintf(c, P("%3s%1s %-2s %3.3s"),
          isLSB ? P2("LSB") : P2("USB"),
          sideBandMode > 0 ? P3("*") : P3(" "),
          inTx ? (inPtt ? P4("PT") : P4("CW")) : P4("RX"),
          freqUnStable ? P8(" ") : vfoStatus[vfo->status]
          );
      printLine2CEL(c);
      
  }
  updateCursor();
}


// -------------------------------------------------------------------------------
void saveCursor(int col, int row) {  
  cursorCol = col;
  cursorRow = row;
}


// -------------------------------------------------------------------------------
void cursorOff() {    
  lcd.noBlink();
  lcd.noCursor();
}


// -------------------------------------------------------------------------------
void updateCursor() {updateCursor(800);}

void updateCursor(int blinkRateMS) {
#define DEBUG(x...)
//#define DEBUG(x...) debugUnique(x)    // UnComment for Debug
#define BLINK (75) // ON Percen
#define BLINK_TIMEOUT (5) // In Minutes
  static unsigned long blinkInterval = 0;
  static boolean toggle = false;
    
  if (inTx) return;   // Don't Blink if inTx
  if (ritOn) return;  // Don't Blink if RIT is ON
  if (freqUnStable) return;  // Don't Blink if Frequency is UnStable
  
  DEBUG(P("\nStart Blink"));
  if (blinkInterval < millis()) { // Wink OFF
      DEBUG(P("Wink OFF"));
      blinkInterval = millis() + blinkRateMS;
      lcd.setCursor(cursorCol, cursorRow); // Postion Cursor 
      lcd.print(P(" ")); 
      toggle = true;
  } 
  else if ((blinkInterval - (blinkRateMS/100*BLINK)) < millis() && toggle) { // Wink ON
      DEBUG(P("Wink ON, %d %d"), (BLINK_TIMEOUT * 600/BLINK * 10), blinkCount);
      toggle = !toggle;
      lcd.setCursor(cursorCol, cursorRow); // Postion Cursor 
      lcd.print(blinkChar);
      if (++blinkCount > (BLINK_TIMEOUT * 600/BLINK * 10)) { // Stop Blink after Idle period, Minutes * 6000/BLINK 
          DEBUG(P("End Blink TIMED OUT"));
          cursorDigitPosition = 0;
          refreshDisplay++;
          updateDisplay();
      }
  }
  return;
}

// ###############################################################################
void decodeSideband(){

  if (editIfMode) return;    // Do Nothing if in Edit-IF-Mode

  switch(sideBandMode) {
    case  AUTO_SIDEBAND_MODE: isLSB = (frequency < 10000000UL) ? 1 : 0 ; break; // Automatic Side Band Mode
    case UPPER_SIDEBAND_MODE: isLSB = 0; break; // Force USB Mode
    case LOWER_SIDEBAND_MODE: isLSB = 1; break; // Force LSB Mode    
  }
  setSideband();
}

// -------------------------------------------------------------------------------
void setSideband(){  
  pinMode(LSB, OUTPUT);
  digitalWrite(LSB, isLSB);
}

 
// ###############################################################################
void setBandswitch(unsigned long freq){ 

  if (editIfMode) return;    // Do Nothing if in Edit-IF-Mode

  if (freq >= 15000000UL) digitalWrite(BAND_HI_PIN, 1);
  else digitalWrite(BAND_HI_PIN, 0);
}


// ###############################################################################
void readTuningPot(){
    
    tuningPosition = analogRead(ANALOG_TUNING);
}



#ifdef USE_ALTERNATE_TUNING

// ###############################################################################
// An Alternate Tuning Strategy or Method
// This method somewhat emulates a normal Radio Tuning Dial
// Tuning Position by Switches on FN Circuit
// Author: Eldon R. Brown - WA0UWH, Apr 25, 2014
void checkTuning() {
#define AUTOTIMER_RATE_MS (200)
  long deltaFreq;
  unsigned long newFreq;
  static unsigned long AutoTimer = 0;

  // Count Down to Freq Stable, i.e. Freq has not changed recently
  if (freqUnStable && freqUnStable < 5) refreshDisplay++;
  freqUnStable = max(--freqUnStable, 0);
  
  // Do Not Change Freq while in Transmit or button opperation
  // Allow Tuning knob to be recentered without changing Frequency
  if (tuningLocked) {
      tuningPositionPrevious = tuningPosition;
      return;
  }
  
  // Compute tuningDaltaPosition from tuningPosition
  tuningPositionDelta = tuningPosition - tuningPositionPrevious;
  
  tuningDir = 0;  // Set Default Tuning Directon to neather Right nor Left
  
  if (AutoTimer > millis()) return;
  
  if (tuningPosition < DEAD_ZONE * 2 && AutoTimer < millis()) { // We must be at the Low end of the Tuning POT
      tuningPositionDelta = -DEAD_ZONE;
      AutoTimer = millis() + AUTOTIMER_RATE_MS;
      if (tuningPosition > DEAD_ZONE ) AutoTimer += AUTOTIMER_RATE_MS*3/2; // At very end of Tuning POT
  }
  if (tuningPosition > 1023 - DEAD_ZONE * 2 && AutoTimer < millis()) { // We must be at the High end of the Tuning POT
      tuningPositionDelta = +DEAD_ZONE;
      AutoTimer = millis() + AUTOTIMER_RATE_MS;
      if (tuningPosition  < 1023 - DEAD_ZONE / 8) AutoTimer += AUTOTIMER_RATE_MS*3/2; // At very end of Tuning POT
  }
  
  // Check to see if Digit Change Action is Required, Otherwise Do Nothing via RETURN 
  if (abs(tuningPositionDelta) < DEAD_ZONE) return;

  tuningDir = tuningPositionDelta < 0 ? -1 : tuningPositionDelta > 0 ? +1 : 0;  
  if (!tuningDir) return;  // If Neather Direction, Abort
  
  // Decode and implement RIT Tuning
  if (ritOn) {
      ritVal += tuningDir * 10;
      ritVal = constrain(ritVal, -990, +990);
      tuningPositionPrevious = tuningPosition; // Set up for the next Iteration
      refreshDisplay++;
      updateDisplay();
      return;
  }
  
  if (cursorDigitPosition < 1) return; // Nothing to do here, Abort, Cursor is in Park position

  // Select Tuning Mode; Digit or 2500 Step Mode
  if (tune2500Mode) {
      // Inc or Dec Freq by 2.5K, useful when tuning between SSB stations
      cursorDigitPosition = 3;
      deltaFreq += tuningDir * 2500;
      
      newFreq = (frequency / 2500) * 2500 + deltaFreq;
  }
  else {
      // Compute deltaFreq based on current Cursor Position Digit
      deltaFreq = tuningDir;
      for (int i = cursorDigitPosition; i > 1; i-- ) deltaFreq *= 10;
  
      newFreq = frequency + deltaFreq;  // Save Least Digits Mode
  }
  
  //newFreq = (frequency / abs(deltaFreq)) * abs(deltaFreq) + deltaFreq; // Zero Lesser Digits Mode 
  if (newFreq != frequency) {
      // Update frequency if within range of limits, 
      // Avoiding Nagative underRoll of UnSigned Long, and over-run MAX_FREQ  
      if (newFreq <= MAX_FREQ) {
        frequency = newFreq;
        if (!editIfMode) vfoActive == VFO_A ? vfoA = frequency : vfoB = frequency;
        refreshDisplay++;
      }
      freqUnStable = 100; // Set to UnStable (non-zero) Because Freq has been changed
      tuningPositionPrevious = tuningPosition; // Set up for the next Iteration
  }
}

#else
void checkTuning(){
  int tuningPot = tuningPosition - 512;
  if (-50 < tuningPot && tuningPot < 50){
    //we are in the middle, so, let go of the lock
    if (tuningLocked)
      tuningLocked = 0;
    delay(50);
    return;
  }

  //if the tuning is locked and we are outside the safe band, then we don't move the freq.
  if (tuningLocked)
    return;

  //dead region between -100 and 100
  if (tuningPot > 100){
    if (tuningPot < 150)
      frequency += 10;
    else if (tuningPot < 200)
      frequency += 30;
    else if (tuningPot < 250)
      frequency += 100;
    else if (tuningPot < 300)
      frequency += 300;
    else if (tuningPot < 350)
      frequency += 1000;
    else if (tuningPot < 400)
      frequency += 3000;
    else if (tuningPot < 450){
      frequency += 100000;
      updateDisplay();
      delay(300);
    }
    else if (tuningPot < 500){
      frequency += 1000000;
      updateDisplay();
      delay(300);
    }
  }

  if (-100 > tuningPot){
    if (tuningPot > -150)
      frequency -= 10;
    else if (tuningPot > -200)
      frequency -= 30;
    else if (tuningPot > -250)
      frequency -= 100;
    else if (tuningPot > -300)
      frequency -= 300;
    else if (tuningPot > -350)
      frequency -= 1000;
    else if (tuningPot > -400)
      frequency -= 3000;
    else if (tuningPot > -450){
      frequency -= 100000;
      updateDisplay();
      delay(300);
    }
    else if (tuningPot > -500){
      frequency -= 1000000;
      updateDisplay();
      delay(300);
    }
  }
  delay(50);
  refreshDisplay++;
}

#endif


// ###############################################################################
int inBandLimits(unsigned long freq){
#define DEBUG(x...)
//#define DEBUG(x...) debugUnique(x)    // UnComment for Debug
    //static unsigned long freqPrev = 0;
    //static byte bandPrev = 0;
    int upper, lower = 0;
    
       if (AltTxVFO) freq = (vfoActive == VFO_A) ? vfoB : vfoA;
       DEBUG(P("%s %d: A,B: %lu, %lu, %lu"), __func__, __LINE__, freq, vfoA, vfoB);
       
       //if (freq == freqPrev) return bandPrev;
       //freqPrev = freq;
       
       for (int band = 0; band < BANDS; band++) {
         lower = band * 2;
         upper = lower + 1;
         if (freq >= pgm_read_dword(&bandLimits[lower]) &&
             freq <= pgm_read_dword(&bandLimits[upper]) ) {
             band++;
             //bandPrev = band;
             DEBUG(P("In Band %d"), band);
             return band;
             }
       }
       DEBUG(P("Out Of Band"));
       return 0;
}

  
// ###############################################################################
void toggleAltVfo(int mode) {
#define DEBUG(x...)
//#define DEBUG(x...) debugUnique(x)    // UnComment for Debug
    
    DEBUG(P("%s %d: A,B: %lu, %lu"), __func__, __LINE__, vfoA, vfoB);
    vfoActive = (vfoActive == VFO_A) ? VFO_B : VFO_A;
    frequency = (vfoActive == VFO_A) ? vfoA : vfoB;
    DEBUG(P("%s %d: %s %lu"), __func__, __LINE__, vfoActive == VFO_A ? P2("A:"): P2("B:"), frequency);
    refreshDisplay++;
}
      
// ###############################################################################
void checkTX() {
#define DEBUG(x ...)
//#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug

    if (freqUnStable) return;  // Do Nothing if Freq is UnStable
    if (editIfMode) return;    // Do Nothing if in Edit-IF-Mode
    
    // DEBUG(P("%s %d:  Start Loop"), __func__, __LINE__);
    
    //if we have keyup for a longish time while in CW and PTT tx mode
    if (inTx && cwTimeout < millis()) {
        DEBUG(P("%s %d: TX to RX"), __func__, __LINE__);
        //Change the radio back to receive
        changeToReceive();
        inTx = inPtt = cwTimeout = tuningLocked = 0;
        if (AltTxVFO) toggleAltVfo(inTx);  // Clear Alt VFO if needed
        refreshDisplay++;
        return;
    }
  
    if (!keyDown && isKeyNowClosed()) { // New KeyDown
        if (!inBandLimits(frequency)) return; // Do nothing if TX is out-of-bounds
        DEBUG(P("\nFunc: %s %d: Start KEY Dn"), __func__, __LINE__);
        if (!inTx){
            //put the  TX_RX line to transmit
            changeToTransmit();
            if (AltTxVFO) toggleAltVfo(inTx); // Set Alt VFI if Needed
            refreshDisplay++;
            //give the T/R relays a few ms to settle
            delay(50);
        }
        inTx = keyDown = tuningLocked = 1;
        startSidetone(); //start the side-tone
        cwTimeout = CW_TIMEOUT + millis(); // Start the timer the key is down
        return;
    }
    
    //if (keyDown && analogRead(ANALOG_KEYER) > 150) { //if we have a keyup
    if (keyDown && isKeyNowOpen()) { //if we have a keyup
        DEBUG(P("%s %d: KEY Up"), __func__, __LINE__);
        keyDown = 0;
        stopSidetone(); // stop the side-tone
        cwTimeout = CW_TIMEOUT + millis(); // Start timer for KeyUp Holdoff
        return;
    } 
    
    if (keyDown) {
        DEBUG(P("%s %d: KEY On"), __func__, __LINE__);
        blinkCount = 0;
        cwTimeout = CW_TIMEOUT + millis(); // Restat timer
        return;
    } 
      
    if (inPtt && inTx) { // Check PTT
        DEBUG(P("%s %d: PTT"), __func__, __LINE__);
        blinkCount = 0;
        cwTimeout = CW_TIMEOUT + millis(); // Restat timer
        // It is OK, to stop TX
        if (!isPttPressed()) { // Is PTT Not pushed, then end PTT
            DEBUG(P("%s %d: Stop PTT"), __func__, __LINE__);
            inPtt = cwTimeout = 0;
        }
        return;       
    } 
    
    if (cwTimeout < millis() && !inTx) { // Check PTT
        DEBUG(P("%s %d: RX Idle"), __func__, __LINE__);
        // It is OK, to go into TX
        if (isPttPressed()) { 
            if (!inBandLimits(frequency)) return; // Do nothing if TX is out-of-bounds 
            DEBUG(P("\nFunc: %s %d: Start PTT"), __func__, __LINE__); 
            if (AltTxVFO) toggleAltVfo(inTx); // Set Alt VFO if Needed
            inTx = inPtt = tuningLocked = 1;
            delay(50);
            cwTimeout = CW_TIMEOUT + millis(); // Restat timer
            refreshDisplay++;
        }
        return;      
    }
    // Else, must be running out the cwTimeout
    DEBUG(P("%s %d: Holdiing"), __func__, __LINE__);
}

// -------------------------------------------------------------
int isPttPressed() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    return !digitalRead(TX_RX); // Is PTT pushed  
}
void changeToReceive() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    stopSidetone();
    pinMode(TX_RX, OUTPUT); digitalWrite(TX_RX, 1); //set the TX_RX pin back to input mode
    pinMode(TX_RX, INPUT);  digitalWrite(TX_RX, 1); // With pull-up!
}

void changeToTransmit() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    pinMode(TX_RX, OUTPUT);
    digitalWrite(TX_RX, 0);
}

int isKeyNowClosed() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    return analogRead(ANALOG_KEYER) < 50;
}

int isKeyNowOpen() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    return analogRead(ANALOG_KEYER) > 150;
}

void startSidetone() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    digitalWrite(CW_KEY, 1); // start the side-tone
}

void stopSidetone() {
    DEBUG(P("\nFunc: %s %d"), __func__, __LINE__);
    pinMode(CW_KEY, OUTPUT);
    digitalWrite(CW_KEY, 0); // stop the side-tone
}


// ###############################################################################
int btnDown(){
#define DEBUG(x ...)
#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug
  int val = -1, val2 = -2;
  
  val = analogRead(FN_PIN);
  while (val != val2) { // DeBounce Button Press
      delay(10);
      val2 = val;
      val = analogRead(FN_PIN);
  }
  
  if (val>1000) return 0;
  
  tuningLocked = 1; // Holdoff Tuning until button is processed
  // 47K Pull-up, and 4.7K switch resistors,
  // Val should be approximately = (btnN×4700)÷(47000+4700)×1023

  DEBUG(P("%s %d: btn Val= %d"), __func__, __LINE__, val);

  if (val > 350) return 7;
  if (val > 300) return 6;
  if (val > 250) return 5;
  if (val > 200) return 4;
  if (val > 150) return 3;
  if (val >  50) return 2;
  return 1;
}


// -------------------------------------------------------------------------------
void deDounceBtnRelease() {
  int i = 2;
    
    while (i--) { // DeBounce Button Release, Check twice
      while (btnDown()) delay(20);
    }
    // The following allows the user to re-center the
    // Tuning POT during any Key Press-n-hold without changing Freq.
    readTuningPot();
    tuningPositionPrevious = tuningPosition;
    tuningLocked = 0; // Allow Tuning to Proceed
}


// ###############################################################################
void checkButton() {
#define DEBUG(x ...)
//#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug
  int btn;
  char buf[PBUFSIZE]; // A Local buf, used to pass mesg's to send messages
  
  if (inTx) return;    // Do Nothing if in TX-Mode
  
  btn = btnDown();
  if (btn) DEBUG(P("%s %d: btn %d"), __func__, __LINE__, btn);
  
  switch (btn) {
    case 0: return; // Abort
    case FN_BTN: decodeFN(btn); break;  
    case LT_CUR_BTN: decodeMoveCursor(+1); break;    
    case RT_CUR_BTN: decodeMoveCursor(-1); break;
    case LT_BTN: switch (getButtonPushMode(btn)) { 
            case MOMENTARY_PRESS: decodeSideBandMode(btn); break;
            case DOUBLE_PRESS:    eePromIO(EEP_SAVE); break;
            case LONG_PRESS:      eePromIO(EEP_LOAD); break;
            case ALT_PRESS_FN:    toggleAltTxVFO();  break;
            case ALT_PRESS_LT:    sendMorseMesg(CW_WPM, P(CW_MSG1));  break;
            case ALT_PRESS_RT:    sendMorseMesg(CW_WPM, P(CW_MSG2));  break;    
            default: return; // Do Nothing
            } break;
    case UP_BTN: decodeBandUpDown(+1); break; // Band Up
    case DN_BTN: decodeBandUpDown(-1); break; // Band Down
    case RT_BTN: switch (getButtonPushMode(btn)) { 
            case MOMENTARY_PRESS: decodeTune2500Mode(); break;
            case LONG_PRESS:      decodeEditIf(); break;
            case ALT_PRESS_LT:    sendQrssMesg(QRSS_DIT_TIME, QRSS_SHIFT, P(QRSS_MSG1));  break;
            case ALT_PRESS_RT:    sendQrssMesg(QRSS_DIT_TIME, QRSS_SHIFT, P(QRSS_MSG2));  break;    
            default: return; // Do Nothing
            } break;
    //case 8: decodeAux(8); break; // Report Un-Used as AUX Buttons
    default: return;
  }     
  refreshDisplay++;
  updateDisplay();
  deDounceBtnRelease(); // Wait for Button Release
}


// ###############################################################################
void toggleAltTxVFO() {
    
    if (editIfMode) return; // Do Nothing if in Edit-IF-Mode 
    AltTxVFO = !AltTxVFO;
}

// ###############################################################################
void decodeEditIf() {  // Set the IF Frequency
    static int vfoActivePrev = VFO_A;
    static boolean sbActivePrev;

    if (editIfMode) {  // Save IF Freq, Reload Previous VFO
        frequency += ritVal;
        isLSB ? iFreqLSB = frequency : iFreqUSB = frequency;
        isLSB = sbActivePrev;
        frequency = (vfoActivePrev == VFO_A) ? vfoA : vfoB;
    }
    else {  // Save Current VFO, Load IF Freq 
        vfoActive == VFO_A ? vfoA = frequency : vfoB = frequency;
        vfoActivePrev = vfoActive;
        sbActivePrev = isLSB;
        frequency = isLSB ? iFreqLSB : iFreqUSB;
    }
    editIfMode = !editIfMode;  // Toggle Edit IF Mode
    tune2500Mode = 0;
    ritOn = ritVal = 0;
}


// ###############################################################################
void decodeTune2500Mode() {
    
    if (editIfMode) return; // Do Nothing if in Edit-IF-Mode   
    if (ritOn) return; // Do Nothing if in RIT Mode
    
    cursorDigitPosition = 3; // Set default Tuning Digit
    tune2500Mode = !tune2500Mode;
    if (tune2500Mode) frequency = (frequency / 2500) * 2500;
}


// ###############################################################################
void decodeBandUpDown(int dir) {
    int j;
    
   if (editIfMode) return; // Do Nothing if in Edit-IF-Mode
      
    if(dir > 0) {  // For Band Change, Up
       for (int i = 0; i < BANDS; i++) {
         j = i*2 + vfoActive;
         if (frequency <= pgm_read_dword(&bandLimits[i*2+1])) {
           if (frequency >= pgm_read_dword(&bandLimits[i*2])) {
             // Save Current Ham frequency and sideBandMode
             freqCache[j] = frequency;
             sideBandModeCache[j] = sideBandMode;
           }
           // Load From Next Cache Up Band
           j += 2;
           frequency = freqCache[min(j,BANDS*2-1)];
           sideBandMode = sideBandModeCache[min(j,BANDS*2-1)];
           vfoActive == VFO_A ? vfoA = frequency : vfoB = frequency;
           break;
         }
       }
     } // End fi
     
     else { // For Band Change, Down
       for (int i = BANDS-1; i > 0; i--) {
         j = i*2 + vfoActive;
         if (frequency >= pgm_read_dword(&bandLimits[i*2])) {
           if (frequency <= pgm_read_dword(&bandLimits[i*2+1])) {
             // Save Current Ham frequency and sideBandMode
             freqCache[j] = frequency;
             sideBandModeCache[j] = sideBandMode;
           }
           // Load From Next Cache Down Band
           j -= 2;
           frequency = freqCache[max(j,vfoActive)];
           sideBandMode = sideBandModeCache[max(j,vfoActive)];
           vfoActive == VFO_A ? vfoA = frequency : vfoB = frequency;
           break;
         }
       }
     } // End else
     
   freqUnStable = 100; // Set to UnStable (non-zero) Because Freq has been changed
   ritOn = ritVal = 0;
   decodeSideband();
}


// ###############################################################################
void decodeSideBandMode(int btn) {
#define DEBUG(x ...)
//#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug

    DEBUG(P("\nCurrent, isLSB %d"), isLSB);
    if (editIfMode) { // Switch Sidebands
        frequency += ritVal;
        ritVal = 0;
        isLSB ? iFreqLSB = frequency : iFreqUSB = frequency;
        isLSB = !isLSB;
        frequency = isLSB ? iFreqLSB : iFreqUSB;
        setSideband();
    }
    else {
        sideBandMode++;
        sideBandMode %= 3; // Limit to Three Modes
        decodeSideband();
    }

    DEBUG(P("Toggle, isLSB %d"), isLSB);
    refreshDisplay++;
    updateDisplay();
}


// ###############################################################################
void decodeMoveCursor(int dir) {

      tuningPositionPrevious = tuningPosition;
      if (tune2500Mode) { tune2500Mode = 0; return; } // Abort tune2500Mode if Cursor Button is pressed
      cursorDigitPosition += dir;
      cursorDigitPosition = constrain(cursorDigitPosition, 0, 7);
      freqUnStable = 0;  // Set Freq is NOT UnStable, as it is Stable     
      refreshDisplay++;
}

// -------------------------------------------------------------------------------
void decodeAux(int btn) {
    
    //debug("%s btn %d", __func__, btn);
    sprintf(c, P("Btn: %.2d"), btn);
    printLine2CEL(c);
    delay(100);
    deDounceBtnRelease(); // Wait for Release
}


// ###############################################################################
int getButtonPushMode(int btn) {
    int t1, t2, tbtn;
  
    t1 = t2 = 0;

    // Time for first press
    tbtn = btnDown();
    while (t1 < 20 && btn == tbtn){
        tbtn = btnDown();
        if (btn != FN_BTN     && tbtn == FN_BTN)     return ALT_PRESS_FN;
        if (btn != LT_CUR_BTN && tbtn == LT_CUR_BTN) return ALT_PRESS_LT;
        if (btn != RT_CUR_BTN && tbtn == RT_CUR_BTN) return ALT_PRESS_RT;
        delay(50);
        t1++;
    }
    // Time between presses
    while (t2 < 10 && !tbtn){
        tbtn = btnDown();
        if (btn != FN_BTN     && tbtn == FN_BTN)     return ALT_PRESS_FN;
        if (btn != LT_CUR_BTN && tbtn == LT_CUR_BTN) return ALT_PRESS_LT;
        if (btn != RT_CUR_BTN && tbtn == RT_CUR_BTN) return ALT_PRESS_RT;
        delay(50);
        t2++;
    }

    if (t1 > 10) return LONG_PRESS;
    if (t2 < 7) return DOUBLE_PRESS; 
    return MOMENTARY_PRESS; 
}


// ###############################################################################
void decodeFN(int btn) {

  switch (getButtonPushMode(btn)) { 
    case MOMENTARY_PRESS:
       ritOn = !ritOn; ritVal = 0;     
       refreshDisplay++;
       updateDisplay();
       break;
      
    case DOUBLE_PRESS:
       if (editIfMode) { // Abort Edit IF Mode, Reload Active VFO
          editIfMode = false;
          frequency = (vfoActive == VFO_A) ? vfoA : vfoB; break;
       } 
       else { // Save Current VFO, Load Other VFO
          if (vfoActive == VFO_A) {
            vfoA = frequency;
            vfoActive = VFO_B;
            frequency = vfoB;
          } 
          else {
            vfoB = frequency;
            vfoActive = VFO_A;
            frequency = vfoA;
          }
       }
       ritOn = ritVal = 0;     
       refreshDisplay++;
       updateDisplay();
       break;
      
    case LONG_PRESS:
       if (editIfMode) return; // Do Nothing if in Edit-IF-Mode
       switch (vfoActive) {
       case VFO_A :
          vfoB = frequency + ritVal;
          sprintf(c, P("A%sB"), ritVal ? P2("+RIT>"): P2(">"));
          printLine2CEL(c);
          break;
       default :
          vfoA = frequency + ritVal;
          sprintf(c, P("B%sA"), ritVal ? P2("+RIT>"): P2(">"));
          printLine2CEL(c);
          break;
       }
       delay(100);
       break;
    default :
       return; // Do Nothing
  }
  deDounceBtnRelease(); // Wait for Release
}


// ###############################################################################
void setFreq(unsigned long freq) {

    if (!inTx && ritOn) freq += ritVal;
    freq += isLSB ? iFreqLSB : iFreqUSB;
    vfo->setFrequency(freq);
}


// ###############################################################################
// ###############################################################################
// ###############################################################################
// ###############################################################################

// ###############################################################################
void setup() {
#define DEBUG(x ...)  // Default to NO debug    
//#define DEBUG(x ...) debugUnique(x)    // UnComment for Debug
  
  // Initialize the Serial port so that we can use it for debugging
  Serial.begin(115200);
  debug(P("%s Radiono - Rev: %s"), __func__, P2(RADIONO_VERSION));

#ifdef USE_PCA9546
  // Initialize the PCA9546 multiplexer and select channel 1 
  mux = new PCA9546(PCA9546_I2C_ADDRESS, PCA9546_CHANNEL_1);
  if (mux->status == PCA9546_ERROR)
  {
    printLine2(P("PCA9546 init error"));
    delay(3000);
  }
#endif

  lcd.begin(LCD_COL, LCD_ROW);
  cursorOff();
  printLine1(P("Farhan - Minima"));
  printLine2(P("  Tranceiver"));
  delay(2000);
  
  sprintf(b, P("Radiono %s"), P2(RADIONO_VERSION));
  printLine1CEL(b);
  
  sprintf(b, P("Rev: %s"), P2(INC_REV));
  printLine2CEL(b);
  delay(2000);
  
  //sprintf(b, P("%s"), __DATE__); // Compile Date and Time
  //sprintf(c, "%3.3s%7.7s %5.5s", b, b+4, __TIME__);
  //printLine2CEL(c);
  //delay(2000); 
  
  // Print just the File Name, Added by ERB
  //sprintf(c, P("F: %-13.13s"), P2(__FILE__));
  //printLine2CLE(c);
  //delay(2000);


  // The library automatically reads the factory calibration settings of your Si570
  // but it needs to know for what frequency it was calibrated for.
  // Looks like most HAM Si570 are calibrated for 56.320 Mhz.
  // If yours was calibrated for another frequency, you need to change that here
  vfo = new Si570(SI570_I2C_ADDRESS, 56320000);

  if (vfo->status == SI570_ERROR) {
    // The Si570 is unreachable. Show an error for 3 seconds and continue.
    printLine2CEL(P("Si570 comm error"));
    delay(3000);
  }
  
  printLine2CEL(P(" "));
 
  // This will print some debugging info to the serial console.
  vfo->debugSi570();

  // Setup the initial frequency
  vfo->setFrequency(frequency);
 
  // Setup with No SideTone
  stopSidetone();

  // Setup in Receive Mode
  changeToReceive();
  
  // Setup to read Tuning POT
  pinMode(ANALOG_TUNING, INPUT);
  digitalWrite(ANALOG_TUNING, 1); //old way to enable the built-in pull-ups
  
  // Setup to read Buttons
  pinMode(FN_PIN, INPUT);
  #ifdef USE_MULTIPLEXED_BUTTONS
  digitalWrite(FN_PIN, 0); // Use an external pull-up of 47K ohm to AREF
  #else
  digitalWrite(FN_PIN, 1); 
  #endif
  
  DEBUG(P("Pre Load EEPROM"));
  loadUserPerferences();
  
  // Setup the First Tuning POT Position
  tuningPositionPrevious = tuningPosition = analogRead(ANALOG_TUNING);
  refreshDisplay++; 
}


// ###############################################################################
// ###############################################################################
void loop(){
  unsigned long freq;
  
  readTuningPot();
  checkTuning();

  checkTX();
  
  checkButton();

  if (editIfMode) {  // Set freq to Current Dial Trail IF Freq + VFO - Prev IF Freq
      freq = frequency;
      if (ritOn) freq += ritVal;
      freq += (vfoActive == VFO_A) ? vfoA : vfoB;
      vfo->setFrequency(freq);
  } else setFreq(frequency);
  
  decodeSideband();
  setBandswitch(frequency);
  setRf386BandSignal(frequency);
  
  updateDisplay();
   
}

// ###############################################################################

//End

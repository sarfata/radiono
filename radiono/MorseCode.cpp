// MorseCode.cpp
// Send Morse Code


#include <Arduino.h>
#include "A1Main.h"
#include "MorseCode.h"

// Local
long ditLen = 1200/13; // Default Speed
byte txSpeed = 0;
#include "MorseTable.h"


// ########################################################
// ########################################################
// ########################################################


// ########################################################
void bitTimer(unsigned long timeOut) {
    
    timeOut += millis();
    while(timeOut > millis()) {
        if (isKeyNowClosed()) return; // Abort Message
        delay(20);
    }
}
// ########################################################
void sendBit(int mode, int freqShift, int mult) {
    
    switch (mode) {
    case MOD_CW:   startSidetone(); break;
    case MOD_QRSS: setFreq(frequency + freqShift); break;
    }
    
    bitTimer(ditLen * mult);
    
    switch (mode) {
    case MOD_CW:   stopSidetone(); break;
    case MOD_QRSS: setFreq(frequency); break;
    }
}

void dit(int mode, int freqShift) {
    sendBit(mode, freqShift, 1);
}

void dah(int mode, int freqShift) {
    sendBit(mode, freqShift, 3);
}


// ########################################################
void sendMesg(int mode, int freqShift, char *msg) {
    byte bits = 0;
    unsigned long timeOut;
    
    if (AltTxVFO) return; // Macros and Beacons not allowed in Split Mode, for now.
    if (editIfMode) return; // Do Nothing if in Edit-IF-Mode       
    if (!inBandLimits(frequency)) return; // Do nothing if TX is out-of-bounds
    if (isKeyNowClosed()) return; // Abort Message
    
    inTx = 1; 
    changeToTransmit();
    
    printLine2CEL(" "); // Clear Line 2
    sprintf(c, P("%s"), mode == MOD_QRSS ? P8("QR"): P8("CW"));
    if (mode == MOD_QRSS && ditLen < 1000) sprintf(c+1, P(".%2.2d"), ditLen/10);
    else sprintf(c, P("%s%02.2d"), c, txSpeed);
    printLineXY(12, 0, c);
    delay(50);
     
    if(mode == MOD_QRSS) startSidetone();        
    printLine2CEL(msg); // Scroll Message on Line 2
    bitTimer(ditLen);
    
    while(*msg) {
        if (isKeyNowClosed()) return; // Abort Message
        if (*msg == ' ') {
           msg++;
           printLine2CEL(msg); // Scroll Message on Line 2
           bitTimer(ditLen * 4); // 3 for previous character + 4 for word = 7 total
        }
        else {
            bits = pgm_read_byte(&morse[*msg & 0x7F - 32]); // Read Code from FLASH
            while(bits > 1) {
                if (isKeyNowClosed()) return; // Abort Message
                bits & 1 ? dit(mode, freqShift) : dah(mode, freqShift);
                bitTimer(ditLen);
                bits /= 2;
            }
            msg++;
            printLine2CEL(msg); // Scroll Message on Line 2
            bitTimer(ditLen * 2); // 1 + 2 added between characters
        } 
        cwTimeout = ditLen * 10 + millis();
    } // main checkTX() will clean-up and stop Transmit
}

// ########################################################
void sendQrssMesg(long len, int freqShift, char *msg) {
    
    ditLen = len < 0 ? -len : len * 1000; // Len is -MS or +Seconds
    txSpeed = ditLen / 1000;
    sendMesg(MOD_QRSS, freqShift, msg);
}

// ########################################################
void sendMorseMesg(int wpm, char *msg) {
    
    ditLen = int(1200 / wpm);
    txSpeed = wpm; 
    sendMesg(MOD_CW, 0, msg);  
}

// End
  


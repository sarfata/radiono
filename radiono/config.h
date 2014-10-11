#ifndef CONFIG_H

#define CONFIG_H

#define RADIONO_VERSION "0.5" // Modifications by: Eldon R. Brown - WA0UWH
//#define INC_REV "ko7m-AC"         // Incremental Rev Code
//#define INC_REV "ERB_FR"          // Incremental Rev Code
#define INC_REV "kk6aht"          // Incremental Rev Code

//#define USE_PCA9546	1         // Define this symbol to include PCA9546 support
//  #define PCA9546_I2C_ADDRESS 0x70
//#define USE_I2C_LCD	1         // Define this symbol to include i2c LCD support
//#define USE_MULTIPLEXED_BUTTONS 1 // Define this symbol to use a series of cascaded buttons+pullup
//#define USE_ALTERNATE_TUNING 1  // Define this to use a classic radio tuning where buttons are used to move active digit
                                  // You need the USE_MULTIPLEXED_BUTTONS and multiple buttons for this

#define LCD_COL (16)
#define LCD_ROW (2)
//#define LCD_COL (20)
//#define LCD_ROW (4)
#define LCD_STR_CEL "%-16.16s"
//#define LCD_STR_CEL "%-20.20s"  // For 20 Character LCD Display

#define SI570_I2C_ADDRESS   0x55

// USB and LSB IF frequencies
#define IF_FREQ_USB   (19997000L)
#define IF_FREQ_LSB   (19992000L)

#define CW_TIMEOUT (600L) // in milliseconds, this is the parameter that determines how long the tx will hold between cw key downs

#define MAX_FREQ (30000000UL)
#define DEAD_ZONE (40)

// Pin Number for the digital output controls
#define LSB (2)
#define TX_RX (3)
#define CW_KEY (4)

// Pin Numbers for analog inputs
#define FN_PIN (A3)
#define ANALOG_TUNING (A2)
#define ANALOG_KEYER (A1)

#endif


#ifndef PCA9546_H
#define PCA9546_h

#ifndef P
#define PBUFSIZE (66)
extern char buf[PBUFSIZE];

#define  P(x) strcpy_P(buf, PSTR(x))
#endif

typedef enum 
{
  PCA9546_ERROR = 0,
  PCA9546_SUCCESS
} PCA9546_Status;

#define PCA9546_NOCHANNEL (0)   // No channel selected
#define PCA9546_CHANNEL_1 (1)	// Bit 1
#define PCA9546_CHANNEL_2 (2)	// Bit 2
#define PCA9546_CHANNEL_3 (4)	// Bit 3
#define PCA9546_CHANNEL_4 (8)	// Bit 4

class PCA9546
{
public:
  PCA9546(uint8_t i2c_address);
  PCA9546(uint8_t i2c_address, uint8_t channel);
  bool selectChannel(uint8_t channel);
  
  PCA9546_Status status;
  uint8_t channel;

private:
  uint8_t i2c_address;
  uint8_t i2c_read();
  void i2c_write(uint8_t data);
};

#endif

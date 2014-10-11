#include <arduino.h>
#include "config.h"
#include "debug.h"

static int s_tuningPosition = 0;
static int s_tuningPositionPrevious = 0;
static boolean s_tuningLocked = false;


/*
 * This should be called once when the program starts.
 */
void tuning_setup() {
  // Turn the potentiometer pin into an input and enable the internal pull-up
  pinMode(ANALOG_TUNING, INPUT);
  digitalWrite(ANALOG_TUNING, 1);

  s_tuningPosition = s_tuningPositionPrevious = analogRead(ANALOG_TUNING);
}

/*
 * Call this function to lock tuning.
 */
void tuning_lock(void) {
  s_tuningLocked = true;
}

/*
 * Call this function to unlock tuning.
 */
void tuning_unlock(void) {
  s_tuningLocked = false;

  // Make sure unlocking does not cause an immediate move
  s_tuningPositionPrevious = s_tuningPosition;

  debug("Unlocking tuning.");
}

/*
 * This should be called once for each time the main loop is executed.
 * This function reads the current value of the pot and will save it so that
 * it is available for all the other functions.
 * All the other functions depend on this being called only once per loop!
 */
void tuning_loop(){
  // Save previous value so we can calculate the delta if needed.
  s_tuningPositionPrevious = s_tuningPosition;
  s_tuningPosition = analogRead(ANALOG_TUNING);

  // If the tuning is locked, check to see if we are in the center zone
  // and unlock tuning if we are.
  if (s_tuningLocked) {
    int signedPosition = s_tuningPosition - 512;
    if (signedPosition < 50 && signedPosition > -50) {
      debug("Unlocking tuning because pot is back to center.");
      s_tuningLocked = false;
    }
  }
}

/*
 * (Applies only in Default Tuning mode (ie: non-Alternate))
 * This returns the frequency shift requested by the user. This function also
 * applies a delay so that large changes come slower than fast changes.
 * This returns 0 if tuning is locked.
 */
long tuning_frequency_delta(void) {
  static unsigned long timer = 0;

  // This table defines the frequency shift to apply to the VFO based on the
  // potentiometer value (-512 to +512).
  // The values are: { potValue, freqDeltaInHz, optionalDelayInMS }
  static const long frequencyShifts[][3] = {
    { 100,       0,  50 },
    { 150,      10,  50 },
    { 200,      30,  50 },
    { 250,     100,  50 },
    { 300,     300,  50 },
    { 350,    1000,  50 },
    { 400,    3000,  50 },
    { 450,  100000,  50 },
    { 490, 1000000, 300 }
  };
  static const int countFreqShifts = sizeof(frequencyShifts) / (3 * sizeof(long));

  // Check to see if we are under the timer "cease-fire". If yes, we just return 0.
  if (timer > millis()) {
    return 0;
  }

  // Return 0 if tuning is locked
  if (s_tuningLocked) {
    return 0;
  }

  // Go through our table to find out appropriate frequency delta and delay for the
  // current position of the pot.
  long delta = 0;
  int delay = 0;
  int position = s_tuningPosition - 512;
  int absolutePosition = abs(position);
  int i;

  for (i = 0; i < countFreqShifts && absolutePosition > frequencyShifts[i][0]; i++) {
    delta = frequencyShifts[i][1];
    delay = frequencyShifts[i][2];
  }

  // Flip the sign of the frequency delta if needed
  if (position < 0) {
    delta *= -1;
  }

  debug("Position=%i => Delta=%li Delay=%i", position, delta, delay);

  // Set the timer to when the next frequency change can happen
  timer = millis() + delay;

  // Return the amount of frequency change required
  return delta;
}

/*
 * (Applies only to delta-tuning mode)
 * Returns the direction in which the tuning button was moved (-1 / 0 / 1).
 */
int tuning_alternate_direction() {
  static unsigned long timer = 0;

  // Check to see if we are under the timer "cease-fire". If yes, we just return 0.
  if (timer > millis()) {
    return 0;
  }

  // Return 0 if tuning is locked
  if (s_tuningLocked) {
    return 0;
  }

  // Set the timer to not fire for another 200ms
  timer = millis() + 200;

  // Return -1, 1, or 0 depending on the direction of the knob
  int delta = s_tuningPosition - s_tuningPositionPrevious;
  if (delta < 0) {
    return -1;
  } else if (delta > 0) {
    return 1;
  } else {
    return 0;
  }
}

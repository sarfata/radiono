/*
 * This file contains all the function relative to the tuning button.
 *
 * Different implementations exist for different hardware configuration.
 */


void tuning_lock(void);
void tuning_unlock(void);

/* Should be called once on startup */
void tuning_setup();

/* This function should be called once per Arduino-loop() */
void tuning_loop();

/* This is the default Minima tuning system */
long tuning_frequency_delta(void);

/* Alternate mode: Returns in what direction the button is moving */
int tuning_alternate_direction();


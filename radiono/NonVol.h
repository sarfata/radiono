// NonVol.h
// NonVolatile Memory I/O

#ifndef NONVOL_H
#define NONVOL_H

    enum {
        EEP_LOAD = 0,
        EEP_SAVE,
    };
    
    void eePromIO(int mode);
    void loadUserPerferences();

#endif

// End

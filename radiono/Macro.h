// Macro.h

#ifndef MACRO_H
#define MACRO_H

    // Message Macros fragments
    #define CALLID "N0CAL"
    #define    QTH "Someplace, US"
    #define   GRID "CN88xc"
    #define MACRO1 "CQ CQ CQ de " CALLID " " CALLID " "
    
    // Message Macros
    #define CW_MSG1 MACRO1 " " MACRO1 " K"
    #define CW_MSG2 "de " CALLID
    
    // QRSS Info, See: http://www.w0ch.net/qrss/qrss.htm
    #define QRSS_MSG1 " " CALLID " " GRID "   " CALLID " " GRID " "
    #define QRSS_MSG2 " " CALLID " " CALLID " "
    #define QRSS_MSG3 "aeiou AEIOU " // A test Mesg

#endif

// End


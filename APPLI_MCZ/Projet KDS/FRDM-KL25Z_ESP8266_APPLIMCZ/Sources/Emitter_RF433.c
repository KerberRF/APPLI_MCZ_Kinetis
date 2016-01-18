#include "WAIT1.h"
#include "Emitter_RF433.h"
#include "RF_OUT.h"
#include "RF_IN.h"

unsigned char manchesterPinOut = 0;
unsigned char manchesterPinIn = 2;

int manchester_init() {

    RF_OUT_Init();
    RF_IN_Init();
    // pinMode(manchesterPinIn, INPUT);
    // pinMode(manchesterPinOut, OUTPUT);
}

void manchester_send_bit(uint8_t b) {
    if (b) {
    RF_OUT_On();
	WAIT1_Waitus(HALF_BIT);
	RF_OUT_Off();
	WAIT1_Waitus(HALF_BIT);
    } else {
    RF_OUT_Off();
	WAIT1_Waitus(HALF_BIT);
	RF_OUT_On();
	WAIT1_Waitus(HALF_BIT);
    }
}

// send 12-bit number in Manchester code.
void manchester_send(uint16_t t) {
    int mask = 0x800;
//    int ones = 0;
    manchester_send_bit(0); /* bit de preambule */
    WAIT1_Waitus(HALF_BIT);
    //delayMicroseconds(HALF_BIT);
    for (int i = 0; i < 12; ++i) {
    	int bit =  !! (t & mask);
    	manchester_send_bit(bit);
    	mask >>= 1;
//	if (bit == 1) {
//	    ones ++;
//	} else {
//	    ones = 0;
//	}
//	if (ones == 2) {
//	    manchester_send_bit(0);
//	    ones = 0;
//	}
    }
//    if (ones > 0) {
//	manchester_send_bit(0);
//    }
}

/*
 * Emitter_RF433.h
 *
 *  Created on: 14 janv. 2016
 *      Author: rferelloc
 */

#ifndef SOURCES_EMITTER_RF433_H_
#define SOURCES_EMITTER_RF433_H_

#include "CLS1.h"

// in microseconds
/*************** Temps théoriques *********************/
#define HALF_BIT        416  /* 833 us/2 */
#define INTER_TRAME     3702 /* 3,702 ms */
#define DEBUT_COMMANDE  100000 /* 100ms */

/************ Temps issues mesure télécommande MCZ ****/
//#define HALF_BIT        414  /* 829 us/2 */
//#define INTER_TRAME     3712 /* 3,712 ms */
//#define DEBUT_COMMANDE  103000 /* 100ms */

/*********** temps prenant en compte la latence *******/
//#define HALF_BIT        393  /* 829 us/2 */
//#define INTER_TRAME     3712 /* 3,712 ms */
//#define DEBUT_COMMANDE  103000 /* 100ms */

int manchester_init();
void manchester_send(uint16_t t);
//uint16_t manchester_read();

// low level, for debug
//int8_t manchester_wait_bit();
void manchester_send_bit(uint8_t b);
//void manchester_send_trame(int *tab,int n);


#endif /* SOURCES_EMITTER_RF433_H_ */

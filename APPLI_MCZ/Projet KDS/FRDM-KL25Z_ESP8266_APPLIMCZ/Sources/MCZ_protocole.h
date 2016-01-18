/*
 * MCZ_protocole.h
 *
 *  Created on: 14 janv. 2016
 *      Author: rferelloc
 */

#ifndef SOURCES_MCZ_PROTOCOLE_H_
#define SOURCES_MCZ_PROTOCOLE_H_

#include "CLS1.h"

struct s_ParametersOfMCZ {
   int Id;
   int Modes;
   int User;
   int Puissance;
   int Ventilateur1;
   int Ventilateur2;
};

struct s_Octet {
   short int O_MSB_Id;
   short int O_ISB_Id;
   short int O_LSB_Id;
   short int O_Parameter1;
   short int O_Parameter2;
   short int O_MSB_CRC;
   short int O_LSB_CRC;
};

struct s_Packet {
   short int *num_packet;
   int tabPacket[7];
};

#define           poly                  0x1021          /* crc-ccitt mask */
#define           CRC_initial_xmodem    0x0000          /* crc-ccitt initial value for xmodem */
#define           CRC_initial_CCITT     0xFFFF          /* crc-ccitt initial value for standard */

/***************************************************************************
              Initialisation des paramètres MCZ.
/***************************************************************************/
void MCZ_init(void);

/***************************************************************************
              Comparaison des anciennes données avec les nouvelles.
    Entrees : Structure de ParametersOfMCZ
    sortie : 0 ou 1
/***************************************************************************/
void CompartParameters(struct s_ParametersOfMCZ *oldParam, struct s_ParametersOfMCZ *newParam);

/***************************************************************************
              Converti les donnees en 5 octets.
    Entree : Structure de ParametersOfMCZ
    sortie : 5 premiers octets de la trame
/***************************************************************************/
void Convert2Databytes(struct s_ParametersOfMCZ *pParam, struct s_Octet *oct);

/***************************************************************************
              Calcul du CRC-CCITT16.
    Entree : Structure de s_Octet
    sortie : 2 octets de fin de trame contenant le CRC
/***************************************************************************/
void Calcul_Crc(struct s_Octet *oct);
void update_good_crc(unsigned short ch);
void augment_message_for_good_crc();
void update_bad_crc(unsigned short ch);

/***************************************************************************
              Calcul de la parite d'un octet.
    Entree : octet b
    sortie : 1 impaire 0 paire
/***************************************************************************/
int parite_paire(unsigned short nombre);

/***************************************************************************
              Encapsulation des octets en paquets de 12bits.
    Entree : octet "oct"
    sortie : packet 12bits
/***************************************************************************/
void encapsule(struct s_Octet *oct, struct s_Packet *Pack);

/***************************************************************************
              Envoie des trames de données codées en "manchester".
    Entree : packet 12bits et n nombre de repetition

/***************************************************************************/
void manchester_send_trame(struct s_Packet *Pack,int n);

static uint8_t MCZ_send_Cmd(const CLS1_StdIOType *io);

static uint8_t MCZ_PrintHelp(const CLS1_StdIOType *io);
/*!
 * \brief Command line parser routine
 * \param cmd Pointer to command line string
 * \param handled Return value if command has been handled
 * \param io Standard Shell I/O handler
 * \return Error code, ERR_OK for no failure
 */
uint8_t MCZ_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

#endif /* SOURCES_MCZ_PROTOCOLE_H_ */

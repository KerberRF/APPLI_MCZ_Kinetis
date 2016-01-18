/*
 * MCZ_protocole.c
 *
 *  Created on: 14 janv. 2016
 *      Author: rferelloc
 */

#include <stdio.h>
#include <stdlib.h>
#include "Emitter_RF433.h"
#include "WAIT1.h"
#include "RF_OUT.h"
#include "Shell.h"
#include "LEDB.h"
#include "MCZ_protocole.h"

/* global variables */
unsigned short good_crc;
unsigned short bad_crc;
unsigned short text_length;
unsigned short emit = 0;

struct s_ParametersOfMCZ Util_config = {0};/* declaration d'une structure vide */
struct s_ParametersOfMCZ Old_config = {0}; /* declaration d'une structure vide */
struct s_Octet octet = {0};                /* declaration d'une structure vide */
struct s_Packet packet = {0};              /* declaration d'une structure vide */

void MCZ_init(void)
{
    Util_config.Id = 9859842; /* ID fixe */
  	Util_config.Modes = 0; /* par default */
  	Util_config.User = 2; /* User 2 fixe */
  	Util_config.Puissance = 3; /* par default */
  	Util_config.Ventilateur1 = 1; /* par default */
  	Util_config.Ventilateur2 = 1; /* par default */
}

void CompartParameters(struct s_ParametersOfMCZ *oldParam, struct s_ParametersOfMCZ *newParam)
{
      if (oldParam->Modes == newParam->Modes & oldParam->Puissance == newParam->Puissance &
          oldParam->Ventilateur1 == newParam->Ventilateur1 & oldParam->Ventilateur2 == newParam->Ventilateur2 )
         emit = 0;
      else
         emit = 1;

      if (emit == 1){

         //printf("changement \n");
         oldParam->Modes = newParam->Modes;
         oldParam->User = newParam->User;
         oldParam->Puissance = newParam->Puissance;
         oldParam->Ventilateur1 = newParam->Ventilateur1;
         oldParam->Ventilateur2 = newParam->Ventilateur2;
      }

}

/***************************************************************************
              Converti les donnees XML en 5 octets.
    Entree : Structure de ParametersOfMCZ
    sortie : 5 premiers octets de la trame
/***************************************************************************/
void Convert2Databytes(struct s_ParametersOfMCZ *pParam, struct s_Octet *oct)
{

   oct->O_MSB_Id = ((pParam->Id & 0xFF0000)>>16) & 0xFF;
   oct->O_ISB_Id = ((pParam->Id & 0xFF00)>>8) & 0xFF;
   oct->O_LSB_Id = (pParam->Id & 0xFF);
   oct->O_Parameter1 = (pParam->Ventilateur1 <<5 |
					   pParam->Puissance <<2 |
					   pParam->Modes) & 0xFF;
   oct->O_Parameter2 = (pParam->User <<6 | pParam->Ventilateur2 << 3) & 0xFF;
}

/***************************************************************************
              Calcul du CRC-CCITT16.
    Entree : Structure de s_Octet
    sortie : 2 octets de fin de trame contenant le CRC
/***************************************************************************/
void Calcul_Crc(struct s_Octet *oct)
{
    void update_good_crc(unsigned short);
    void augment_message_for_good_crc();
    void update_bad_crc(unsigned short);

    unsigned short i;
    unsigned short data[6];

	data[0] = oct->O_MSB_Id;
	data[1] = oct->O_ISB_Id;
	data[2] = oct->O_LSB_Id;
	data[3] = oct->O_Parameter1;
	data[4] = oct->O_Parameter2;
    good_crc = CRC_initial_xmodem;
    bad_crc = 0xffff;
    i = 0;
    text_length= 0;
	for (i=0; i<5; i++)
    {
        //printf("%02X\n",data[i]);
        update_good_crc(data[i]);
        update_bad_crc(data[i]);
        text_length++;
    }
    augment_message_for_good_crc();
#ifdef DEBUG
     printf("good_CRC = %04X  Bad_CRC = %04X  Length = %u\n",
            good_crc,bad_crc,text_length,data);
#endif /* DEBUG */
    oct->O_MSB_CRC = (good_crc & 0xFF00)>>8;
    oct->O_LSB_CRC = good_crc & 0xFF;
}

void update_good_crc(unsigned short ch)
{
    unsigned short i, v, xor_flag;

    /*
    Align test bit with leftmost bit of the message byte.
    */
    v = 0x80;

    for (i=0; i<8; i++)
    {
        if (good_crc & 0x8000)
        {
            xor_flag= 1;
        }
        else
        {
            xor_flag= 0;
        }
        good_crc = good_crc << 1;

        if (ch & v)
        {
            /*
            Append next bit of message to end of CRC if it is not zero.
            The zero bit placed there by the shift above need not be
            changed if the next bit of the message is zero.
            */
            good_crc= good_crc + 1;
        }

        if (xor_flag)
        {
            good_crc = good_crc ^ poly;
        }

        /*
        Align test bit with next bit of the message byte.
        */
        v = v >> 1;
    }
}

void augment_message_for_good_crc()
{
    unsigned short i, xor_flag;

    for (i=0; i<16; i++)
    {
        if (good_crc & 0x8000)
        {
            xor_flag= 1;
        }
        else
        {
            xor_flag= 0;
        }
        good_crc = good_crc << 1;

        if (xor_flag)
        {
            good_crc = good_crc ^ poly;
        }
    }
}

void update_bad_crc(unsigned short ch)
{
    /* based on code found at
    http://www.programmingparadise.com/utility/crc.html
    */

    unsigned short i, xor_flag;

    /*
    Why are they shifting this byte left by 8 bits??
    How do the low bits of the poly ever see it?
    */
    ch<<=8;

    for(i=0; i<8; i++)
    {
        if ((bad_crc ^ ch) & 0x8000)
        {
            xor_flag = 1;
        }
        else
        {
            xor_flag = 0;
        }
        bad_crc = bad_crc << 1;
        if (xor_flag)
        {
            bad_crc = bad_crc ^ poly;
        }
        ch = ch << 1;
    }
}

/***************************************************************************
              Calcul de la parite d'un octet.
    Entree : octet b
    sortie : 1 impaire 0 paire
/***************************************************************************/
int parite_paire(unsigned short nombre) { //retourne 0 si paire, 1 si impaire
   unsigned int ret=0;

   while(nombre) {
      ret^=nombre & 1;
      nombre>>=1;
   }
   return ret;
}

/***************************************************************************
              Encapsulation des octets en paquets de 12bits.
    Entree : octet "oct"
    sortie : packet 12bits
/***************************************************************************/
void encapsule(struct s_Octet *oct, struct s_Packet *Pack)
{
	unsigned short i, par;
	unsigned short data[7];
    unsigned short first=1;
    int inter;

	data[0] = oct->O_MSB_Id;
	data[1] = oct->O_ISB_Id;
	data[2] = oct->O_LSB_Id;
	data[3] = oct->O_Parameter1;
	data[4] = oct->O_Parameter2;
	data[5] = oct->O_MSB_CRC;
	data[6] = oct->O_LSB_CRC;
	for (i=0; i<7; i++)
    {
        if (first == 1){
          inter = ((data[i] <<1) | 0x1) & 0x1FF;
          par = parite_paire(inter);
          Pack->tabPacket[i] = ((0x1 <<11) | (inter<<2) | (par<<1) | 0x1) & 0xFFF;
          first = 0;
        }
        else {
          inter = ((data[i] <<1) | 0x0) & 0x1FF;
          par = parite_paire(inter);
          Pack->tabPacket[i] = ((0x1 <<11) | (inter<<2) | (par<<1) | 0x1) & 0xFFF;

        }
    }
}

/***************************************************************************
              Envoie des trames de données codées en "manchester".
    Entree : packet 12bits et n nombre de repetition

/***************************************************************************/
void manchester_send_trame(struct s_Packet *Pack,int n) {

  int i, lenght;
  unsigned char txPin = 0;

  lenght = n;
  RF_OUT_On();
  //digitalWrite(txPin, HIGH);
  WAIT1_Waitus(DEBUT_COMMANDE);
  //delayMicroseconds(DEBUT_COMMANDE);
  while(n>0) {
    for (i=0; i<7; i++){
        manchester_send(Pack->tabPacket[i]);
    }
   WAIT1_Waitus(INTER_TRAME);
   //delayMicroseconds(INTER_TRAME);
    n--;
  }
}

static uint8_t MCZ_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr("MCZ", "MCZ commands\r\n", io->stdOut);
  CLS1_SendHelpStr("  help|status", "Print help or status information\r\n", io->stdOut);
  CLS1_SendHelpStr("  MCZ ON ", "Sends a command ON to MCZ\r\n", io->stdOut);
  CLS1_SendHelpStr("  MCZ OFF ", "Sends a command OFF to MCZ\r\n", io->stdOut);
  CLS1_SendHelpStr("  MCZ P <ARG> ", "Sends a command ON to MCZ\r\n", io->stdOut);
  CLS1_SendHelpStr("  MCZ V1 <ARG> ", "Sends a command ON to MCZ\r\n", io->stdOut);
  CLS1_SendHelpStr("  MCZ V2 <ARG> ", "Sends a command ON to MCZ\r\n", io->stdOut);
  return ERR_OK;
}

/***************************************************************************
              Envoie des trames de données codées en "manchester".
    Entree : packet 12bits et n nombre de repetition

/***************************************************************************/
static uint8_t MCZ_send_Cmd(const CLS1_StdIOType *io)
{
	// envoi de la commande RF
    Convert2Databytes(&Util_config,&octet);
    Calcul_Crc(&octet);
    encapsule(&octet,&packet);
    LEDB_On();
    manchester_send_trame(&packet,5);
    LEDB_Off();

    /* Affichage valeur de chaque champs de données */
/*    CLS1_SendStr("ID = ", io->stdErr);
    CLS1_SendNum32u(Util_config.Id, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Modes = ", io->stdErr);
    CLS1_SendNum32u(Util_config.Modes, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("User = ", io->stdErr);
    CLS1_SendNum32u(Util_config.User, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Puissance = ", io->stdErr);
    CLS1_SendNum32u(Util_config.Puissance, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Ventilateur 1 = ", io->stdErr);
    CLS1_SendNum32u(Util_config.Ventilateur1, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Ventilateur 2 = ", io->stdErr);
    CLS1_SendNum32u(Util_config.Ventilateur2, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);

    /* Affichage des octets */
/*    CLS1_SendStr("Octet 1 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_MSB_Id, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Octet 2 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_ISB_Id, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Octet 3 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_LSB_Id, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Octet 4 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_Parameter1, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Octet 5 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_Parameter2, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Octet 6 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_MSB_CRC, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Octet 7 = ", io->stdErr);
    CLS1_SendNum32u(octet.O_LSB_CRC, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);

    /* Affichage des octets */
/*    CLS1_SendStr("Packet 1 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[0], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Packet 2 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[1], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Packet 3 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[2], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Packet 4 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[3], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Packet 5 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[4], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Packet 6 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[5], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("Packet 7 = ", io->stdErr);
    CLS1_SendNum32u(packet.tabPacket[6], io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);                    */
}

uint8_t MCZ_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint32_t val;
  uint8_t res;
  const unsigned char *p;
  uint8_t pwd[24], ssid[24];

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "MCZ help")==0) {
    *handled = TRUE;
    res = MCZ_PrintHelp(io);
  } else if (UTIL1_strcmp((char*)cmd, "MCZ ON")==0) {
    *handled = TRUE;
    Util_config.Modes = 1;
    MCZ_send_Cmd(io);
    CLS1_SendStr("Commande ON envoyée\r\n", io->stdErr);
  } else if (UTIL1_strcmp((char*)cmd, "MCZ OFF")==0) {
    *handled = TRUE;
    Util_config.Modes = 0;
    MCZ_send_Cmd(io);
    CLS1_SendStr("Commande OFF envoyée\r\n", io->stdErr);
  } else if (UTIL1_strncmp((char*)cmd, "MCZ P ", sizeof("MCZ P ")-1)==0) {
	*handled = TRUE;
	p = cmd+sizeof("MCZ P ")-1;
    Util_config.Puissance = atoi(p);
    MCZ_send_Cmd(io);
    CLS1_SendStr("Modification de la puissance de chauffe à ", io->stdErr);
    CLS1_SendNum32u(Util_config.Puissance, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
  } else if (UTIL1_strncmp((char*)cmd, "MCZ V1 ", sizeof("MCZ V1 ")-1)==0) {
    *handled = TRUE;
    p = cmd+sizeof("MCZ V1 ")-1;
    Util_config.Ventilateur1 = atoi(p);
    MCZ_send_Cmd(io);
    CLS1_SendStr("Modification de la vitesse du ventilateur 1 à ", io->stdErr);
    CLS1_SendNum32u(Util_config.Ventilateur1, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
  } else if (UTIL1_strncmp((char*)cmd, "MCZ V2 ", sizeof("MCZ V2 ")-1)==0) {
    *handled = TRUE;
    p = cmd+sizeof("MCZ V2 ")-1;
    Util_config.Ventilateur2 = atoi(p);
    MCZ_send_Cmd(io);
    CLS1_SendStr("Modification de la vitesse du ventilateur 2 à ", io->stdErr);
    CLS1_SendNum32u(Util_config.Ventilateur2, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
  }
 return res;
}

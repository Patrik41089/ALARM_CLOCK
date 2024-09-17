//knihovny
#include <stdbool.h>
#include <stm8s.h>
#include <stdio.h>
#include "main.h"
#include "milis.h"
#include "uart1.h"


//inicializace
void init(void)
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  init_milis();

  //I2C
  /* I2C_DeInit();
  I2C_Init(100000,  //frekvence výstupní pro komunikaci I2C
  0x00,             //(slave) adresa I2C, hodnota mě nezajímá, protože mám mikrokontrolér jako MASTER
  I2C_DUTYCYCLE_2,  //trvání LOW a HIGH (tady to je 1:1, 16:9 je ještě možné vybrat)
  I2C_ACK_CURR,     //povolení signálu, když příjmu data, mikrokontrolér odešle ACK
  I2C_ADDMODE_7BIT, //pro RTC mi stačí 7bit adresa (0x68), možnost ještě 10bit
  16000000);        //taktování procesoru => mám na 16Mhz */

  //UART
  UART1_DeInit();
  UART1_Init(9600,                    //baudrate = komunikační rychlost
   UART1_WORDLENGTH_8D,               //délka slova
    UART1_STOPBITS_1,                 //stopbit (pauza pro konec přenosu)
     UART1_PARITY_NO,                 //parita (kontrola chyb)
      UART1_SYNCMODE_CLOCK_DISABLE,   //asynchronní (nepoužívám clock, jednodušší a častější, používám baudrate pro komunikaci (tx a rx kanály))
       UART1_MODE_TXRX_ENABLE         //přijímám a odesílám
       );

  //enableInterrupts();                         //globálně povolím přerušení
  UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);   //povolí přerušení při příjmu znaku
                                              //RXNE(Receive Data Register Not Empty) přijatý byte dat je připraven k přetečení
                                              //OR(OverRUn) signalizuje přetečení při přijímání dat
  UART1_Cmd(ENABLE);
}

//pro funkci printf musím nadeklarovat funkci putchar
int putchar(int c) {
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET) //čeká dokud není prázdný k přijetí nových dat
        ;
    UART1_SendData8(c);                                  //pošlu data
    return (c);                                          //vracím data
}

void main(void)
{
    uint32_t time = 0;

    init();

    while (1) {
      if(milis() - time > 500)
      {
        time = milis();
        UART1_SendData8('A');
        printf("AHOJ!");
      }
    }
}

/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
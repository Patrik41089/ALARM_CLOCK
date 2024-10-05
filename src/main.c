//knihovny
#include <stdbool.h>
#include <stm8s.h>
#include <stdio.h>
#include "main.h"
#include "milis.h"

//PD4 protoze na tomhle pinu je nastaven vystup casovace TIM2 pro OC1 (outputchannel1)
#define BUZZER_PORT GPIOD
#define BUZZER_PIN GPIO_PIN_4
//(I2C) PB4 pro SCL a PB5 pro DATA, urceno strukturou stm8s(mohu zmenit)
#define SCL_PORT GPIOB
#define SCL_PIN GPIO_PIN_4
#define SDA_PORT GPIOB
#define SDA_PIN GPIO_PIN_5
//NCODER (urcil jsem si sam, neni prikazan, kde by musel byt)
#define NCLK_PORT GPIOF
#define NCLK_PIN GPIO_PIN_4
#define NDT_PORT GPIOF
#define NDT_PIN GPIO_PIN_5
#define SW_PORT GPIOE
#define SW_PIN GPIO_PIN_3

volatile bool tlacitko_SW = false;

//pokud tlacitko bylo stisknuto nastavim na true
void preruseni_enkoderem(void)
{
    if (GPIO_ReadInputPin(SW_PORT, SW_PIN) == RESET)
    {
        tlacitko_SW = true;
    }
    //NEJAK SMAZAT VLAJKU!
}

//nastavim preruseni na port E kde mam tlacitko enkoderu
void E(void)
{
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOE, EXTI_SENSITIVITY_FALL_ONLY);
}

//inicializace
void init(void)
{
  //BUZZER
  GPIO_Init(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUT_PP_LOW_FAST);

  //NCODER
  GPIO_Init(NCLK_PORT, NCLK_PIN, GPIO_MODE_IN_FL_NO_IT);
  GPIO_Init(NDT_PORT, NDT_PIN, GPIO_MODE_IN_FL_NO_IT);        //float prototoze neurcita zmena stavu se hodi pro enkoder vzhledem k stavu kdy se s nim nic nedeje
  GPIO_Init(SW_PORT, SW_PIN, GPIO_MODE_IN_PU_IT);           //pull-up rezistor protoze SW budu pouzivat jako tlacitko co dela preruseni
  //taktování procesoru na 16MHz
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  init_milis();

  //I2C
  GPIO_Init(SCL_PORT, SCL_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
  GPIO_Init(SDA_PORT, SDA_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);

  I2C_DeInit();
  I2C_Init(100000,  //frekvence výstupní pro komunikaci I2C
  0x00,             //(slave) adresa I2C, hodnota mě nezajímá, protože mám mikrokontrolér jako MASTER
  I2C_DUTYCYCLE_2,  //trvání LOW a HIGH (tady to je 1:1, 16:9 je ještě možné vybrat)
  I2C_ACK_CURR,     //povolení signálu, když příjmu data, mikrokontrolér odešle ACK
  I2C_ADDMODE_7BIT, //pro RTC mi stačí 7bit adresa (0x68), možnost ještě 10bit
  16000000);        //taktování procesoru => mám na 16Mhz

  //UART
  UART1_DeInit();
  UART1_Init(9600,                    //baudrate = komunikační rychlost
   UART1_WORDLENGTH_8D,               //délka slova
    UART1_STOPBITS_1,                 //stopbit (pauza pro konec přenosu)
     UART1_PARITY_NO,                 //parita (kontrola chyb)
      UART1_SYNCMODE_CLOCK_DISABLE,   //asynchronní (nepoužívám clock, jednodušší a častější, používám baudrate pro komunikaci (tx a rx kanály))
       UART1_MODE_TXRX_ENABLE);        //přijímám a odesílám

  enableInterrupts();                         //globálně povolím přerušení
  UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);   //povolí přerušení při příjmu znaku
                                              //RXNE(Receive Data Register Not Empty) přijatý byte dat je připraven k přetečení
                                              //OR(OverRUn) signalizuje přetečení při přijímání dat
  UART1_Cmd(ENABLE);

  //TIM2
  TIM2_DeInit();
  TIM2_TimeBaseInit(TIM2_PRESCALER_1, 15999);   //16:1 = 16MHz : 16000 = 1kHz
  TIM2_OC1Init(                // konfigurace output channel1
        TIM2_OCMODE_PWM1,        // mod PWM1
        TIM2_OUTPUTSTATE_ENABLE, // povolím
        1000,                      // nastavuju šířku impulzu
        TIM2_OCPOLARITY_HIGH);   // nastavení polarity
  TIM2_OC1PreloadConfig(ENABLE);
  //TIM2_Cmd(ENABLE);                                   //spustí TIM2
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);              //povolí přerušení od TIM2
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
    bool buzzer = true;
    uint32_t time = 0;


    init();
    E();

    while(1)
    {
       if (milis() - time > 500)
        {
            if(buzzer)
            {
                time = milis();
                TIM2_Cmd(ENABLE);
                printf("zapinam\r\n");
                buzzer = false;
            }
            else
            {
                time = milis();
                TIM2_Cmd(DISABLE);
                printf("vypinam\r\n");
                buzzer = true;
            }
        }

/*         if(milis() - time > 50)
        {
            time = milis();
            if(tlacitko_SW == true)
                {
                    printf("LOL\r\n");
                    tlacitko_SW = false;
                }

        } */

    }
}
/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
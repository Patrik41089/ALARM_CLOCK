//knihovny
#include <stdbool.h>
#include <stm8s.h>
#include <stdio.h>
#include "main.h"
#include "milis.h"
#include "stm8_hd44780.h"
#include "swi2c.h"

//PD4 protoze na tomhle pinu je nastaven vystup casovace TIM2 pro OC1 (outputchannel1)
#define BUZZER_PORT GPIOD
#define BUZZER_PIN GPIO_PIN_4
//(I2C) PB4 pro SCL a PB5 pro DATA, urceno strukturou stm8s
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

volatile bool tlacitko_SW = false; //volatile protoze externi preruseni (zvenku)

//pokud tlacitko bylo stisknuto nastavim na true
void preruseni_enkoderem(void)
{
    if (GPIO_ReadInputPin(SW_PORT, SW_PIN) == RESET)
    {
        tlacitko_SW = true;
    }
}

/* void otaceni_enkoderem(void)
{
    //...
} */

//inicializace
void init(void)
{
  //taktování procesoru na 16MHz
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);

  //BUZZER
  GPIO_Init(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUT_PP_LOW_FAST);

  //NCODER
  GPIO_Init(NCLK_PORT, NCLK_PIN, GPIO_MODE_IN_FL_NO_IT);
  GPIO_Init(NDT_PORT, NDT_PIN, GPIO_MODE_IN_FL_NO_IT);        //float prototoze neurcita zmena stavu se hodi pro enkoder vzhledem k stavu kdy se s nim nic nedeje
  GPIO_Init(SW_PORT, SW_PIN, GPIO_MODE_IN_PU_IT);           //pull-up rezistor protoze SW budu pouzivat jako tlacitko co dela preruseni
                                                        //IT PROTOZE JINAK BY BYLO PRERUSENI ZAKAZANE = NEFUNGOVALO BY

  //milis
  init_milis();

  //I2C
  swi2c_init();

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
  TIM2_TimeBaseInit(TIM2_PRESCALER_1, 63999);   //16:1 = 16MHz : 16000 = 1kHz
  TIM2_OC1Init(                // konfigurace output channel1
        TIM2_OCMODE_PWM1,        // mod PWM1
        TIM2_OUTPUTSTATE_ENABLE, // povolím
        1000,                      // nastavuju šířku impulzu
        TIM2_OCPOLARITY_HIGH);   // nastavení polarity
  TIM2_OC1PreloadConfig(ENABLE);                                 //spustí TIM2
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);              //povolí přerušení od TIM2

}

//pro funkci printf musím nadeklarovat funkci putchar
int putchar(int c) {
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET) //čeká dokud není prázdný k přijetí nových dat
        ;
    UART1_SendData8(c);                                  //pošlu data
    return (c);                                          //vracím data
}

//I2C test
void test_I2C(void)
{
    if(swi2c_recover() == 0)
    {
        printf("neselhal recover\n\r");
        if(swi2c_test_slave(0x68<<1) == 0)
        {
            printf("neselhal test slave\n\r");
            printf("%02X\n\r", 0x68);
        }
        else
        {
            printf("selhal test slave\n\r");
        }
    }        
    else
    {
        printf("selhal recover\n\r");
    }  
}


void main(void)
{
    bool alarm = false;
    bool prepnuti = true;
    bool zapsal = false;

    uint32_t time  = 0;
    uint32_t time1 = 0;
    uint32_t time2 = 0;
    uint32_t time3 = 0;
    uint32_t time4 = 0;
    uint32_t time5 = 0;

    uint8_t DATA_DO_RTC[7]= {0,0,0,0,0,0,0};
    uint8_t DATA_Z_RTC[7] = {0,0,0,0,0,0,0};

    uint8_t DATA_Z_ALARM1[4] = {0,0,0,0};
    uint8_t DATA_Z_ALARM2[3] = {0,0,0};

    uint8_t DATA_DO_ALARM1[4] ={0,0,0,0};
    uint8_t DATA_DO_ALARM2[3] ={0,0,0};


    uint8_t status_ALARMU = 0;
    uint8_t reset_status = status_ALARMU & 0xFC; //resetuje oba alarmy (A1F a A2F)

    //RTC v BCD davam HEXA pak nezapomenout prevest
    DATA_DO_RTC[0] = 0x00;  //sekundy
    DATA_DO_RTC[1] = 0x10;  //minuty
    DATA_DO_RTC[2] = 0x10;  //hodiny
    //DATA_DO_RTC[3] = 0x01;  //den v tydnu
    DATA_DO_RTC[4] = 0x00;  //dny
    DATA_DO_RTC[5] = 0x10;  //měsíce
    DATA_DO_RTC[6] = 0x24;  //roky

    //ALARM1
    DATA_DO_ALARM1[0] = 0x10;   //sekundy
    DATA_DO_ALARM1[1] = 0x10;   //minuty
    DATA_DO_ALARM1[2] = 0x10;   //hodiny
    DATA_DO_ALARM1[3] = 0x00;   //den/datum kdyz 0 tak kazdy den

    //ALARM2
    DATA_DO_ALARM2[0] = 0x11;   //minuty
    DATA_DO_ALARM2[1] = 0x10;   //hodiny
    DATA_DO_ALARM2[2] = 0x00;   //den/datum kdyz 0 tak kazdy den

    init(); //init vseho co jsem inicializoval
    test_I2C();

    while(1)
    {
        if (milis() - time > 500)
        {
            if(alarm)
            {
                if(milis() - time > 500 && prepnuti)
                {
                    time = milis();
                    TIM2_Cmd(ENABLE);
                    prepnuti = false;
                }
                else
                {
                    time = milis();
                    TIM2_Cmd(DISABLE);
                    prepnuti = true;

                }
                if(tlacitko_SW == true)
                {
                    TIM2_Cmd(DISABLE);
                    alarm = false;                                      //funkce pipani vypnout
                    swi2c_write_buf(0x68 << 1, 0x0F, &reset_status, 1); //reset bity alarmy
                    tlacitko_SW = false;                                //reset tlacitko zmacknuto
                }
                if(zapsal)                                              //pokud jsem nahral cas a alarmy tak je resetovat + vypnout tuhle funkci aby nepipalo
                {
                    TIM2_Cmd(DISABLE);
                    alarm = false;                                      
                    swi2c_write_buf(0x68 << 1, 0x0F, &reset_status, 1); 
                    zapsal = false;
                }
            }
        }
        //ZAPISUJI ALARM A CAS
        if(milis() - time1 > 80000) //80000 //1000
        {
            time1 = milis();            //MUSIM POUZIT JINOU PROMENNOU CASU !!!
              
            printf("zapisu do RTC %d\n\r",  swi2c_write_buf(0x68 << 1, 0x00, DATA_DO_RTC, 7));
            printf("zapisu do ALARM1 %d\n\r", swi2c_write_buf(0x68 << 1, 0x07, DATA_DO_ALARM1, 4));
            printf("zapisu do ALARM2 %d\n\r", swi2c_write_buf(0x68 << 1, 0x0B, DATA_DO_ALARM2, 3));
            zapsal = true;
            //TIM2_Cmd(DISABLE); nepomohlo
            //swi2c_write_buf(0x68 << 1, 0x0F, &reset_status, 1); jen resetlo ale furt pipa
        }
        
        //ALARMY do UART
        if(milis() - time2 > 20000 )
        {
            time2 = milis();
            swi2c_read_buf(0x68 << 1, 0x07, DATA_Z_ALARM1, 4); //posouvam adresu, od jake adresy, jaka data, a kolik dat (kolik adres zaplni)
            printf("Alarm1: %d%d:%d%d:%d%d \n\r",
                    //DATA_Z_ALARM1[3] >> 4, DATA_Z_ALARM1[3] & 0x0F,
                    DATA_Z_ALARM1[2] >> 4, DATA_Z_ALARM1[2] & 0x0F, 
                    DATA_Z_ALARM1[1] >> 4, DATA_Z_ALARM1[1] & 0x0F, 
                    DATA_Z_ALARM1[0] >> 4, DATA_Z_ALARM1[0] & 0x0F);

            swi2c_read_buf(0x68 << 1, 0x0B, DATA_Z_ALARM2, 3);
            printf("Alarm2: %d%d:%d%d \n\r",
                    //DATA_Z_ALARM2[2] >> 4, DATA_Z_ALARM2[2] & 0x0F, 
                    DATA_Z_ALARM2[1] >> 4, DATA_Z_ALARM2[1] & 0x0F, 
                    DATA_Z_ALARM2[0] >> 4, DATA_Z_ALARM2[0] & 0x0F);
        }
        //CAS do UART
        if(milis() - time3 > 5000)
        {
            time3 = milis();
            swi2c_read_buf(0x68 << 1, 0x00, DATA_Z_RTC, 7);
            printf("dat: %d%d.%d%d.\n\r rok: 20%d%d \n\r cas: %d%d:%d%d:%d%d \n\r",
                DATA_Z_RTC[4] >> 4, DATA_Z_RTC[4] & 0x0F,
                DATA_Z_RTC[5] >> 4, DATA_Z_RTC[5] & 0x0F,
                DATA_Z_RTC[6] >> 4, DATA_Z_RTC[6] & 0x0F,
                DATA_Z_RTC[2] >> 4, DATA_Z_RTC[2] & 0x0F,
                DATA_Z_RTC[1] >> 4, DATA_Z_RTC[1] & 0x0F,
                DATA_Z_RTC[0] >> 4, DATA_Z_RTC[0] & 0x0F);
        }
        if (milis() - time4 > 1000)
        {
            time4 = milis();
            swi2c_read_buf(0x68 << 1, 0x0F, &status_ALARMU, 1);
            if (status_ALARMU & 0x01) //nejnizsi bit
            {
                printf("Alarm1 on\n\r");
                alarm = true;
            }
            else
            {
                printf("Alarm1 off\n\r");
            }
            if (status_ALARMU & 0x02) //druhy nejnizsi bit
            {
                printf("Alarm2 on\n\r");
                alarm = true;
            }
            else
            {
                printf("Alarm2 off\n\r");
            }
        }
        
    }
}


/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
//knihovny
#include <stdbool.h>
#include <stm8s.h>
#include <stdio.h>
#include "main.h"
#include "milis.h"
#include "delay.h"
#include "uart1.h"

#define BUZZER_PIN GPIO_PIN_7
#define BUZZER_PORT GPIOB
//int8_t minule = 0;

//init buzzeru
void buzzer(void)
{
  GPIO_Init(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
  BEEP_Init(BEEP_FREQUENCY_4KHZ);
}

int main(void)
{
  bool zvuk = false;
  int32_t time1 = 0;
  int32_t time2 = 0;
  buzzer();

  while(1){
    if(milis() - time1 > 500)
    {
      time1 = milis();
    }
  }
}


/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"














/* //časovač tim2
void casovac1(void)
{
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  TIM2_TimeBaseInit(TIM2_PRESCALER_16, 10000 - 1);
  TIM2_Cmd(ENABLE);
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
  init_milis();
}
 */



/* void pipani(void)
{
  if(minule == 0)
  {
    GPIO_WriteHigh(BUZZER_PORT, BUZZER_PIN);
    minule = 1;
  }
  else
  {
    GPIO_WriteLow(BUZZER_PORT, BUZZER_PIN);
    minule = 0;
  }
} */





/* void uart(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);      // taktovani MCU na 16MHz

    UART1_DeInit();
    UART1_Init(
      9600,
      UART1_WORDLENGTH_8D,
      UART1_STOPBITS_1,
      UART1_PARITY_NO,
      UART1_SYNCMODE_CLOCK_DISABLE,
      UART1_MODE_TXRX_ENABLE
    );
    
    enableInterrupts();
    UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
}

int putchar(int x){
  while(UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
  ;
  UART1_SendData8(x);
  return(x);

} */


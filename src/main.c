//knihovny
#include <stdbool.h>
#include <stm8s.h>
#include <stdio.h>
#include "main.h"
#include "milis.h"
#include "delay.h"
#include "uart1.h"


#define BUZZER_PIN GPIO_PIN_?
#define BUZZER_PORT GPIO?

//inicializace
void init(void) {
    GPIO_Init(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    init_milis();
}
//inicializace casovace TIM2
void casovac(void)
{
    TIM2_DeInit();
    TIM2_TimeBaseInit(TIM2_PRESCALER_16, 250 - 1);
    TIM2_OC2Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 500, TIM2_OCPOLARITY_HIGH);
    TIM2_OC2PreloadConfig(ENABLE);
    TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
    TIM2_Cmd(ENABLE);

}

int main(void)
{
  int32_t time = 0;

  init();
  casovac();

  while(1)
  {
    if(milis() - time > 500)
    {
      time = milis();
    }
  }
}

/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"


















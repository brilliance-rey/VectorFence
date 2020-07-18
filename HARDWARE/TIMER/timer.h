#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"

#define SET_VAR_BIT(var, bit) 	(var |= (1<<bit))
#define CLEAR_VAR_BIT(var, bit) (var &= ~(1<<bit))
#define IS_SET_VAR_BIT(var, bit) ((var >> bit) & 0x01)

extern u8 timerHandlerFlag;

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM4_Int_Init(u16 arr,u16 psc);
void TIM5_Int_Init(u16 arr,u16 psc);

#endif



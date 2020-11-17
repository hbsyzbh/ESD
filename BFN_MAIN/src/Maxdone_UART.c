#include "em_usart.h"
#include "em_cmu.h"
#include "Maxdone_UART.h"
#include "hal-config.h"

#define UART_BAUDRATE 9600

void SetE22State(E22_STATE state)
{
	switch(state)
	{
	case E22_Config:
		   GPIO_PinOutClear( gpioPortB, 13U );
		   GPIO_PinOutSet  ( gpioPortB, 14U );
		break;
	case E22_Normal:
	default:
		   GPIO_PinOutClear( gpioPortB, 13U );
		   GPIO_PinOutClear( gpioPortB, 14U );
	}
}

void MD_UART_init( void )
{
	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_CORELE, true);

   CMU_ClockEnable( cmuClock_USART0, true );
   CMU_ClockEnable( cmuClock_USART1, true );

   init.baudrate = UART_BAUDRATE;
   init.parity = usartNoParity;

   USART_InitAsync( USART0, &init );
   USART_InitAsync( USART1, &init );

   GPIO_PinModeSet( PORTIO_USART0_RX_PORT, PORTIO_USART0_RX_PIN, gpioModeInputPull, 1 );
   GPIO_PinModeSet( PORTIO_USART0_TX_PORT, PORTIO_USART0_TX_PIN, gpioModePushPull, 1 );

   GPIO_PinModeSet( PORTIO_USART1_RX_PORT, PORTIO_USART1_RX_PIN, gpioModeInputPull, 1 );
   GPIO_PinModeSet( PORTIO_USART1_TX_PORT, PORTIO_USART1_TX_PIN, gpioModePushPull, 1 );

   USART0->ROUTELOC0 = ( 	 BSP_USART0_RX_LOC << _USART_ROUTELOC0_RXLOC_SHIFT
		   	   	   	   	   | BSP_USART0_TX_LOC << _USART_ROUTELOC0_TXLOC_SHIFT);
   USART0->ROUTEPEN  = (      USART_ROUTEPEN_RXPEN
                            | USART_ROUTEPEN_TXPEN );


   USART1->ROUTELOC0 = ( 	 BSP_USART1_RX_LOC << _USART_ROUTELOC0_RXLOC_SHIFT
		   	   	   	   	   | BSP_USART1_TX_LOC << _USART_ROUTELOC0_TXLOC_SHIFT);
   USART1->ROUTEPEN  = (      USART_ROUTEPEN_RXPEN
                            | USART_ROUTEPEN_TXPEN );


   NVIC_ClearPendingIRQ(USART0_IRQn);
   NVIC_EnableIRQ(USART0_IRQn);

   NVIC_ClearPendingIRQ(USART1_IRQn);
   NVIC_EnableIRQ(USART1_IRQn);

   //USART_IntClear(USART0, USART_IFC_RXDATAV);
   USART_IntEnable(USART0, USART_IEN_RXDATAV);

   //USART_IntClear(USART1, USART_IFC_RXDATAV);
   USART_IntEnable(USART1, USART_IEN_RXDATAV);

   GPIO_PinModeSet( gpioPortB, 13U, gpioModePushPull, 1 );
   GPIO_PinModeSet( gpioPortB, 14U, gpioModePushPull, 1 );

   SetE22State(E22_Normal);
}

void USBUART_Tx(uint8_t data)
{
	USART_Tx( USB_UART, data);
}

void E22UART_Tx(uint8_t data)
{
	USART_Tx( USART1, data);
}

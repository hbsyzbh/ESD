#define USB_UART USART0

typedef enum {
	E22_Normal,
	E22_Config
}	E22_STATE;

void MD_UART_init( void );
void USBUART_Tx(unsigned char data);
void E22UART_Tx(unsigned char data);
void SetE22State(E22_STATE state);

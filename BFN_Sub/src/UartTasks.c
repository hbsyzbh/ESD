#include <stdio.h>
#include <string.h>

#include  <kernel/include/os.h>
#include  <common/include/rtos_utils.h>
#include "em_usart.h"
#include "em_timer.h"
#include "em_msc.h"
#include "UartTasks.h"
#include  "Maxdone_UART.h"
#include  "DisplayTasks.h"


#define USB_TX_DONE		1
#define E22_TX_DONE		2


USER_DATA *const pFlashUserData = (USER_DATA *)USERDATA_BASE;
USER_DATA UserData;

bool isRFCfg = false;
//OS_FLAG_GRP UartFlagG;

OS_TMR USBUartTimer;
OS_TMR E22UartTimer;

OS_SEM BeepSem;

OS_Q USBUartAnalysisQ;
OS_Q E22UartAnalysisQ;

#define USBUartTimerTick	(2)
#define E22UartTimerTick	(2)

static char InitDone = 0;

void USBUartTimerCallback(void *p_tmr, void *p_arg);
void E22UartTimerCallback(void *p_tmr, void *p_arg);

unsigned short getRFIDFromComPort(char *Buff)
{
	int i;
	unsigned short id = 0;
	if((Buff) && (4 == strnlen(Buff, 32))){
		for(i=0; i<4; i++) {
			unsigned char tmp;

			id *= 10;
			tmp = Buff[i];
			if ((tmp >= '0') && (tmp <= '9')){
				id += Buff[i] - '0';
			}
		}
	} else {
		return 9999;
	}

	return id;
}

void saveUserData(void)
{
	//没必要不修改，保护Flash
	if( (UserData.RF_ID 	!= pFlashUserData->RF_ID) 	 ||
		(UserData.AlertOhm  != pFlashUserData->AlertOhm) ||
		(UserData.state  != pFlashUserData->state)
	) {
		MSC_ErasePage(USERDATA_BASE);
		MSC_WriteWord(USERDATA_BASE, &UserData, sizeof(USER_DATA));
	}
}

void UartTasksInit(void) {
	extern OS_Q KeyQ;

	RTOS_ERR err;

	OSQCreate(&USBUartAnalysisQ, "", 3, &err);
	OSQCreate(&E22UartAnalysisQ, "", 3, &err);

	OSQCreate(&KeyQ, "", 3, &err);


	OSTmrCreate(&USBUartTimer, "", USBUartTimerTick, USBUartTimerTick,
			OS_OPT_TMR_ONE_SHOT, USBUartTimerCallback, NULL, &err);
	OSTmrCreate(&E22UartTimer, "", E22UartTimerTick, E22UartTimerTick,
			OS_OPT_TMR_ONE_SHOT, E22UartTimerCallback, NULL, &err);

	OSSemCreate(&BeepSem, "Beeper", 0, &err);
	UserData = *pFlashUserData;
	if (UserData.RF_ID > 9999) UserData.RF_ID = 9999;

	if (UserData.AlertOhm > 999) UserData.AlertOhm = 999;

	if (UserData.state >= SysState_undefined) UserData.state = SysState_both;
	UserData.subs = 0;
	InitDone = 1;

#if 0
	DrawLogo();
	while(1)
		USBUartSendBuff("Maxdone!", 8);
#endif
}

#define ACK_BUFF_SIZE	(128)

void FillDebugACK(char *Buff, unsigned char len) {
	RTOS_ERR err;

	OSSchedLock(&err);
	snprintf(Buff, ACK_BUFF_SIZE, "SUB% 04d% 04d% 04d% 04d% 04d% 04d% 04d% 04d% 04d",
			getAlertOhmInt(), getCurOhmInt(0), getCurOhmInt(1), getCurOhmInt(2), getCurOhmInt(3),
			getCurOhmInt(4), getCurOhmInt(5), getCurOhmInt(6), getCurOhmInt(7)
	);
	OSSchedUnlock(&err);
}

void FillSubACK (char *Buff, unsigned char len, unsigned char count) {
	RTOS_ERR err;

	OSSchedLock(&err);
	snprintf(Buff, len, "SUB");

	for(int i = 0; i < count; i++)
	{
		snprintf(&Buff[3 + i*4], len - 3 - i*4, "%04d", getCurOhmInt(i));
	}
	OSSchedUnlock(&err);
}

void FillACK(char *Buff, unsigned char len) {
	RTOS_ERR err;

	OSSchedLock(&err);
	snprintf(Buff, ACK_BUFF_SIZE, "SUB% 04d% 04d% 04d% 04d% 04d% 04d% 04d% 04d% 04d",
			getAlertOhmInt(), getCurOhmInt(0), getCurOhmInt(1), getCurOhmInt(2), getCurOhmInt(3),
			getCurOhmInt(4), getCurOhmInt(5), getCurOhmInt(6), getCurOhmInt(7)
	);
	OSSchedUnlock(&err);
}

void E22UartTask(void *p_arg) {
	OS_MSG_SIZE msg_size;
	CPU_TS ts;
	RTOS_ERR err;
	char Buff[ACK_BUFF_SIZE];

	for (;;) {

		snprintf(Buff, 32, "MDQ%04d", UserData.RF_ID);
		char *str = OSQPend(&E22UartAnalysisQ, 0, OS_OPT_PEND_BLOCKING,
				&msg_size, &ts, &err);
		if ((RTOS_ERR_NONE == err.Code) && (msg_size > 0) && (str != NULL)) {
			//E22UART_Tx(str[i]);
			if(isRFCfg) {
				for (int i = 0; i < msg_size; i++) {
					USBUART_Tx(str[i]);
				}
				continue;
			}

			if ((7 == msg_size) && (0 == memcmp(str, Buff, msg_size))) {
				//Beep();
				//FillACK(Buff, ACK_BUFF_SIZE);
				FillDebugACK(Buff, ACK_BUFF_SIZE);
				for (int i = 0; i < strnlen(Buff, ACK_BUFF_SIZE); i++) {
					E22UART_Tx(Buff[i]);
				}
			}
		}
	}
}

void USBUartTask(void *p_arg) {
	OS_MSG_SIZE msg_size;
	CPU_TS ts;
	RTOS_ERR err;
	char Buff[ACK_BUFF_SIZE];

	for (;;) {
		char *str = OSQPend(&USBUartAnalysisQ, 0, OS_OPT_PEND_BLOCKING,
				&msg_size, &ts, &err);
		if ((RTOS_ERR_NONE == err.Code) && (msg_size > 0) && (str != NULL)) {
			if(isRFCfg) {
				for (int i = 0; i < msg_size; i++) {
					E22UART_Tx(str[i]);
				}
				continue;
			}

			if ((4 == msg_size) && (0 == memcmp(str, "SHOW", msg_size))) {
				//Beep();
				FillACK(Buff, ACK_BUFF_SIZE);
				for (int i = 0; i < strnlen(Buff, ACK_BUFF_SIZE); i++) {
					USBUART_Tx(Buff[i]);
				}
			} else if ((5 == msg_size) && (0 == memcmp(str, "DEBUG", 5))) {
				FillDebugACK(Buff, ACK_BUFF_SIZE);
				for (int i = 0; i < strnlen(Buff, ACK_BUFF_SIZE); i++) {
					USBUART_Tx(Buff[i]);
				}
			} else if ((15 == msg_size) && (0 == memcmp(str, "SUB", 3))) {
				char buff[5] = {0};

				strncpy(buff, &str[3], 4);
				UserData.subs = getRFIDFromComPort(buff);

				strncpy(buff, &str[11], 4);
				UserData.AlertOhm = getRFIDFromComPort(buff);
				FillSubACK(Buff, ACK_BUFF_SIZE, UserData.subs);
				for (int i = 0; i < strnlen(Buff, ACK_BUFF_SIZE); i++) {
					USBUART_Tx(Buff[i]);
				}
			}
		}
	}
}

#define USART0_RX_BUFF_SIZE (64)
static unsigned char USART0_RX_BUFF[USART0_RX_BUFF_SIZE];
static unsigned char USART0_RX_Ptr = 0;
void USART0_IRQHandler(void) {
	RTOS_ERR err;
	if (InitDone) {

		uint32_t flags = USART_IntGet(USART0);

		if ((flags & USART_IF_RXDATAV) == USART_IF_RXDATAV) {
			USART0_RX_BUFF[USART0_RX_Ptr++] = USART_Rx(USART0);
			if (USART0_RX_Ptr >= USART0_RX_BUFF_SIZE) {
				OSTmrStop(&USBUartTimer, OS_OPT_TMR_CALLBACK, NULL, &err);
			} else {
				OSTmrStart(&USBUartTimer, &err);
			}
		}
	} else {
		USART_Rx(USART0);
	}

	USART_IntClear(USART0, _USART_IFC_MASK);
}

static unsigned char USART0_RX_BUFF2[USART0_RX_BUFF_SIZE];
static unsigned char USART0_RX_BUFF2_size = 0;
void USBUartTimerCallback(void *p_tmr, void *p_arg) {
	CORE_irqState_t irqState;
	RTOS_ERR err;
	if (InitDone) {
		CPU_CRITICAL_ENTER();
		memcpy(USART0_RX_BUFF2, USART0_RX_BUFF, USART0_RX_Ptr);
		USART0_RX_BUFF2_size = USART0_RX_Ptr;
		USART0_RX_Ptr = 0;
		CPU_CRITICAL_EXIT();
		OSQPost(&USBUartAnalysisQ, USART0_RX_BUFF2, USART0_RX_BUFF2_size,
				OS_OPT_POST_FIFO, &err);
	} else {
		USART0_RX_Ptr = 0;
	}
}

#define USART_RX_BUFF_SIZE (64)
static unsigned char USART1_RX_BUFF[USART_RX_BUFF_SIZE];
static unsigned char USART1_RX_Ptr = 0;
void USART1_IRQHandler(void) {
	RTOS_ERR err;
	if (InitDone) {

		uint32_t flags = USART_IntGet(USART1);

		if ((flags & USART_IF_RXDATAV) == USART_IF_RXDATAV) {
			USART1_RX_BUFF[USART1_RX_Ptr++] = USART_Rx(USART1);
			if (USART1_RX_Ptr >= USART_RX_BUFF_SIZE) {
				OSTmrStop(&E22UartTimer, OS_OPT_TMR_CALLBACK, NULL, &err);
			} else {
				OSTmrStart(&E22UartTimer, &err);
			}
		}
	} else {
		USART_Rx(USART1);
	}
	USART_IntClear(USART1, _USART_IFC_MASK);
}

static unsigned char USART1_RX_BUFF_2[USART_RX_BUFF_SIZE];
static unsigned char USART1_RX_BUFF_2_size;
void E22UartTimerCallback(void *p_tmr, void *p_arg) {
	CORE_irqState_t irqState;
	RTOS_ERR err;
	if (InitDone) {
		CPU_CRITICAL_ENTER();
		memcpy(USART1_RX_BUFF_2, USART1_RX_BUFF, USART_RX_BUFF_SIZE);
		USART1_RX_BUFF_2_size = USART1_RX_Ptr;
		USART1_RX_Ptr = 0;
		CPU_CRITICAL_EXIT();

		OSQPost(&E22UartAnalysisQ, USART1_RX_BUFF_2, USART1_RX_BUFF_2_size,
				OS_OPT_POST_FIFO, &err);
	} else {
		USART1_RX_Ptr = 0;
	}
}

void BeepTask(void *p_arg) {
	RTOS_ERR err;
	for (;;) {
		OSSemPend(&BeepSem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
		TIMER_Enable(TIMER1, true);
		OSTimeDly(100, OS_OPT_TIME_DLY, &err);
		TIMER_Enable(TIMER1, false);
		OSTimeDly(100, OS_OPT_TIME_DLY, &err);
	}
}

void Beep(void) {
	RTOS_ERR err;

	OSSemPost(&BeepSem, OS_OPT_POST_1, &err);
}

void USBUartSendBuff(unsigned char *str, unsigned char msg_size)
{
	for (int i = 0; i < msg_size; i++) {
		USBUART_Tx(str[i]);
	}
}

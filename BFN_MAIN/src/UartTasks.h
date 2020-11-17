void UartTasksInit();
void E22UartTask(void *p_arg);
void USBUartTask(void *p_arg);
void USBUartSendBuff(unsigned char *buff, unsigned char size);
void BeepTask (void  *p_arg);
void Beep(void);

//humiture
//humidity
typedef enum {
	SysState_both,
	SysState_humiture,
	SysState_ground,
	SysState_undefined
} SYS_STATE;

typedef struct {
	unsigned short RF_ID;
	unsigned int AlertOhm;
	SYS_STATE state;
	unsigned int dummy;
}	USER_DATA;

extern USER_DATA UserData;

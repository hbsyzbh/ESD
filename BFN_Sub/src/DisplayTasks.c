#include "hal-config.h"
#include  <kernel/include/os.h>
//#include  <common/include/rtos_utils.h>
#include  "Maxdone_SPI_OLDE.h"
#include  "DisplayTasks.h"
#include  "UartTasks.h"
#include "em_adc.h"
#include "em_cmu.h"

#define Sensor_SDA ((GPIO->P[gpioPortE].DIN & (1 << 13)) == (1 << 13))

enum GroundCheckState {
	GndChk_Normal,
	GndChk_Setting
} 	GndChkState;

OS_Q KeyQ;

bool Sensor_AnswerFlag = 0;
bool Sensor_ErrorFlag = 1;
unsigned char Sensor_Data[6];
unsigned int H;
unsigned int T;
unsigned int SettingOhm;
unsigned int CurOhm[12];


ADCResult ADC[ADCChannels];

unsigned int getCurOhmInt(unsigned char index)
{
	return CurOhm[index];
}

unsigned int getAlertOhmInt(void)
{
	return UserData.AlertOhm;
}

unsigned int getLastH(void)
{
	return H;
}

unsigned int getLastT(void)
{
	return T;
}

void Clear_Sensor_SDA(void)
{
	GPIO_PinModeSet( gpioPortE, 13,
			gpioModePushPull, 0);
}

void Set_Sensor_SDA(void)
{
	GPIO_PinModeSet( gpioPortE, 13,
			gpioModePushPull, 1);
}

void Turn_Sensor_SDA_Input(void)
{
	GPIO_PinModeSet( gpioPortE, 13,
			gpioModeInput, 0);
}

/********************************************\

\********************************************/
void Delay_Nus(unsigned char t)
{
   while(t--)
   {
   	 ;
   }
}
/********************************************\
|* 功能： 延时	晶振为12M时					*|
|* 延时大约 1ms			    				*|
\********************************************/
void Delay_N1ms(unsigned int t)
{
	unsigned int d;
	while(t--) {
		d = 2000;
		while(d--);
	}
}

/********************************************\
|* 功能： 读传感器发送的单个字节	        *|
 \********************************************/
#if 0
//old
unsigned char Read_SensorData(void) {
	unsigned char i;
	unsigned int cnt;
	unsigned char buffer, tmp;
	buffer = 0;
	for (i = 0; i < 8; i++) {
		cnt = 0;
		while (!Sensor_SDA)	//检测上次低电平是否结束
		{
			if (++cnt >= 3000) {
				break;
			}
		}
		//延时Min=26us Max50us 跳过数据"0" 的高电平
		Delay_Nus(40);	 //延时30us

		//判断传感器发送数据位
		tmp = 0;
		if (Sensor_SDA) {
			tmp = 1;
			//Delay_Nus(30);
		}
		cnt = 0;
		while (Sensor_SDA)		//等待高电平 结束
		{
			if (++cnt >= 3000) {
				break;
			}
		}
		buffer <<= 1;
		buffer |= tmp;
	}
	return buffer;
}
#else
unsigned int debugCnt0[6*8];
unsigned char index0 =0;
unsigned int debugCnt[6*8];
unsigned char index =0;
unsigned char Read_SensorData(void) {
	unsigned char i;
	unsigned int cnt;
	unsigned char buffer, tmp;
	buffer = 0;
	for (i = 0; i < 8; i++) {
		cnt = 0;
		while (!Sensor_SDA)	//检测上次低电平是否结束
		{
			if (++cnt >= 3000) {
				break;
			}
		}
		debugCnt0[index0++] = cnt;
		cnt = 0;
		while (Sensor_SDA)		//等待高电平 结束
		{
			if (++cnt >= 3000) {
				break;
			}
		}

		//判断传感器发送数据位
		tmp = 0;
		if (cnt > 26) {
			tmp = 1;
		}
		debugCnt[index++] = cnt;

		buffer <<= 1;
		buffer |= tmp;
	}
	return buffer;
}
#endif
/********************************************\
|* 功能： 读传感器              	        *|
 \********************************************/
unsigned char Read_Sensor(void) {
	unsigned char i;
	unsigned int Sys_CNT;
	CORE_irqState_t irqState ;

	//主机拉低(Min=800US Max=20Ms)
	Clear_Sensor_SDA();
	Delay_N1ms(2);  //延时2Ms

	//释放总线 延时(Min=30us Max=50us)
	Set_Sensor_SDA();
	Turn_Sensor_SDA_Input();
#if 1
	Delay_Nus(20);  //延时30us
	CPU_CRITICAL_ENTER();

	//主机设为输入 判断传感器响应信号


	Sensor_AnswerFlag = 0;  // 传感器响应标志
	Sensor_ErrorFlag = 1;

	//判断从机是否有低电平响应信号 如不响应则跳出，响应则向下运行
	if (Sensor_SDA == 0) {
		Sensor_AnswerFlag = 1;  //收到起始信号
		Sys_CNT = 0;

		//判断从机是否发出 80us 的低电平响应信号是否结束
		while ((!Sensor_SDA)) {
			if (++Sys_CNT > 300) //防止进入死循环
			{
				goto end;
			}
		}
		Sys_CNT = 0;
		//判断从机是否发出 80us 的高电平，如发出则进入数据接收状态
		while ((Sensor_SDA)) {
			if (++Sys_CNT > 300) //防止进入死循环
			{
				goto end;
			}
		}
#endif
		// 数据接收	传感器共发送40位数据
		// 即5个字节 高位先送  5个字节分别为湿度高位 湿度低位 温度高位 温度低位 校验和
		// 校验和为：湿度高位+湿度低位+温度高位+温度低位
		for (i = 0; i < 5; i++) {
			Sensor_Data[i] = Read_SensorData();
		}
		Sensor_ErrorFlag = 0;	//读到数据

#if 1
	} else {
		Sensor_AnswerFlag = 0;	  // 未收到传感器响应
	}
#endif

end:
	CPU_CRITICAL_EXIT();
	return ! Sensor_ErrorFlag;
}


ADC_PosSel_TypeDef getADCChannel(unsigned char i)
{
	ADC_PosSel_TypeDef channel[] = {
		adcPosSelAPORT0XCH4,
		adcPosSelAPORT0XCH5,
		adcPosSelAPORT0XCH6,
		adcPosSelAPORT0XCH7,
		adcPosSelAPORT3XCH10,
		adcPosSelAPORT3YCH11,
		adcPosSelAPORT3XCH12,
		adcPosSelAPORT3YCH13,
	};

	if(i >= 8) i = 0;

	return channel[i];
}

void SwitchToChannel(unsigned char i)
{
	//set74HC4051(i);

	if( i & 1) {
		GPIO_PinOutSet(gpioPortB, 11U);
	} else {
		GPIO_PinOutClear(gpioPortB, 11U);
	}

	if( i & 2) {
		GPIO_PinOutSet(gpioPortB, 13U);
	} else {
		GPIO_PinOutClear(gpioPortB, 13U);
	}

	if( i & 4) {
		GPIO_PinOutSet(gpioPortB, 14U);
	} else {
		GPIO_PinOutClear(gpioPortB, 14U);
	}
}

void SensorTask(void *p_arg) {
	RTOS_ERR err;

	GPIO_PinModeSet(gpioPortD, 4U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortD, 5U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortD, 6U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortD, 7U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortE, 10U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortE, 11U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortE, 12U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortE, 13U, gpioModeInput, 0);

	CMU_ClockEnable(cmuClock_ADC0, true);

	for (;;) {
		for(int i = 0; i < 8; i++)
		{
			SwitchToChannel(i);
			OSTimeDly(50, OS_OPT_TIME_DLY, &err);

			ADC_InitSingle_TypeDef ADC0Init = ADC_INITSINGLE_DEFAULT;
			ADC0Init.reference = adcRef2V5;
			ADC0Init.posSel = getADCChannel(i);
			ADC_InitSingle(ADC0, &ADC0Init);
			ADC_Start(ADC0, adcStartSingle);

			while ( ! (ADC0->STATUS & ADC_STATUS_SINGLEDV));
			ADC[i].Value = ADC_DataSingleGet(ADC0);

			float temp10 = 25 * ADC[i].Value / 4096;

			temp10 = temp10 * 62 / 1.25;
			temp10 -= 10;

			if(temp10 > 999)  temp10 = 999;
			if(temp10 < 0)  temp10 = 0;

			CurOhm[i] = temp10;
		}

		SettingOhm = CurOhm[0];

		OSTimeDly(1000,  /*   1000 OS Ticks  */
		OS_OPT_TIME_DLY, /*   from now.      */
		&err);
	}
}

void WorkAsHumiture(unsigned char offset)
{
	static unsigned char SensorErr = 0;

	if ((offset % 20) == 0) {
		if ((!Sensor_AnswerFlag) || Sensor_ErrorFlag) {
			if (SensorErr < 100)
				SensorErr++;
		} else {
			SensorErr = 0;
		}
	}

	if (SensorErr >= 2) {
		if ((offset % 10) == 0) {
			Beep();
			DrawLogo();
		} else if ((offset % 10) == 5) {
			Beep();
			DrawIPC();
		}
	} else {
		Drawhumiture(H, T);
	}
}

unsigned char g_index = 0;
void WorkAsGroundChecker(unsigned char offset)
{
	if((offset % 5) == 0)
	{
		for(int i = 0; i< 8; i++)
		{
			if(CurOhm[i] > UserData.AlertOhm)
			{
				Beep();
			}
		}
	}


	if((offset % 4) == 0)
	{

		g_index ++;
		if( g_index >= 8) g_index = 0;
		switch(GndChkState)
		{
			case GndChk_Normal:
				DrawGround(UserData.AlertOhm, CurOhm[g_index]);
				Draw4816DotNumEx(false, 48, 3, g_index);
				break;

			case GndChk_Setting:
				if((g_index % 2) == 0) {
					CleanGndAlert();
				} else {
					DrawGround(SettingOhm, CurOhm[g_index]);
					Draw4816DotNumEx(false, 48, 3, g_index);
				}
				break;
		}
	}

	RTOS_ERR err;
	OS_MSG_SIZE IsLongPressing;
	char *key = OSQPend(&KeyQ, 0, OS_OPT_PEND_NON_BLOCKING, &IsLongPressing, NULL, &err);
	if((err.Code == RTOS_ERR_NONE) && key) {
		switch(GndChkState)
		{
			case GndChk_Normal:
				if(IsLongPressing)
				{
					GndChkState = GndChk_Setting;
					Beep();
				}
				break;

			case GndChk_Setting:
				GndChkState = GndChk_Normal;
				UserData.AlertOhm = SettingOhm;
				saveUserData();
				Beep();
				break;
		}
	}
}

//这个理论上应该移动到 bios bsp 才对
unsigned char getKeys(void)
{
	unsigned char Keys = 0;
	//Keys = ! GPIO_PinInGet(gpioPortD, 7U);
	Keys = ! GPIO_PinInGet(gpioPortF, 2U);

	return Keys;
}

void KeyTask(void *p_arg) {
	RTOS_ERR err;
	unsigned char LastKey;
	unsigned char CurKey;
	unsigned int KeyTime = 0;

	LastKey = CurKey = getKeys();

	for (;;) {

		CurKey = getKeys();

		if(LastKey != CurKey) {
			KeyTime = 0;
			OSQPost(&KeyQ, CurKey, 0, OS_OPT_POST_FIFO, &err);
		} else {
			if(CurKey){
				KeyTime++;
				if(KeyTime == 10)
					OSQPost(&KeyQ, CurKey, 1, OS_OPT_POST_FIFO, &err);
			}
		}

		LastKey = CurKey;
		OSTimeDly(200, OS_OPT_TIME_DLY, &err);
	}
}

void OLEDTask(void *p_arg) {
	RTOS_ERR err;
	CleanOLED();
	DrawLogo();
	OSTimeDly(1000, OS_OPT_TIME_DLY, &err);
	DrawIPC();
	OSTimeDly(1000, OS_OPT_TIME_DLY, &err);

	for (unsigned char offset = 0;; offset++)
	{
		if(offset >= 128) offset = 0;

		WorkAsGroundChecker(offset);
		OSTimeDly(200, OS_OPT_TIME_DLY, &err);
	}
}

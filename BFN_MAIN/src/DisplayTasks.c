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
unsigned int CurOhm[32];


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
|* ���ܣ� ��ʱ	����Ϊ12Mʱ					*|
|* ��ʱ��Լ 1ms			    				*|
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
|* ���ܣ� �����������͵ĵ����ֽ�	        *|
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
		while (!Sensor_SDA)	//����ϴε͵�ƽ�Ƿ����
		{
			if (++cnt >= 3000) {
				break;
			}
		}
		//��ʱMin=26us Max50us ��������"0" �ĸߵ�ƽ
		Delay_Nus(40);	 //��ʱ30us

		//�жϴ�������������λ
		tmp = 0;
		if (Sensor_SDA) {
			tmp = 1;
			//Delay_Nus(30);
		}
		cnt = 0;
		while (Sensor_SDA)		//�ȴ��ߵ�ƽ ����
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
		while (!Sensor_SDA)	//����ϴε͵�ƽ�Ƿ����
		{
			if (++cnt >= 3000) {
				break;
			}
		}
		debugCnt0[index0++] = cnt;
		cnt = 0;
		while (Sensor_SDA)		//�ȴ��ߵ�ƽ ����
		{
			if (++cnt >= 3000) {
				break;
			}
		}

		//�жϴ�������������λ
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
|* ���ܣ� ��������              	        *|
 \********************************************/
unsigned char Read_Sensor(void) {
	unsigned char i;
	unsigned int Sys_CNT;
	CORE_irqState_t irqState ;

	//��������(Min=800US Max=20Ms)
	Clear_Sensor_SDA();
	Delay_N1ms(2);  //��ʱ2Ms

	//�ͷ����� ��ʱ(Min=30us Max=50us)
	Set_Sensor_SDA();
	Turn_Sensor_SDA_Input();
#if 1
	Delay_Nus(20);  //��ʱ30us
	CPU_CRITICAL_ENTER();

	//������Ϊ���� �жϴ�������Ӧ�ź�


	Sensor_AnswerFlag = 0;  // ��������Ӧ��־
	Sensor_ErrorFlag = 1;

	//�жϴӻ��Ƿ��е͵�ƽ��Ӧ�ź� �粻��Ӧ����������Ӧ����������
	if (Sensor_SDA == 0) {
		Sensor_AnswerFlag = 1;  //�յ���ʼ�ź�
		Sys_CNT = 0;

		//�жϴӻ��Ƿ񷢳� 80us �ĵ͵�ƽ��Ӧ�ź��Ƿ����
		while ((!Sensor_SDA)) {
			if (++Sys_CNT > 300) //��ֹ������ѭ��
			{
				goto end;
			}
		}
		Sys_CNT = 0;
		//�жϴӻ��Ƿ񷢳� 80us �ĸߵ�ƽ���緢����������ݽ���״̬
		while ((Sensor_SDA)) {
			if (++Sys_CNT > 300) //��ֹ������ѭ��
			{
				goto end;
			}
		}
#endif
		// ���ݽ���	������������40λ����
		// ��5���ֽ� ��λ����  5���ֽڷֱ�Ϊʪ�ȸ�λ ʪ�ȵ�λ �¶ȸ�λ �¶ȵ�λ У���
		// У���Ϊ��ʪ�ȸ�λ+ʪ�ȵ�λ+�¶ȸ�λ+�¶ȵ�λ
		for (i = 0; i < 5; i++) {
			Sensor_Data[i] = Read_SensorData();
		}
		Sensor_ErrorFlag = 0;	//��������

#if 1
	} else {
		Sensor_AnswerFlag = 0;	  // δ�յ���������Ӧ
	}
#endif

end:
	CPU_CRITICAL_EXIT();
	return ! Sensor_ErrorFlag;
}

void SensorTask(void *p_arg) {
	RTOS_ERR err;
	unsigned char sum;
	int i;
	uint32_t Result;

	ADC_InitScan_TypeDef ADC0Init = ADC_INITSCAN_DEFAULT;
	GPIO_PinModeSet(gpioPortD, 4U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortD, 5U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortD, 6U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortD, 7U, gpioModeInput, 0);
	GPIO_PinModeSet(gpioPortE, 12U, gpioModeInput, 0);

	CMU_ClockEnable(cmuClock_ADC0, true);
	ADC0Init.reference = adcRefVDD;
	ADC0Init.acqTime = adcAcqTime256;
	ADC0Init.diff = false;

	ADC[0].ID = ADC_ScanSingleEndedInputAdd(&ADC0Init, adcScanInputGroup0,
			adcPosSelAPORT0XCH4);
	ADC[1].ID = ADC_ScanSingleEndedInputAdd(&ADC0Init, adcScanInputGroup0,
			adcPosSelAPORT0XCH5);
	ADC[2].ID = ADC_ScanSingleEndedInputAdd(&ADC0Init, adcScanInputGroup0,
			adcPosSelAPORT0XCH6);
	ADC[3].ID = ADC_ScanSingleEndedInputAdd(&ADC0Init, adcScanInputGroup0,
			adcPosSelAPORT0XCH7);
	ADC[4].ID = ADC_ScanSingleEndedInputAdd(&ADC0Init, adcScanInputGroup1,
			adcPosSelAPORT3XCH12);

	ADC_InitScan(ADC0, &ADC0Init);

	for (;;) {
		index = 0;
		index0 = 0;

		if( Read_Sensor()){
			sum = 0;
			for(i = 0; i < 4; i++) {
				sum += Sensor_Data[i];
			}

			if(sum == Sensor_Data[4]) {
				H = ((unsigned int)Sensor_Data[0] << 8) + Sensor_Data[1];
				T = ((unsigned int)Sensor_Data[2] << 8) + Sensor_Data[3];
			}
		} else {
			H = T = 0;
		}

		ADC_Start(ADC0, adcStartScan);
		OSSchedLock(&err);
		while ( ! (ADC0->STATUS & ADC_STATUS_SCANDV));
		while ((ADC0->STATUS & ADC_STATUS_SCANDV)) {
			Result = ADC0->SCANDATAX;
			for (i = 0; i < ADCChannels; i++) {
				if (ADC[i].ID == (Result >> 16)) {
					ADC[i].Value = Result & 0xFFFF;
				}
			}
		}
		OSSchedUnlock(&err);


		uint32_t delta = 0;

		for(int i = 0; i < 4; i++)
		{
			if(ADC[4].Value > ADC[i].Value){
				delta = ADC[4].Value - ADC[i].Value;
			}

			float temp10 = 1000 * ADC[i].Value;

			if(delta == 0) {
				temp10 = 999;
			} else {
				temp10 /= delta;
				//temp10 -= 10;
			}

			if(temp10 > 999)  temp10 = 999;

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

unsigned char g_index = 0xFF;
void WorkAsGroundChecker(unsigned char offset)
{
	if((offset % 5) == 0)
	{
		for(int i = 0; i<4; i++)
		{
			if(i >= UserData.subCount) continue;
			if(CurOhm[i] > UserData.AlertOhm)
			{
				Beep();
			}
		}
	}


	if((offset % 4) == 0)
	{

		g_index ++;
		if( g_index >= 4) g_index = 0;
		if( g_index >= UserData.subCount ) g_index = 0;
		switch(GndChkState)
		{
			case GndChk_Normal:
				DrawGround(UserData.AlertOhm, CurOhm[g_index]);
				Draw4816DotNumEx(false, 48, 3, g_index + 1);
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

#if 0
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
#endif

}

//���������Ӧ���ƶ��� bios bsp �Ŷ�
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

		switch(UserData.state)
		{
			case SysState_humiture:
				WorkAsHumiture(offset);
				//DrawHanzi(offset);
				break;
			case SysState_ground:
				WorkAsGroundChecker(offset);
				break;
			default:
				if(offset % 32 >= 16) {
					cleanHanzi();
					WorkAsHumiture(offset);
					//DrawHanzi(offset);
					g_index = 0xFF;
				} else {
					WorkAsGroundChecker(offset);
				}
				break;
		}

		OSTimeDly(200, OS_OPT_TIME_DLY, &err);
	}
}

typedef struct {
	uint32_t ID;
	uint32_t Value;
} ADCResult;

void SensorTask(void *p_arg);
void OLEDTask(void *p_arg);
void KeyTask(void *p_arg);
unsigned int getLastH(void);
unsigned int getCurOhm(void);
unsigned int getLastT(void);

unsigned int getCurOhmInt(unsigned char index);
unsigned int getAlertOhmInt(void);

#define ADCChannels		(5)
extern ADCResult ADC[ADCChannels];

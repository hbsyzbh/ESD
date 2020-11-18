#include "em_device.h"
#include "em_chip.h"
#include "em_timer.h"
#include "em_cmu.h"
#include "hal-config.h"
#include "em_adc.h"
#include  <bsp_os.h>
#include  "bsp.h"

#include  <cpu/include/cpu.h>
#include  <common/include/common.h>
#include  <kernel/include/os.h>

#include  <common/include/lib_def.h>
#include  <common/include/rtos_utils.h>
#include  <common/include/toolchains.h>
#include  "Maxdone_SPI_OLDE.h"
#include  "Maxdone_UART.h"
#include  "UartTasks.h"
#include  "DisplayTasks.h"

#define  EX_MAIN_START_TASK_PRIO              21u
#define  EX_MAIN_START_TASK_STK_SIZE         256u
#define  USER_TASK_STK_SIZE					 320u

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                        LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 *********************************************************************************************************
 */

/* Start Task Stack.                                    */
static CPU_STK Ex_MainStartTaskStk[EX_MAIN_START_TASK_STK_SIZE];

/* Start Task TCB.                                      */
static OS_TCB Ex_MainStartTaskTCB;

#define USER_TASK_COUNT		(6)
static CPU_STK UserTaskStk[USER_TASK_COUNT][USER_TASK_STK_SIZE];
static OS_TCB UserTaskTCB[USER_TASK_COUNT];
const int UserTaskPrio[USER_TASK_COUNT] = { 10, 19, 18, 5, 22, 23 };


OS_TASK_PTR UserTasks[USER_TASK_COUNT] = { OLEDTask, E22UartTask, USBUartTask,
		SensorTask, BeepTask, KeyTask };

const char *UserTaskNames[USER_TASK_COUNT] = { "OLEDTask", "E22UartTask",
		"USBUartTask", "SensorTask", "BeepTask", "KeyTask" };

void CreateUserTasks(void) {
	RTOS_ERR err;
	int i;

	for (i = 0; i < USER_TASK_COUNT; i++) {
		OSTaskCreate(&(UserTaskTCB[i]), /* Create the Start Task.                               */
		UserTaskNames[i], UserTasks[i],
		DEF_NULL, UserTaskPrio[i], &UserTaskStk[i][0],
				(USER_TASK_STK_SIZE / 10u),
				USER_TASK_STK_SIZE, 0u, 0u,
				DEF_NULL, (OS_OPT_TASK_STK_CLR ), &err);
	}
}

/*
 *********************************************************************************************************
 *                                          Ex_MainStartTask()
 *
 * Description : This is the task that will be called by the Startup when all services are initializes
 *               successfully.
 *
 * Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
 *
 * Return(s)   : None.
 *
 * Notes       : None.
 *********************************************************************************************************
 */

static void Ex_MainStartTask(void *p_arg) {
	RTOS_ERR err;

	PP_UNUSED_PARAM(p_arg); /* Prevent compiler warning.                            */

	BSP_TickInit(); /* Initialize Kernel tick source.                       */

#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
	OSStatTaskCPUUsageInit(&err); /* Initialize CPU Usage.                                */
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
	CPU_IntDisMeasMaxCurReset(); /* Initialize interrupts disabled measurement.          */
#endif

	Common_Init(&err);
	APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

	BSP_OS_Init(); /* Initialize the BSP. It is expected that the BSP ...  */
	/* ... will register all the hardware controller to ... */
	/* ... the platform manager at this moment.             */

	UartTasksInit();
	CreateUserTasks();
	while (DEF_ON) {
		CPU_STK_SIZE used,free;

		//BSP_LedToggle(0);

//		for (int i = 0; i < USER_TASK_COUNT; i++) {
//			OSTaskStkChk(&UserTaskTCB[i], &free, &used, &err);
//		}

		/* Delay Start Task execution for                       */
		OSTimeDly(1000, /*   1000 OS Ticks                                      */
		OS_OPT_TIME_DLY, /*   from now.                                          */
		&err);
		/*   Check error code.                                  */
		APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
	}
}

void MD_BEEP_init(void) {
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

	timerInit.enable = false;
	timerCCInit.mode = timerCCModePWM;

	CMU_ClockEnable(cmuClock_TIMER1, true);
	TIMER_InitCC(TIMER1, 0, &timerCCInit);
	TIMER1->ROUTEPEN = TIMER_ROUTEPEN_CC0PEN;
	TIMER1->ROUTELOC0 = PORTIO_TIMER1_CC0_LOC;
	TIMER_TopSet(TIMER1, 4750);
	TIMER_CompareBufSet(TIMER1, 0, 2375);
	TIMER_Init(TIMER1, &timerInit);

	GPIO_PinModeSet( PORTIO_TIMER1_CC0_PORT, PORTIO_TIMER1_CC0_PIN,
			gpioModePushPull, 1);
}

int main(void) {
	RTOS_ERR err;

	/* Chip errata */
	CHIP_Init();

	MD_OLED_init();
	MD_KEY_init();
	MD_UART_init();
	MD_BEEP_init();
	MD_74HC4051_init();
	BSP_SystemInit(); /* Initialize System.                                   */
	CPU_Init(); /* Initialize CPU.                                      */

	OSInit(&err); /* Initialize the Kernel.                               */
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSTaskCreate(&Ex_MainStartTaskTCB, /* Create the Start Task.                               */
	"Ex Main Start Task", Ex_MainStartTask,
	DEF_NULL,
	EX_MAIN_START_TASK_PRIO, &Ex_MainStartTaskStk[0],
			(EX_MAIN_START_TASK_STK_SIZE / 10u),
			EX_MAIN_START_TASK_STK_SIZE, 0u, 0u,
			DEF_NULL, (OS_OPT_TASK_STK_CLR ), &err);
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSStart(&err); /* Start the kernel.                                    */
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	return (1);
}

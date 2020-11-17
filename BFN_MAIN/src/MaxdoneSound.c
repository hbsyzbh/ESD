//#include "MaxdoneSound.h"
#include  <kernel/include/os.h>
#include  <common/include/rtos_utils.h>
#include "em_timer.h"

#if 0
/*
C	261.63
C#	277.18
D	293.66
D#	311.13
E	329.63
F	349.23
F#	369.99
G	392.00
G#	415.30
A	440.00
A#	466.16
B	493.88
*/

double SoundFreq[] = {
		261.63,
		277.18,
		293.66,
		311.13,
		329.63,
		349.23,
		369.99,
		392.00,
		415.30,
		440.00,
		466.16,
		493.88
};

#define C_1			0
#define C_1s		1
#define C_2 		2
#define C_2s		3
#define C_3 		4
#define C_4 		5
#define C_4s		6
#define C_5 		7
#define C_5s		8
#define C_6 		9
#define C_6s		10
#define C_7 		11
#define C_MUTE		255


#define beat_Eighth_time 125

#define beat_Long   16
#define beat_Full	8
#define beat_Half	4
#define beat_Quter	2
#define beat_Eighth	1

typedef struct {
	char BaseScale;
	char Time;
	char HiLow;
}	MUSIC;


MUSIC HappyNewYear[] ={
	{C_1, beat_Half}, {C_1, beat_Half}, {C_1, beat_Full}, {C_5, beat_Full, -1},
	{C_3, beat_Half}, {C_3, beat_Half}, {C_3, beat_Full}, {C_1, beat_Full},

};

void PlayMusic(MUSIC *music, int size){
	RTOS_ERR  err;
	int i;
	int len = size / sizeof(MUSIC);
	int time;
	int Freq;
	double temp;

	for(i = 0; i < len; i++ ) {
		time = music[i].Time * beat_Eighth_time;
		if(music[i].BaseScale != C_MUTE) {
			temp = SoundFreq[music[i].BaseScale];
			if(music[i].HiLow >= 0) {
				temp =  temp * (double)( 1 <<  music[i].HiLow);
			} else {
				temp =  temp * (double)( 1 <<  (0 -music[i].HiLow));
			}

			Freq = (double)1000.0 * (double)20000.0/ temp;

			TIMER_TopSet(TIMER1, Freq);
			TIMER_CompareBufSet(TIMER1, 0, Freq/2);
			TIMER_Enable(TIMER1, true);
		}
        OSTimeDly( time, OS_OPT_TIME_DLY, &err);
        TIMER_Enable(TIMER1, false);

	}

}

PlayMusic(HappyNewYear, sizeof(HappyNewYear));
#endif

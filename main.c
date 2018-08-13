#define __MAIN_C__


#include "main.h"
#include "board.h"
#include "cpu.h"
#include "Common.h"

void delay200us ( U16 usdelay )
{
	U16 i, j;

	for ( j = usdelay; j > 0; j-- )
	{
		for ( i = 465; i > 0; i-- )
		{
			//
		}
	}
}


void address_init ( void )
{
	U8 u8tmp;

	do
	{
		u8tmp = PIN_DOOR_ADDRESS; /*�ڶ�������*/
		delay200us ( 2 );
	}
	while ( u8tmp != PIN_DOOR_ADDRESS );

	if ( 0 == u8tmp )
	{
		gucNetworkLocalAddress = DOOR2_ADDRESS;
	}
	else
	{
		gucNetworkLocalAddress = DOOR1_ADDRESS;
	}
}


void Task20ms ( void )
{
	static BYTE ucKeyPressLongCount = 0;
	static BYTE ucKeyPressLimit     = 0;

	BYTE ucKeyValue = 0;
	static U8 u8TimeCnt = 0;

	u8TimeCnt++;

	if ( u8TimeCnt >= 5 ) // 100ms do once
	{
		u8TimeCnt = 0;

		ResetStateRecovery();
		SendCamAddress();

		if ( gu8SendResetTimeCnt )
		{
			gu8SendResetTimeCnt--;
		}
	}

	//if(OUTDOOR_STATUS_TALKING != gucOutdoorStatus)
	{
		ucKeyValue = KeyScanning();
	}

	/* �����ſڻ���������*/
	if ( ucKeyPressLimit > 0 )
	{
		ucKeyPressLimit--;

		if ( ucKeyPressLimit > 0 )
		{
			ucKeyValue = 0xFF;
		}
	}

	/* ̧�����*/
	if ( ucKeyValue == 0x55 )
	{
		ucKeyPressLimit = 100;
		PIN_34118_EN = LOW;
		NetworkTxProcess ( BROADCAST_ADDR, CMD_NETWORK_OUTDOOR_CALL_OUT );
	}

	/* �������*/
	if ( ucKeyValue == 0xAA )
	{
		ucKeyPressLongCount++;

		if ( ucKeyPressLongCount == 100 )
		{
			ucKeyPressLongCount = 0;
			PIN_34118_EN = LOW;
			NetworkTxProcess ( BROADCAST_ADDR, CMD_NETWORK_OUTDOOR_CALL_OUT );
		}
	}

	if ( ucKeyValue != 0xAA )
	{
		ucKeyPressLongCount = 0;
	}

	if ( gucCallWaitTime > 0 )
	{
		gucCallWaitTime--;
	}

	if ( gucRingTimeCnt > 1 )
	{
		gucRingTimeCnt--;

		if ( ( 1 == gucRingTimeCnt ) && ( OUTDOOR_STATUS_STANDBY != gucOutdoorStatus ) )
		{
			Set34118MuteFlag ( LOW );
		}
	}

	if ( gu8WaitStateCnt )
	{
		gu8WaitStateCnt--;
		if ( 0 == gu8WaitStateCnt )
		{
			if ( OUTDOOR_STATUS_STANDBY != gucOutdoorStatus )
			{
				Standby();
			}
		}
	}

	CameraLEDAdaptiveControl ( CTL_CAM_LED_DET );

	OpenDoor ( NULL, STATE_DOOR_DET_ON, NULL );

}


void Task1s ( void )
{
	NetworkStatusTimeoutProcess();
}

void RoundRobinScheduling ( void )
{
	static U8 u8timecnt = 0;

	if ( guc1msflg >= 20 )
	{
		guc1msflg = 0;
		Task20ms();

		u8timecnt++;
		if ( 0 == ( u8timecnt % 5 ) ) // 100ms
		{
			//SendCamAddress();
		}

		if ( u8timecnt >= 50 ) // 1s
		{
			u8timecnt = 0;
			Task1s();
		}
	}
}

void InitSystem ( void )
{
	InitCPU();

	HardReset();

	address_init();

}

void main ( void )
{
	InitSystem();

	WDog_Init();
	while ( 1 )
	{
		RoundRobinScheduling();

		ManchesterTxProcess();

		ManchesterRxProcess();

		if ( ManchesterCollisionDetectionFlag() == 1 )
		{
			PIN_TALK_OFF = 1;
			PIN_34118_MUTE = 1;
		}
		else
		{
			PIN_TALK_OFF = 0;
			Set34118MuteFlag ( Get34118MuteFlag() );
		}

		set_WDCLR;/*���WDT�Ĵ���*/
	}
}



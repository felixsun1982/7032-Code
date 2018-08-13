#define __BOARD_C__


#include "board.h"
#include "cpu.h"
#include "network.h"
#include "manchester.h"
#include "Common.h"

BYTE gucOutdoorStatus = OUTDOOR_STATUS_STANDBY;

U16 gucOutdoorStatusTimeoutCounter = 0;

/* 外呼后，记录进入振铃状态的室内机数量*/
U8 u8MonitorRingNum = 0;

U8 u8CallFlag = 0;

/*
一台门口机外呼后，
限制一定时间后再允许其他门口机外呼
*/
BYTE gucCallWaitTime = 0;

/* 记录最近一个开锁的室内机*/
BYTE u8OpenAdder = 0;


BYTE gucRingTimeCnt = 0;

BYTE gu834118MuteFlag = 1;

void Set34118MuteFlag( BYTE en )
{
    if(gucManchesterTxFlag)
    {
        PIN_34118_MUTE = 1;
        PIN_AUDIO_MSG_EN = 0;
    }
    else
    {
        PIN_34118_MUTE = en;
        PIN_AUDIO_MSG_EN = en;
    }

	gu834118MuteFlag = en;
}

BYTE Get34118MuteFlag( void )
{
    return gu834118MuteFlag;
}

void HardReset( void )
{    
    PIN_34118_EN = 1;
	Set34118MuteFlag(HIGH);

    PIN_KEY_OPEN     = LOW;
    PIN_DOOR_OPEN    = LOW;
    
    PIN_CAM_POWER    = HIGH;
    PIN_72910_CTL    = HIGH;
    PIN_DOOR_LED     = LOW;
	PIN_DOOR_LED2    = LOW;
	PIN_CAM_LED      = LOW;
	
    PIN_TALK_OFF     = LOW;

    PIN_386_EN       = HIGH;
	
    CameraLEDAdaptiveControl(CTL_CAM_LED_OFF);
}


void OtherDoorCall( void )
{
	PIN_386_EN		 = HIGH;
	PIN_DOOR_LED	 = LOW;
	
    PIN_34118_EN = 1;
	Set34118MuteFlag( HIGH );
	
	gucOutdoorStatus = OUTDOOR_STATUS_STANDBY;
	gucOutdoorStatusTimeoutCounter = 0;
	CameraLEDAdaptiveControl(CTL_CAM_LED_OFF);
	OpenDoor( DOOR_OPEN1, STATE_DOOR_CLOSE, 0 );
	OpenDoor( DOOR_OPEN2, STATE_DOOR_CLOSE, 0 );
}


void Standby( void )
{
    //SendBytes("void Standby( void )\n");
	u8MonitorRingNum = 0;
	
    PIN_CAM_POWER    = LOW;
    PIN_72910_CTL    = HIGH;
    PIN_386_EN       = HIGH;
    PIN_DOOR_LED     = LOW;
    	
    PIN_34118_EN = 1;
	Set34118MuteFlag( HIGH );
	
    gucOutdoorStatus = OUTDOOR_STATUS_STANDBY;
    gucOutdoorStatusTimeoutCounter = 0;

    CameraLEDAdaptiveControl(CTL_CAM_LED_OFF);
	OpenDoor( DOOR_OPEN1, STATE_DOOR_CLOSE, 0 );
	OpenDoor( DOOR_OPEN2, STATE_DOOR_CLOSE, 0 );

	Stm8sTimerDelay(50);
    PIN_CAM_POWER = HIGH;
}


void Monitor( void )
{
    //SendBytes("void Monitor( void )\n");   
	u8CallFlag       = 1;
	u8MonitorRingNum = 0;
	
    PIN_34118_EN = 0;
	Set34118MuteFlag(LOW);
		
    PIN_72910_CTL    = LOW;
    PIN_CAM_POWER    = HIGH;
	
    PIN_386_EN       = HIGH;
	
    gucOutdoorStatus = OUTDOOR_STATUS_MONITOR;
	gucOutdoorStatusTimeoutCounter = 600;

    CameraLEDAdaptiveControl( CTL_CAM_LED_ON );
}


void Ringing( void )
{
	//SendBytes("void Ringing( void )\n");   
	
	u8CallFlag      = 1;
	
    PIN_34118_EN = 0;
	
	if(0 == gucRingTimeCnt)
	{
		Set34118MuteFlag(HIGH);
		gucRingTimeCnt = 100;
	}

	PIN_72910_CTL	= LOW;
	PIN_CAM_POWER	= HIGH;

	PIN_386_EN       = LOW;
	
	gucOutdoorStatus = OUTDOOR_STATUS_RINGING;
	gucOutdoorStatusTimeoutCounter = 600;
	
    CameraLEDAdaptiveControl( CTL_CAM_LED_ON );
}


void Talking( void )
{    
    //SendBytes("void Talking( void )\n");
	u8CallFlag       = 1;
	u8MonitorRingNum = 0;

    PIN_34118_EN = 0;
	Set34118MuteFlag( LOW );
	
    PIN_72910_CTL    = LOW;
    PIN_CAM_POWER    = HIGH;
    
	PIN_386_EN       = LOW;
	
    gucOutdoorStatus = OUTDOOR_STATUS_TALKING;
	gucOutdoorStatusTimeoutCounter = 600;
	
    CameraLEDAdaptiveControl( CTL_CAM_LED_ON );
}


static U8 get_light_state(U8 u8temp) 
{
    static U8 u8state = 0;

    if(u8temp < 0xA0)
    {
		u8state = 1;
    }

    if(u8state)
    {
        if(u8temp > 0xA8)
        {
		    u8state = 0;
		}
    }

    return u8state;
}

/*
控制摄像头的LED，用于补光或红外
监控检测是否打开LED，之后退出关闭LED
*/
void CameraLEDAdaptiveControl( BYTE ucMode )
{
    U8  ucTemp = 0;
    static U8 i = 0;
    static U8 ucstate = 0;
    
    if( CTL_CAM_LED_ON == ucMode )  
    {
        if(HIGH == ucstate)
        {
			PIN_CAM_LED = HIGH;
			ucstate = 0xFF; //开灯后停止检测
        }
    }
    else
    {
        if(CTL_CAM_LED_DET == ucMode)
        {
			if(0xFF == ucstate) return;
			
            ucTemp = ReadAD4(); 
			
            //if( ucTemp < 0x90 )
			if(get_light_state(ucTemp))
            {
				if( i < 20 )
				{
					i++;
				}

				if(20 == i)
				{
					ucstate = HIGH;
					PIN_DOOR_LED2 = 1;
					if(OUTDOOR_STATUS_STANDBY != gucOutdoorStatus)
					{
						PIN_CAM_LED = HIGH;
						PIN_DOOR_LED = 1;
						ucstate = 0xFF; //开灯后停止检测
					}
					else
					{
						PIN_DOOR_LED = 0;
					}
				}
            }
            else
            {
                i = 0;
				ucstate = LOW;
				PIN_DOOR_LED2 = 0;
				PIN_DOOR_LED = 0;
            }
        }
        else if(CTL_CAM_LED_OFF == ucMode)
        {
            i = 0;
			ucstate = LOW;
            PIN_CAM_LED = LOW;
        }
    }
    
	return;
}


void DoorActionOpen( BYTE ucMode )
{
    if( DOOR_OPEN1 == ucMode )
    {
		PIN_DOOR_OPEN = LOW;
        PIN_KEY_OPEN = HIGH;
    }
    else
    {
		PIN_KEY_OPEN  = LOW;
        PIN_DOOR_OPEN = HIGH;
    }
}

void DoorActionClose( void )
{
	PIN_DOOR_OPEN = LOW;
	PIN_KEY_OPEN  = LOW;
}

void OpenDoor( BYTE ucMode, BYTE ucAction, U32 ulTime )
{
    static U32 ulOpenTime = NULL;

    if(STATE_DOOR_CLOSE == ucAction)
    {
		ulOpenTime = 0;
		DoorActionClose();
    }
    else if(STATE_DOOR_OPEN == ucAction)
    {
		ulOpenTime = ulTime;
		DoorActionOpen( ucMode );
    }
    else if(STATE_DOOR_DET_ON == ucAction)
    {
		if( ulOpenTime ) 
		{
			ulOpenTime--;
		
			if( 0 == ulOpenTime ) 
			{
				DoorActionClose();
			}
		}
    }
    
}

U8 gu8SendResetTimeCnt = 0;
U8 gu8ResetTimeCnt = 0;

#if 1
void ResetStateRecovery(void)
{
    if(gu8ResetTimeCnt < 6)
    {
        gu8ResetTimeCnt++;

        if(3 == gu8ResetTimeCnt)
        {
            if(gucNetworkLocalAddress == DOOR1_ADDRESS)
            {
                gu8SendResetTimeCnt = 20;
                NetworkTxProcess(BROADCAST_ADDR, NET_MONITOR_RESET);
            }
        }
        else if(6 == gu8ResetTimeCnt)
        {
            if(gucNetworkLocalAddress == DOOR2_ADDRESS)
            {
                gu8SendResetTimeCnt = 20;
                NetworkTxProcess(BROADCAST_ADDR, NET_MONITOR_RESET);
            }
        }
    }
}
#endif

U8 gu8StartTime = 0;

void SendCamAddress(void)
{
    if(gu8StartTime < 20)
    {
        gu8StartTime++;
        if(10 == gu8StartTime)
        {
            if(DOOR1_ADDRESS == gucNetworkLocalAddress)
            {
                NetworkTxProcess(BROADCAST_ADDR, NET_ADDRESS_CAM1);
            }
        }
        else if(14 == gu8StartTime)
        {
            if(DOOR2_ADDRESS == gucNetworkLocalAddress)
            {
                NetworkTxProcess(BROADCAST_ADDR, NET_ADDRESS_CAM2);
            }
        }
    }
}







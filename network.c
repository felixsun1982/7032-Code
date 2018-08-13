#define __NETWORK_C__

#include "main.h"
#include "cpu.h"
#include "board.h"
#include "manchester.h"
#include "network.h"
#include "Common.h"


WORD guiNetworkMsgTxBackup      = 0;                    /*  备份发送数据，用于重发机制*/
WORD guiNetworkMsgRxBackup      = 0;                    /*  备份接收数据，用于回应机制*/
WORD guiNetworkMonitorData      = 0;                    /*  网络消息监控，消息捕获机制*/

/* 等待回复网络状态计时   单位20ms*/
BYTE gu8WaitStateCnt = 0;

BYTE gucNetworkLocalAddress     = DOOR1_ADDRESS;

const BYTE AdressFlag[2][3] = {
{0x01,0x02,0x04},
{0xFE,0xFD,0xFB}
};

WORD GetNetworkMsgTxBackup( void )
{
    return guiNetworkMsgTxBackup;
}


void SetNetworkMsgTxBackup( WORD uiData )
{
    guiNetworkMsgTxBackup = uiData;
}


U8 CalCrcProcess(U16 u16Commd)
{
    U16 i = 0;
    U8 u8Crc = 0;
    U8 u8CrcTmp = 0;
    
    for(i = 0; i < 6; i++)
    {
        if(u16Commd & (0x01 << i))
        {
			u8CrcTmp++;
        }
    }

    if(u8CrcTmp % 2)
    {
		u8Crc |= 0x01;
    }
    else
    {
		u8Crc &= 0xFE;
    }
    
	u8CrcTmp = 0;
	for(i = 0; i < 7; i++)
	{
		if( u16Commd & (0x01 << (i + 6)) )
		{
			u8CrcTmp++;
		}
	}

	if(u8CrcTmp % 2)
	{
		u8Crc |= 0x02;
	}
	else
	{
		u8Crc &= 0xFD;
	}

	return u8Crc;
}

COMMAND_FORMAT NetworkMsgDecode( WORD uiData )
{
    BYTE ucByteH = 0, ucByteL = 0;

    COMMAND_FORMAT tCommand;

    ucByteH = (BYTE)(uiData >> 0x08);
    ucByteL = (BYTE)(uiData  & 0xFF);
    
#if 0
    tCommand.crc  = (ucByteH >> 5) & 0x03;
    tCommand.sender     = (ucByteH >> 2) & 0x07;
    tCommand.receiver   = ((ucByteH & 0x03) << 0x01 ) | (ucByteL >> 0x07);
    tCommand.parameter  = ucByteL & 0x7F;
#endif

    tCommand.crc  = ((ucByteH >> 5) & 0x03) ^ 0x03;
    tCommand.sender     = ((ucByteH >> 2) & 0x07) ^ 0x07 ;
    tCommand.receiver   = (((ucByteH & 0x03) << 0x01 ) | (ucByteL >> 0x07) ) ^ 0x07;
    tCommand.parameter  = (ucByteL & 0x7F) ^ 0x7F;

    return tCommand;
}


WORD NetworkMsgEncode( COMMAND_FORMAT tCommand )
{
    BYTE ucByteH = 0, ucByteL = 0;

    WORD uiData  = 0;

#if 0
    tCommand.crc         = 0;
    tCommand.sender     &= 0x07;
    tCommand.receiver   &= 0x07;
    tCommand.parameter  &= 0x7F;
#endif

    tCommand.crc         = 0 ^ 0x03;
    tCommand.sender     = (tCommand.sender & 0x07) ^ 0x07;
    tCommand.receiver   = (tCommand.receiver & 0x07) ^ 0x07;
    tCommand.parameter  = (tCommand.parameter & 0x7F) ^ 0x7F;
    
    ucByteH = (tCommand.crc << 5) | (tCommand.sender << 2) | (tCommand.receiver >> 1);

    ucByteL = (tCommand.receiver << 7) | tCommand.parameter;

    uiData = (ucByteH << 8) | ucByteL;

	tCommand.crc = CalCrcProcess(uiData);
	
    ucByteH = (tCommand.crc << 5) | (tCommand.sender << 2) | (tCommand.receiver >> 1);

    ucByteL = (tCommand.receiver << 7) | tCommand.parameter;

    uiData = (ucByteH << 8) | ucByteL;

	//Send_Data_To_UART0(0x55);
	//Send_Data_To_UART0( (U8)(uiData >> 8) );
	//Send_Data_To_UART0( (U8)uiData);
	
    return uiData;
}


void NetworkTxProcess( BYTE ucReceiver, BYTE ucCommand )
{
    COMMAND_FORMAT tCommand;

    WORD uiData = 0;

    //tCommand.crc       = 0;
    tCommand.sender    = gucNetworkLocalAddress;
    tCommand.receiver  = ucReceiver;
    tCommand.parameter = ucCommand;

    uiData = NetworkMsgEncode(tCommand);

    ManchesterTxAdd(uiData);
}


void NetworkTxPostProcess( void )
{
    COMMAND_FORMAT tCommand;

    tCommand = NetworkMsgDecode(guiNetworkMsgTxBackup);

#if 0
    Uart2TxData(0xA0);
    Uart2TxData(tCommand.crc);
    Uart2TxData(tCommand.sender);
    Uart2TxData(tCommand.receiver);
    Uart2TxData(tCommand.parameter);
#endif

    switch(tCommand.parameter)
    {
        case CMD_NETWORK_REPORT_OWN_POWER_ON:
            break;

        default:
            break;
    }
    
    guiNetworkMsgTxBackup = 0;
}


void NetworkRxProcess( void )
{
    COMMAND_FORMAT tCommand;

    tCommand = NetworkMsgDecode(guiNetworkMsgRxBackup);

#if 0
    Uart2TxData(0xA1);
    Uart2TxData(tCommand.crc);
    Uart2TxData(tCommand.sender);
    Uart2TxData(tCommand.receiver);
    Uart2TxData(tCommand.parameter);
#endif

    switch(tCommand.parameter)
    {
        case CMD_NETWORK_OUTDOOR_STANDBY   :
			u8MonitorRingNum &= AdressFlag[1][tCommand.sender - MASTER_ADDRESS];

			if(tCommand.sender == u8OpenAdder)
			{
				OpenDoor( DOOR_OPEN1, STATE_DOOR_CLOSE, 0 );
				OpenDoor( DOOR_OPEN2, STATE_DOOR_CLOSE, 0 );
			}

            /* 告诉所有机器监控事件结束*/
			//if(0 == u8MonitorRingNum)
			{
				if(u8CallFlag)
				{
					Standby();
					NetworkTxProcess(BROADCAST_ADDR, CMD_NETWORK_OUTDOOR_CAM_STANDBY);
				}
			}
            break;

        case CMD_NETWORK_OUTDOOR_OPEN1     :
			u8OpenAdder = tCommand.sender;
            OpenDoor( DOOR_OPEN1, STATE_DOOR_OPEN, 75 );
            break;

        case CMD_NETWORK_OUTDOOR_CLOSE1    :
            OpenDoor( DOOR_OPEN1, STATE_DOOR_CLOSE, 0 );
            break;

        case CMD_NETWORK_OUTDOOR_CLOSE2    :
            OpenDoor( DOOR_OPEN2, STATE_DOOR_CLOSE, 0 );
            break;

        case CMD_NETWORK_OUTDOOR_OPEN2     :
			u8OpenAdder = tCommand.sender;
            OpenDoor( DOOR_OPEN2, STATE_DOOR_OPEN, 500 );
            break;
            
        case CMD_NETWORK_OUTDOOR_TALKING   :
		    if(OUTDOOR_STATUS_STANDBY == gucOutdoorStatus)
		    {
				Standby();
		    }
		    else
		    {
				Talking();
		    }

            if(gu8SendResetTimeCnt)
            {
                gu8ResetTimeCnt = 6;
				Talking();
            }
            
			break;
        case NET_MONITOR_WIFI_CAM:
            Talking();
            
            if(gu8SendResetTimeCnt)
            {
                gu8ResetTimeCnt = 6;
				Talking();
            }
            break;
        case NET_MONITOR_WIFI_CAM_END:
            Standby();
            break;

        case CMD_NETWORK_REPORT_OWN_OUTDOOR_CALL  :
            if(gu8SendResetTimeCnt)
            {
                gu8ResetTimeCnt = 6;
                u8MonitorRingNum |= AdressFlag[0][tCommand.sender - MASTER_ADDRESS];
			    Ringing();
			}
			
            break;
            
        default:
            break;
    }

    guiNetworkMsgRxBackup = 0;
}


void NetworkMonitorProcess( void )
{
    COMMAND_FORMAT tCommand;

    tCommand = NetworkMsgDecode(guiNetworkMonitorData);

#if 0
    Uart2TxData(0xA2);
    //Uart2TxData(tCommand.crc);
    //Uart2TxData(tCommand.sender);
    //Uart2TxData(tCommand.receiver);
    //Uart2TxData(tCommand.parameter);
#endif

    switch(tCommand.parameter)
    {
        case CMD_NETWORK_STATE_BUSY:
            //Uart2TxData(gu8WaitStateCnt);
            gu8WaitStateCnt = 0;
            break;
            
		case CMD_NETWORK_OUTDOOR_OPEN1	    :
		case CMD_NETWORK_OUTDOOR_OPEN2	    :
            OpenDoor( DOOR_OPEN1, STATE_DOOR_CLOSE, 0 );
            OpenDoor( DOOR_OPEN2, STATE_DOOR_CLOSE, 0 );
		    break;
	    case NET_MONITOR_WIFI_CAM:
            gucCallWaitTime = 50;
		    OtherDoorCall();
            break;
        case NET_MONITOR_WIFI_CAM_END:
		    if(OUTDOOR_STATUS_STANDBY != gucOutdoorStatus)
		    {
                Standby();
            }
            break;
   
        default:
            break;
    }

    guiNetworkMonitorData = 0;
}






void NetworkMsgParsing( WORD uiData )
{
    COMMAND_FORMAT tCommand;

    tCommand = NetworkMsgDecode(uiData);
    
#if 0
	Send_Data_To_UART0(0x55);
	Send_Data_To_UART0(tCommand.crc);
	Send_Data_To_UART0(tCommand.sender);
	Send_Data_To_UART0(tCommand.receiver);
	Send_Data_To_UART0(tCommand.parameter);
#endif


    if(tCommand.receiver == BROADCAST_ADDR)
    {
        if(guiNetworkMsgTxBackup == uiData)/*发送方*/
        {
            switch(tCommand.parameter)
            {
				case CMD_NETWORK_OUTDOOR_CALL_OUT	:
					gucRingTimeCnt   = 0;
					u8MonitorRingNum = 0;
					u8CallFlag       = 0;
					gucOutdoorStatus = OUTDOOR_STATUS_RINGING;
					break;

                default:
                    break;
            }
            
            guiNetworkMsgTxBackup = 0;
        }
        else   /*接收方*/
        {
            switch(tCommand.parameter)
            {
                case CMD_NETWORK_STATE_ASK:
                    gu8WaitStateCnt = 20;
                    break;
                    
				case CMD_NETWORK_OUTDOOR_CALL_OUT	:
					gucCallWaitTime = 50;
					OtherDoorCall();
					break;
					
				case CMD_NETWORK_OUTDOOR_CAM_STANDBY:
					//Standby();
				    break;
				    
				case CMD_NETWORK_OUTDOOR_MONITOR_CAM1   :
				    if(DOOR1_ADDRESS == gucNetworkLocalAddress)
				    {
						Monitor();
				    }
				    else
				    {
						Standby();
				    }

				    if(gu8SendResetTimeCnt)
				    {
                        gu8ResetTimeCnt = 6;
				    }
				    break;
				    
				case CMD_NETWORK_OUTDOOR_MONITOR_CAM2	:
				    if(DOOR2_ADDRESS == gucNetworkLocalAddress)
				    {
						Monitor();
				    }
				    else
				    {
						Standby();
				    }
					break;
					
				case CMD_NETWORK_REPORT_OWN_OUTDOOR_CALL  :
				    if(OUTDOOR_STATUS_RINGING == gucOutdoorStatus)
				    {
				        u8MonitorRingNum |= AdressFlag[0][tCommand.sender - MASTER_ADDRESS];
						Ringing();
				    }
				    else
				    {
						Standby();
				    }
				    
					break;
				case CMD_NETWORK_OUTDOOR_CLOSE2 :
				    if(gucOutdoorStatus >= OUTDOOR_STATUS_RINGING )
				    {
                        OpenDoor( DOOR_OPEN2, STATE_DOOR_CLOSE, 0 );
				    }
				    break;
						
                default:
                    break;
            }
        }

        return;
    }


    if(tCommand.sender == gucNetworkLocalAddress)
    {
        if(guiNetworkMsgTxBackup == uiData)
        {
			NetworkTxPostProcess();   //发送方
        }
        else
        {
            guiNetworkMonitorData = uiData;
            NetworkMonitorProcess(); //地址重复
        }
    }
    else
    {
        if(tCommand.receiver == gucNetworkLocalAddress)
        {
			guiNetworkMsgRxBackup = uiData;
			NetworkRxProcess();  //接收方
        }
        else
        {
            guiNetworkMonitorData = uiData;
            NetworkMonitorProcess(); //旁观
        }
    }
}



void NetworkStatusTimeoutProcess( void )
{
    if(gucOutdoorStatusTimeoutCounter > 0)
    {
        gucOutdoorStatusTimeoutCounter--;

        if(gucOutdoorStatusTimeoutCounter == 0)
        {
            gucOutdoorStatus = OUTDOOR_STATUS_STANDBY;
            Standby();
			NetworkTxProcess(BROADCAST_ADDR, CMD_NETWORK_OUTDOOR_CAM_STANDBY);
        }
    }
}






#define __MANCHESTER_C__


#include "cpu.h"
#include "board.h"
#include "manchester.h"
#include "network.h"
#include "Common.h"


#define NET_MSG_LENGTH   (32 - 1)

extern BYTE gucNetworkLocalAddress;



DWORD  guiManchesterTxData   = 0;    /*  发送的数据*/
BYTE  gucManchesterTxFlag   = 0;    /*  发送数据的状态标识或开关*/
BYTE  gucManchesterTxPoint  = 0;    /*  发送数据的位下标*/
BYTE  gucManchesterTxPulse  = 0;    /*  发送数据的半位标识*/


DWORD  guiManchesterRxData   = 0;    /*  收到的数据*/
BYTE  gucManchesterRxPoint  = 0;    /*  收到数据的位下标*/
BYTE  gucManchesterRxState  = 0;    /*  收到数据位的状态标识*/
BYTE  gucManchesterRxTime   = 0;    /*  收到数据位的维持时间*/


DWORD  guiManchesterRxBuffer[MANCHESTER_RX_BUFFER_SIZE] = {0};     /*  数据接收缓冲区*/
WORD  guiManchesterTxBuffer[MANCHESTER_TX_BUFFER_SIZE] = {0};     /*  数据发送缓冲区*/
BYTE  gucManchesterRxIndex   = 0;   /*  数据缓冲区数据下标*/
BYTE  gucManchesterDealIndex = 0;   /*  数据缓冲区处理下标*/


BYTE gucManchesterCriticalFlag = 0;             /*  临界区标志量，在处理的时候，不接收任何数据*/
WORD guiManchesterStandbyTime  = 0;             /*  总线空闲时间*/
WORD guiManchesterTxWaitingCnt = 0;             /*  总线冲突等待*/

BYTE gucManchesterCollisionDetectionFlag = 0;   /*  侦测网络中数据侦的接收情况*/



void ManchesterEncoding( void )  /*1ms轮询一次*/
{	
    if(gucManchesterTxFlag)
    {
    
		PIN_TALK_OFF = 1;
		
		PIN_34118_MUTE = 1;
		PIN_AUDIO_MSG_EN = 0;
		
        if((guiManchesterTxData << gucManchesterTxPoint) & 0x80000000)
        {
            if(gucManchesterTxPulse == 0)
            {
                set_PWMRUN;
                gucManchesterTxPulse = 1;
            }
            else
            {
                clr_PWMRUN;
                gucManchesterTxPulse = 0;
            }
        }
        else
        {
            if(gucManchesterTxPulse == 0)
            {
                clr_PWMRUN;
                gucManchesterTxPulse = 1;
            }
            else
            {
                set_PWMRUN;
                gucManchesterTxPulse = 0;
            }
        }

        if(gucManchesterTxPulse == 0)
        {
            if( gucManchesterTxPoint < NET_MSG_LENGTH )
            {
                gucManchesterTxPoint++;
            }
            else
            {
                gucManchesterTxFlag = 0;
            }
        }
    }
    else
    {
        clr_PWMRUN;
		PIN_TALK_OFF = 0;
		Set34118MuteFlag(Get34118MuteFlag());
    }
    
}


void ManchesterDecodingInput0( void )
{
    guiManchesterStandbyTime = 0;

    if( gucManchesterRxPoint < NET_MSG_LENGTH )
    {
        gucManchesterRxPoint++;

        gucManchesterCollisionDetectionFlag = 1;
    }
    else
    {
        gucManchesterCriticalFlag = 1;

        guiManchesterRxBuffer[gucManchesterRxIndex] = guiManchesterRxData;

        if(gucManchesterRxIndex < (MANCHESTER_RX_BUFFER_SIZE - 1))
        {
            gucManchesterRxIndex++;
        }
        else
        {
            gucManchesterRxIndex = 0;
        }

        guiManchesterRxData  = 0;
        gucManchesterRxPoint = 0;
        gucManchesterRxTime  = 0;

        gucManchesterCriticalFlag = 0;

        gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
    }
}


void ManchesterDecodingInput1( void )
{
    guiManchesterStandbyTime = 0;

    if(gucManchesterRxPoint != 0)
    {
        guiManchesterRxData |= (0x80000000 >> gucManchesterRxPoint);

        if( gucManchesterRxPoint < NET_MSG_LENGTH )
        {
            gucManchesterRxPoint++;

            gucManchesterCollisionDetectionFlag = 1;
        }
        else
        {
            gucManchesterCriticalFlag = 1;

            guiManchesterRxBuffer[gucManchesterRxIndex] = guiManchesterRxData;

            if(gucManchesterRxIndex < (MANCHESTER_RX_BUFFER_SIZE - 1))
            {
                gucManchesterRxIndex++;
            }
            else
            {
                gucManchesterRxIndex = 0;
            }

            guiManchesterRxData  = 0;
            gucManchesterRxPoint = 0;
            gucManchesterRxTime  = 0;

            gucManchesterCriticalFlag = 0;

            gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
        }
    }
    else
    {
        gucManchesterRxPoint = 0;
        guiManchesterRxData  = 0;
    }
}


void ManchesterDecoding( void )
{
    if((MANCHESTER_DATA_RX_PIN == 0) && (gucManchesterCriticalFlag == 0))
    {
        switch(gucManchesterRxState)
        {
            case MANCHESTER_DATA_STATE_F_L:     /*  总线处于空闲且低电平状态，检测到IO  为低，说明无数据输入*/
                gucManchesterCollisionDetectionFlag = 0;
                break;

            case MANCHESTER_DATA_STATE_F_H:     /*  总线处于空闲且高电平状态，检测到IO  为低，说明数据冲突位结束*/
                guiManchesterRxData  = 0;
                gucManchesterRxPoint = 0;
                gucManchesterRxTime  = 0;

                gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
                break;

            case MANCHESTER_DATA_STATE_1_L:
            case MANCHESTER_DATA_STATE_0_L:
                if(gucManchesterRxTime < MANCHESTER_DATA_DUTY)
                {
                    gucManchesterRxTime++;      /*  电平持续计时加一，如果超过12 * 200us  就认为是回到空闲状态*/
                }
                else
                {
                    guiManchesterRxData  = 0;
                    gucManchesterRxPoint = 0;
                    gucManchesterRxTime  = 0;

                    gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
                }
                break;

            case MANCHESTER_DATA_STATE_1_H:     /*  数据输入1    并处于高电平状态，检测到下跳变，如果高电平的时间为1ms，说明是输入1，如果高电平时间为2ms，说明数据冲突*/
                if(gucManchesterRxTime < MANCHESTER_HALT_DATA_DUTY)
                {
                    gucManchesterRxState = MANCHESTER_DATA_STATE_1_L;
                    ManchesterDecodingInput1();
                }
                else
                {
                    guiManchesterRxData  = 0;
                    gucManchesterRxPoint = 0;

                    gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
                }

                gucManchesterRxTime = 0;
                break;

            case MANCHESTER_DATA_STATE_0_H:    /*  数据输入1    并处于高电平状态*/
                if(gucManchesterRxTime < MANCHESTER_HALT_DATA_DUTY)
                {
                    gucManchesterRxState = MANCHESTER_DATA_STATE_0_L;
                }
                else
                {
                    gucManchesterRxState = MANCHESTER_DATA_STATE_1_L;
                    ManchesterDecodingInput1();
                }

                gucManchesterRxTime = 0;
                break;

            default:
                break;
        }
    }

    if((MANCHESTER_DATA_RX_PIN == 1) && (gucManchesterCriticalFlag == 0))
    {
        switch(gucManchesterRxState)
        {
            case MANCHESTER_DATA_STATE_F_L:     /*  总线处于空闲且低电平状态，检测到上跳变，说明是输入0，而且是输入的第一位的数据*/
                gucManchesterRxState = MANCHESTER_DATA_STATE_0_H;
                ManchesterDecodingInput0();
                gucManchesterRxTime  = 0;
                break;

            case MANCHESTER_DATA_STATE_F_H:     /*  总线处于空闲且高电平状态，检测到IO  为高，说明数据冲突位结束中*/
                gucManchesterCollisionDetectionFlag = 0;
                break;

            case MANCHESTER_DATA_STATE_1_H:
            case MANCHESTER_DATA_STATE_0_H:     /*  电平持续加一，如果超过12 * 200us，说明数据冲突*/
                if(gucManchesterRxTime < MANCHESTER_DATA_DUTY)
                {
                    gucManchesterRxTime++;
                }
                else
                {
                    guiManchesterRxData  = 0;
                    gucManchesterRxPoint = 0;
                    gucManchesterRxTime  = 0;

                    gucManchesterRxState = MANCHESTER_DATA_STATE_F_H;
                }
                break;

            case MANCHESTER_DATA_STATE_1_L:     /*  数据输入1    并处于低电平状态，检测到上跳变，如果低电平时间为1ms，说明是空上跳，如果低电平时间为2ms，说明输入为0*/
                if(gucManchesterRxTime < MANCHESTER_HALT_DATA_DUTY)
                {
                    gucManchesterRxState = MANCHESTER_DATA_STATE_1_H;
                }
                else
                {
                    gucManchesterRxState = MANCHESTER_DATA_STATE_0_H;
                    ManchesterDecodingInput0();
                }

                gucManchesterRxTime = 0;
                break;

            case MANCHESTER_DATA_STATE_0_L:     /*  数据输入1    并处于低电平状态，检测到上跳变，如果低电平时间为1ms，说明输入为0，如果低电平时间为2ms，说明发生错误*/
                if(gucManchesterRxTime < MANCHESTER_HALT_DATA_DUTY)
                {
                    gucManchesterRxState = MANCHESTER_DATA_STATE_0_H;
                    ManchesterDecodingInput0();
                }
                else
                {
                    guiManchesterRxData  = 0;
                    gucManchesterRxPoint = 0;

                    gucManchesterRxState = MANCHESTER_DATA_STATE_F_H;
                }

                gucManchesterRxTime = 0;
                break;

            default:
                break;
        }
    }
}


void ManchesterTxAdd( WORD uiData )
{
    BYTE i = 0;

    for(i = 0; i < MANCHESTER_TX_BUFFER_SIZE; i++)
    {
        if(guiManchesterTxBuffer[i] == 0)
        {
            guiManchesterTxBuffer[i] = uiData;

            break;
        }
    }
}


BYTE ManchesterTxData( WORD uiData )
{
    if(
       (guiManchesterStandbyTime < 15) 
    || (gucManchesterTxFlag == TRUE) 
    || (gu8WaitStateCnt > 0)
    )
    {
        return FALSE;
    }

    SetNetworkMsgTxBackup(uiData);
	
    gucManchesterTxPoint = 0;
    gucManchesterTxPulse = 0;
    guiManchesterTxData  = uiData;
    guiManchesterTxData  |= (guiManchesterTxData << 16);
    gucManchesterTxFlag  = TRUE;

    return TRUE;
}


void ManchesterTxProcess( void )
{
    BYTE i = 0;
    
	if(GetStartTime()) return;

    if(guiManchesterTxWaitingCnt > 0) return;
    
    if(ManchesterCollisionDetectionFlag() == BUSY)
    {
		guiManchesterTxWaitingCnt = 500;
		
        return;
    }
    
    if(guiManchesterTxBuffer[0] != 0)
    {
        if(ManchesterTxData(guiManchesterTxBuffer[0]) == TRUE)
        {
            for(i = 1; i < MANCHESTER_TX_BUFFER_SIZE; i++)
            {
                guiManchesterTxBuffer[i - 1] = guiManchesterTxBuffer[i];
                guiManchesterTxBuffer[i]     = 0;
            }
        }
    }
}

//#define NET_MSG_PRINTF 

extern U8 CalCrcProcess(U16 u16Commd);

void ManchesterRxProcess( void )
{
    WORD uiData = 0;
    U8 u8Crc = 0;
    
    if(gucManchesterDealIndex != gucManchesterRxIndex)
    {
		uiData = (U16)((guiManchesterRxBuffer[gucManchesterDealIndex] >> 16) & 0x0000FFFF);
		u8Crc = CalCrcProcess(uiData);
		
        if( u8Crc == ((uiData >> 13) & 0x03) )
        {
#ifdef NET_MSG_PRINTF
            Send_Data_To_UART0(0x03);
            Send_Data_To_UART0( (U8)(uiData >> 8) );
            Send_Data_To_UART0( (U8)uiData );
#endif
            NetworkMsgParsing(uiData);
        }
        else
        {
            uiData = (U16)(guiManchesterRxBuffer[gucManchesterDealIndex] & 0x0000FFFF);
            u8Crc = CalCrcProcess(uiData);
            
            if( u8Crc == ((uiData >> 13) & 0x03) )
            {
#ifdef NET_MSG_PRINTF
                Send_Data_To_UART0(0x04);
                Send_Data_To_UART0( (U8)(uiData >> 8) );
                Send_Data_To_UART0( (U8)uiData );
#endif
				NetworkMsgParsing(uiData);
            }
        }
        
        if(gucManchesterDealIndex < (MANCHESTER_RX_BUFFER_SIZE - 1))
        {
           gucManchesterDealIndex++; 
        }
        else
        {
            gucManchesterDealIndex = 0;
        }
    }
}


/*  曼彻斯特冲突检测，1 busy*/
BYTE ManchesterCollisionDetectionFlag( void )
{
    return gucManchesterCollisionDetectionFlag;
}



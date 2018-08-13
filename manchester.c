#define __MANCHESTER_C__


#include "cpu.h"
#include "board.h"
#include "manchester.h"
#include "network.h"
#include "Common.h"


#define NET_MSG_LENGTH   (32 - 1)

extern BYTE gucNetworkLocalAddress;



DWORD  guiManchesterTxData   = 0;    /*  ���͵�����*/
BYTE  gucManchesterTxFlag   = 0;    /*  �������ݵ�״̬��ʶ�򿪹�*/
BYTE  gucManchesterTxPoint  = 0;    /*  �������ݵ�λ�±�*/
BYTE  gucManchesterTxPulse  = 0;    /*  �������ݵİ�λ��ʶ*/


DWORD  guiManchesterRxData   = 0;    /*  �յ�������*/
BYTE  gucManchesterRxPoint  = 0;    /*  �յ����ݵ�λ�±�*/
BYTE  gucManchesterRxState  = 0;    /*  �յ�����λ��״̬��ʶ*/
BYTE  gucManchesterRxTime   = 0;    /*  �յ�����λ��ά��ʱ��*/


DWORD  guiManchesterRxBuffer[MANCHESTER_RX_BUFFER_SIZE] = {0};     /*  ���ݽ��ջ�����*/
WORD  guiManchesterTxBuffer[MANCHESTER_TX_BUFFER_SIZE] = {0};     /*  ���ݷ��ͻ�����*/
BYTE  gucManchesterRxIndex   = 0;   /*  ���ݻ����������±�*/
BYTE  gucManchesterDealIndex = 0;   /*  ���ݻ����������±�*/


BYTE gucManchesterCriticalFlag = 0;             /*  �ٽ�����־�����ڴ����ʱ�򣬲������κ�����*/
WORD guiManchesterStandbyTime  = 0;             /*  ���߿���ʱ��*/
WORD guiManchesterTxWaitingCnt = 0;             /*  ���߳�ͻ�ȴ�*/

BYTE gucManchesterCollisionDetectionFlag = 0;   /*  ���������������Ľ������*/



void ManchesterEncoding( void )  /*1ms��ѯһ��*/
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
            case MANCHESTER_DATA_STATE_F_L:     /*  ���ߴ��ڿ����ҵ͵�ƽ״̬����⵽IO  Ϊ�ͣ�˵������������*/
                gucManchesterCollisionDetectionFlag = 0;
                break;

            case MANCHESTER_DATA_STATE_F_H:     /*  ���ߴ��ڿ����Ҹߵ�ƽ״̬����⵽IO  Ϊ�ͣ�˵�����ݳ�ͻλ����*/
                guiManchesterRxData  = 0;
                gucManchesterRxPoint = 0;
                gucManchesterRxTime  = 0;

                gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
                break;

            case MANCHESTER_DATA_STATE_1_L:
            case MANCHESTER_DATA_STATE_0_L:
                if(gucManchesterRxTime < MANCHESTER_DATA_DUTY)
                {
                    gucManchesterRxTime++;      /*  ��ƽ������ʱ��һ���������12 * 200us  ����Ϊ�ǻص�����״̬*/
                }
                else
                {
                    guiManchesterRxData  = 0;
                    gucManchesterRxPoint = 0;
                    gucManchesterRxTime  = 0;

                    gucManchesterRxState = MANCHESTER_DATA_STATE_F_L;
                }
                break;

            case MANCHESTER_DATA_STATE_1_H:     /*  ��������1    �����ڸߵ�ƽ״̬����⵽�����䣬����ߵ�ƽ��ʱ��Ϊ1ms��˵��������1������ߵ�ƽʱ��Ϊ2ms��˵�����ݳ�ͻ*/
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

            case MANCHESTER_DATA_STATE_0_H:    /*  ��������1    �����ڸߵ�ƽ״̬*/
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
            case MANCHESTER_DATA_STATE_F_L:     /*  ���ߴ��ڿ����ҵ͵�ƽ״̬����⵽�����䣬˵��������0������������ĵ�һλ������*/
                gucManchesterRxState = MANCHESTER_DATA_STATE_0_H;
                ManchesterDecodingInput0();
                gucManchesterRxTime  = 0;
                break;

            case MANCHESTER_DATA_STATE_F_H:     /*  ���ߴ��ڿ����Ҹߵ�ƽ״̬����⵽IO  Ϊ�ߣ�˵�����ݳ�ͻλ������*/
                gucManchesterCollisionDetectionFlag = 0;
                break;

            case MANCHESTER_DATA_STATE_1_H:
            case MANCHESTER_DATA_STATE_0_H:     /*  ��ƽ������һ���������12 * 200us��˵�����ݳ�ͻ*/
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

            case MANCHESTER_DATA_STATE_1_L:     /*  ��������1    �����ڵ͵�ƽ״̬����⵽�����䣬����͵�ƽʱ��Ϊ1ms��˵���ǿ�����������͵�ƽʱ��Ϊ2ms��˵������Ϊ0*/
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

            case MANCHESTER_DATA_STATE_0_L:     /*  ��������1    �����ڵ͵�ƽ״̬����⵽�����䣬����͵�ƽʱ��Ϊ1ms��˵������Ϊ0������͵�ƽʱ��Ϊ2ms��˵����������*/
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


/*  ����˹�س�ͻ��⣬1 busy*/
BYTE ManchesterCollisionDetectionFlag( void )
{
    return gucManchesterCollisionDetectionFlag;
}



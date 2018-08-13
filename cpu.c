#define __CPU_C__


#include "cpu.h"
#include "manchester.h"
#include "Common.h"


extern WORD guiManchesterTxWaitingCnt;

WORD guiTick200us = 0;
WORD gucDlyCnt    = 0;
BYTE gucFlag20ms  = 0;
BYTE gucFlag1s    = 0;
WORD guiStartTime = 0;
U8 guc1msflg = 0;
#if 1 
void Timer3_ISR (void) interrupt 16
{
    clr_TF3;/*清除标志位*/
    
    guiTick200us++;
	
    ManchesterDecoding();

    if(gucDlyCnt > 0)
    {
        gucDlyCnt--;
    }

    if(guiTick200us >= 5)  /*  200us * 5 = 1ms  */
    {
        guiTick200us = 0;
        ManchesterEncoding();

        if(guiManchesterStandbyTime < 100)
        {
            guiManchesterStandbyTime++;
        }
        
        guc1msflg++;
    }

    if(guiStartTime  > 0) guiStartTime--;

    if(guiManchesterTxWaitingCnt > 0) guiManchesterTxWaitingCnt--;

}
#endif


U8 gucUART1RXNU   = 0;
U8 gucUART1ReadNU = 0;
U8 UART1RXBuffer[ UART1RXBufferNU ];

/*
串口中断消息接收处理
*/
#if 1
void SerialPort0_ISR(void) interrupt 4 
{
	if (RI)
	{
		RI = 0;
		UART1RXBuffer[gucUART1RXNU] = SBUF;
		gucUART1RXNU++;
		gucUART1RXNU %= UART1RXBufferNU;
	}
	if (TI)
	{
		TI = 0;
	}
	
}
#endif


void Uart2TxData( BYTE ucData )
{
    TI = 0;
    SBUF = ucData;
    while(TI==0);
}


WORD GetStartTime( void )
{
    return guiStartTime;
}


void SetStartTime( WORD uiTime200us )
{
    guiStartTime = uiTime200us;
}

void Stm8sTimerDelay( WORD uiCnt200us )
{
    gucDlyCnt = uiCnt200us;

    while(gucDlyCnt > 0);
}

void Init_ADC(void)
{
	Enable_ADC_AIN4;
}

U16 ReadAD4(void)
{
    U16 VOLADCValue = 0;
	U8 ADCValueH = 0,ADCValueL = 0;
	
    clr_ADCF;           //清空标志位转换完成
    set_ADCS;           //开始转换
    while(ADCF == 0);   //在转换中置1转换完成
    ADCValueL = ADCRL;
    ADCValueH = ADCRH;
    
    //Send_Data_To_UART0(0x55);
    //Send_Data_To_UART0(ADCValueH);
    //Send_Data_To_UART0(ADCValueL);
    
    VOLADCValue = ADCRL;
    VOLADCValue |= ADCRH<<4;

    return  ADCValueH;
}


void GPIOConfigure( void )
{
	Set_All_GPIO_Quasi_Mode;
	
	P06_PushPull_Mode;
	P07_PushPull_Mode;
	P30_PushPull_Mode;
	P17_PushPull_Mode;
	P16_PushPull_Mode;
	P14_PushPull_Mode;
	P13_PushPull_Mode;
	P11_PushPull_Mode;
	P00_PushPull_Mode;
	P01_PushPull_Mode;
	P02_PushPull_Mode;
	P04_PushPull_Mode;

    P10_Input_Mode;
    P03_Input_Mode;
}

void InitCPU( void )
{
    GPIOConfigure();
    
    Init_timer3();
    InitPwm0();
    Init_ADC();
}




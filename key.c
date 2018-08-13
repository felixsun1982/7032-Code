#define __KEY_C__


#include "key.h"
#include "board.h"


BYTE KeyScanning( void )
{
	BYTE ucKeyValue = 0;
	static BYTE gucKeyCount = 0;
	static BYTE gucKeyState = KEY_STATE_IDLE;

	if(gucCallWaitTime > 0)
	{
        return 0xFF;
	}

	switch(gucKeyState)
	{
		case KEY_STATE_IDLE:
			if(0 == PIN_KEY_CALL)
			{
				gucKeyState = KEY_STATE_DOWN;
				gucKeyCount = 0;
			}
			break;

		case KEY_STATE_DOWN:
			if(0 == PIN_KEY_CALL)
			{
				gucKeyCount++;

				if(gucKeyCount == 3)
				{
					gucKeyState = KEY_STATE_WAIT_UP;
					gucKeyCount = 0;
				}
			}
			else
			{
				gucKeyState = KEY_STATE_DOWN;
				gucKeyCount = 0;
			}
			break;

		case KEY_STATE_WAIT_UP:
			if(1 == PIN_KEY_CALL)
			{
				gucKeyState = KEY_STATE_IDLE;
				gucKeyCount = 0;

				ucKeyValue  = 0x55;     /*  按键抬起返回值*/
			}
            else
            {
                ucKeyValue  = 0xAA;     /*  按键按着返回值*/
            }
			break;

		default:
			break;
	}

	return ucKeyValue;
}

/*
BYTE Key2Scanning( void )
{
	BYTE ucKeyValue = 0;
	static BYTE gucKeyCount = 0;
	static BYTE gucKeyState = KEY_STATE_IDLE;
	
	if(gucCallWaitTime > 0)
	{
        return 0xFF;
	}

	switch(gucKeyState)
	{
		case KEY_STATE_IDLE:
			if(0 == PIN_KEY_CALL_2)
			{
				gucKeyState = KEY_STATE_DOWN;
				gucKeyCount = 0;
			}
			break;

		case KEY_STATE_DOWN:
			if(0 == PIN_KEY_CALL_2)
			{
				gucKeyCount++;

				if(gucKeyCount == 3)
				{
					gucKeyState = KEY_STATE_WAIT_UP;
					gucKeyCount = 0;
				}
			}
			else
			{
				gucKeyState = KEY_STATE_DOWN;
				gucKeyCount = 0;
			}
			break;

		case KEY_STATE_WAIT_UP:
			if(1 == PIN_KEY_CALL_2)
			{
				gucKeyState = KEY_STATE_IDLE;
				gucKeyCount = 0;

				ucKeyValue  = 0x55;     //  按键抬起返回值
			}
            else
            {
                ucKeyValue  = 0xAA;     //  按键按着返回值
            }
			break;

		default:
			break;
	}

	return ucKeyValue;

}*/


#include "BluetoothControl.h"



BTControl::~BTControl()
{

}

BTControl::BTControl()
{

}

uint8_t BTControl::BT_Ctrl_OpenOrClose(uint8_t isOpen)
{
	if(isOpen == 0)
	{
	
	}
	else
	{
	
	}

	return 0;
}

uint8_t BTControl::BT_Ctrl_Mode(uint8_t mode)
{
	if(mode == 0)
	{
	
	}
	else
	{
	
	}

	return 0;
}

uint8_t BTControl::BT_STA_SetSSID(char *ssid, uint32_t len)
{
	if(ssid == NULL && len == 0)
	{
		BTLOG("No ssid!\n");
		return -1;
	}
	else
	{
	
	}

	return 0;
}



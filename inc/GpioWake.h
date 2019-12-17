#ifndef _GPIO_WAKE_H_
#define _GPIO_WAKE_H_
#include <stdint.h>
#include "LedControl.h"
#include "TBoxDataPool.h"
#include "WiFiControl.h"
#include "common.h"

#define MCU_UART_DEVICE	        "/dev/ttyHS1"    /* for mcu uart */
#define UART_SLEEP_CTL			"/sys/class/tty/ttyHS1/device/power/control"

//#define bool int
//#define false 0
//#define true 1


/*****************************************************************************
* Function Name : gipo_wakeup_init
* Description   : gpio唤醒脚的配置
* Input         : None
* Output        : None
* Return        : int 
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
extern int gipo_wakeup_init();


/*****************************************************************************
* Function Name : modem_ri_notify_mcu
* Description   : 用于4g唤醒mcu
* Input         : None
* Output        : None
* Return        : int 
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
extern int modem_ri_notify_mcu(void);

/*****************************************************************************
* Function Name : lowPowerMode
* Description   : go to low power mode,
                  isSleep ->0,  go to sleep
                  isSleep ->1,  wake up system
                  isCloseWifi ->0, close
                  isCloseWifi ->1, open
	              isCloseWifi ->-1, Indicate the other source,
	                                so don't close wifi
* Input         : None
* Output        : None
* Return        : 0: success 
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
extern int lowPowerMode(int isSleep, int isCloseWifi, int isInit);


#endif


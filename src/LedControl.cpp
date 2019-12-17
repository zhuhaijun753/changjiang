#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>  //define O_WRONLY and O_RDONLY
#include "LedControl.h"


#if 0

#define SYSFS_GPIO_EXPORT         "/sys/class/gpio/export"
#define SYSFS_GPIO_4G_LED         "40"
#define SYSFS_GPIO_WIFI_LED       "41"
#define SYSFS_GPIO_OUTPUT_VAL     "OUT"
#define SYSFS_GPIO_4G_DIR         "/sys/class/gpio/gpio40/direction"
#define SYSFS_GPIO_WIFI_DIR       "/sys/class/gpio/gpio41/direction"
#define SYSFS_GPIO_4G_VAL         "/sys/class/gpio/gpio40/value"
#define SYSFS_GPIO_WIFI_VAL       "/sys/class/gpio/gpio41/value"
#define SYSFS_GPIO_VAL_H          "1"
#define SYSFS_GPIO_VAL_L          "0"

#define LTE_LED_NUM 	(0)
#define WIFI_LED_NUM	(1)


static int lteFd, wifiFd;
static bool isSleepOn = false;



/*****************************************************************************
* Function Name : tbox_gpio_config
* Description   : GPIO配置
* Input			: None
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date         : 2018.01.18
*****************************************************************************/
int tbox_gpio_config()
{
	int fd;
    fd = open(SYSFS_GPIO_EXPORT, O_WRONLY);
    if (fd == -1)
    {
        printf("ERR: export pin open error.\n");
        return -1;
    }
    write(fd, SYSFS_GPIO_4G_LED, sizeof(SYSFS_GPIO_4G_LED));
    write(fd, SYSFS_GPIO_WIFI_LED, sizeof(SYSFS_GPIO_WIFI_LED));
    close(fd);

	fd = open(SYSFS_GPIO_4G_DIR, O_WRONLY);
    if (fd == -1)
    {
        printf("ERR: Config 4g pin direction open error.\n");
        return -1;
    }
    write(fd, SYSFS_GPIO_OUTPUT_VAL, sizeof(SYSFS_GPIO_OUTPUT_VAL));
    close(fd);

	fd = open(SYSFS_GPIO_WIFI_DIR, O_WRONLY);
    if (fd == -1)
    {
        printf("ERR: Config wifi pin direction open error.\n");
        return -1;
    }
    write(fd, SYSFS_GPIO_OUTPUT_VAL, sizeof(SYSFS_GPIO_OUTPUT_VAL));
    close(fd);

	return 0;
}

/*****************************************************************************
* Function Name : tbox_gpio_init
* Description   : GPIO初始化函数
* Input			: None
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date         : 2018.01.18
*****************************************************************************/
int tbox_gpio_init()
{
	tbox_gpio_config();

    lteFd = open(SYSFS_GPIO_4G_VAL, O_RDWR);
    if (lteFd == -1)
    {
        printf("ERR: 4g value open error.\n");
        return -1;
    }

	wifiFd = open(SYSFS_GPIO_WIFI_VAL, O_RDWR);
    if (wifiFd == -1)
    {
        printf("ERR: wifi pin value open error.\n");
        return -1;
    }
	
	return 0;
}

/*****************************************************************************
* Function Name : led_on_off
* Description   : led开关
* Input			: int ledNum; int onAndOff
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date         : 2018.01.18
*****************************************************************************/
int led_on_off(int ledNum, int onAndOff)
{
	int ret;
    if (ledNum == 0)
    {
        if (onAndOff == 0)
        {
			ret = write(lteFd, SYSFS_GPIO_VAL_L, sizeof(SYSFS_GPIO_VAL_L));
			/*if (ret > 0)
				printf("11 success!\n");
			else
				printf("11 falied!\n");*/
        }
        else
        {
			ret = write(lteFd, SYSFS_GPIO_VAL_H, sizeof(SYSFS_GPIO_VAL_H));
			/*if (ret > 0)
				printf("success!\n");
			else
				printf("falied!\n");*/
        }
    }
    else if (ledNum == 1)
    {
        if (onAndOff == 0)
        {
			write(wifiFd, SYSFS_GPIO_VAL_L, sizeof(SYSFS_GPIO_VAL_L));
        }
        else
        {
			write(wifiFd, SYSFS_GPIO_VAL_H, sizeof(SYSFS_GPIO_VAL_H));
        }
    }

    return 0;
}

/*****************************************************************************
* Function Name : led_control_timer1ms
* Description   : led 1 Millisecond controk function
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
#if 0

int led_control_timer1ms()
{
	static uint32_t u32_ticks = 0;
	
	u32_ticks++;

	if(isSleepOn == false)
	{
		//网络故障及wifi故障
		if((tboxInfo.networkStatus.isLteNetworkAvailable == 0) && (tboxInfo.networkStatus.wifiConnectionType == 0)) 
		{
			//printf("222222222\n");
			led_on_off(LTE_LED_NUM, 1);
			led_on_off(WIFI_LED_NUM, 0);
		}
		 //网络 & wifi可用
		else if((tboxInfo.networkStatus.isLteNetworkAvailable == 1) && (tboxInfo.networkStatus.wifiConnectionType != 0))
		{
			//printf("333333\n");
			if(u32_ticks == 200)
			{
				led_on_off(LTE_LED_NUM, 1);
				led_on_off(WIFI_LED_NUM, 0);
			}
			else if(u32_ticks == 1000)
			{
				led_on_off(LTE_LED_NUM, 0);
				led_on_off(WIFI_LED_NUM, 1);
			}
			else if(u32_ticks == 1200)
			{
				led_on_off(LTE_LED_NUM, 1);
				led_on_off(WIFI_LED_NUM, 0);
			}
			else if(u32_ticks == 2000)
			{
				u32_ticks = 0;
				led_on_off(LTE_LED_NUM, 0);
				led_on_off(WIFI_LED_NUM, 1);
			}	
		}
		//网络不可用 & wifi可用
		else if((tboxInfo.networkStatus.isLteNetworkAvailable == 0) && (tboxInfo.networkStatus.wifiConnectionType != 0))
		{
			//printf("444444\n");
			led_on_off(LTE_LED_NUM, 0);

			if(u32_ticks == 200)
			{
				led_on_off(WIFI_LED_NUM, 0);
			}
			else if(u32_ticks >= 1000)
			{
				u32_ticks = 0;
				led_on_off(WIFI_LED_NUM, 1);
			}	
		}
		//网络可用 & wifi不可用
		else if((tboxInfo.networkStatus.isLteNetworkAvailable == 1) && (tboxInfo.networkStatus.wifiConnectionType == 0))
		{
			//printf("5555555   u32_ticks:%d\n",u32_ticks);
			led_on_off(WIFI_LED_NUM, 0);
			
			if(u32_ticks == 200)
			{
				led_on_off(LTE_LED_NUM, 0);
			}
			else if(u32_ticks >= 1000)
			{
				u32_ticks = 0;
				led_on_off(LTE_LED_NUM, 1);
			}
		}
	}
	else
	{
		//printf("333333333333333   u32_ticks:%d\n",u32_ticks);
		led_on_off(LTE_LED_NUM, 0);
		led_on_off(WIFI_LED_NUM, 0);
	}

    return 0;
}

#endif 


int led_control_timer1ms()
{
	static uint32_t u32_ticks = 0;
	
	u32_ticks++;

	if(u32_ticks == 1000)
	{
		led_on_off(LTE_LED_NUM, 0);
	}
	else if(u32_ticks >= 2000)
	{
		u32_ticks = 0;
		led_on_off(LTE_LED_NUM, 1);
	}

	return 0;
}

#endif




uint8_t lteLedStatus  = 0;//1:灯亮0:灯灭2:闪烁
uint8_t wifiLedStatus = 0;//1:灯亮0:灯灭2:闪烁

void file_w_s(const char *path,char *str)
{
	int fd;
	int vlen = 0;
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		printf("%s,error %d\n",path,fd);
		return;
	}
	write(fd, str, strlen(str));
	close(fd);
}


void led_blink(const char *led,int light_t)
{
	int fd;
	char ledname[50] ={0};
	char ledlight_t[10] ={0};
	sprintf(ledname, "/sys/class/leds/%s", led);
	sprintf(ledlight_t, "%d", light_t);
	file_w_s(ledname,ledlight_t);
}

void lte_led_blink(int light, int lightOff)
{
    file_w_s("/sys/class/leds/led4g/trigger","timer");
    led_blink("led4g/delay_on",light);
	led_blink("led4g/delay_off",lightOff);

	lteLedStatus = 2;
}

void lte_led_on()
{
    file_w_s("/sys/class/leds/led4g/trigger","none");
    file_w_s("/sys/class/leds/led4g/brightness","1");

	lteLedStatus = 1;
}

void lte_led_off(uint8_t isSetStatus)
{
    file_w_s("/sys/class/leds/led4g/trigger","none");
    file_w_s("/sys/class/leds/led4g/brightness","0");
	
	if(isSetStatus == 1)
	{
		lteLedStatus = 0;
	}
}


void wifi_led_blink(int light, int lightOff)
{
    file_w_s("/sys/class/leds/ledwifi/trigger","timer");
    led_blink("ledwifi/delay_on",light);
	led_blink("ledwifi/delay_off",lightOff);

	wifiLedStatus = 2;
}

void wifi_led_on()
{
    file_w_s("/sys/class/leds/ledwifi/trigger","none");
    file_w_s("/sys/class/leds/ledwifi/brightness","1");
	
	wifiLedStatus = 1;
}

void wifi_led_off(uint8_t isSetStatus)
{	
    file_w_s("/sys/class/leds/ledwifi/trigger","none");
    file_w_s("/sys/class/leds/ledwifi/brightness","0");

	if(isSetStatus == 1)
	{
		wifiLedStatus = 0;
	}
}




static uint32_t u32Count = 0;


int led_init()
{
	u32Count++;

	if(u32Count == 1000)
	{
		wifi_led_on();
	}
	else if(u32Count >= 2000)
	{
		u32Count = 0;
		wifi_led_off(0);
	}

	return 0;
}


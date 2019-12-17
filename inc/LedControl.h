#ifndef _LED_CONTROL_H_
#define _LED_CONTROL_H_


/*
extern int tbox_gpio_init();
extern int led_on_off(int ledNum, int onAndOff);
extern int led_control_timer1ms();
extern bool isSleepOn;
*/

extern void lte_led_blink(int light, int lightOff);
extern void lte_led_on();
extern void lte_led_off(uint8_t isSetStatus); //isSetStatus: 1 表示需要设置

extern void wifi_led_blink(int light, int lightOff);
extern void wifi_led_on();
extern void wifi_led_off(uint8_t isSetStatus); //isSetStatus: 1 表示需要设置

extern uint8_t lteLedStatus;
extern uint8_t wifiLedStatus;

extern int led_init();


#endif // _LED_CONTROL_H_


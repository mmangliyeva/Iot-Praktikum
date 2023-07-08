#include "epaper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"


#define COLORED     0
#define UNCOLORED   1
Epd epd;
unsigned char* frame;
Paint *p;

extern "C" void epaperInit(){

    frame = (unsigned char*)malloc(epd.width * epd.height / 8);
    p=new Paint(frame, epd.width, epd.height);
	p->Clear(UNCOLORED);
	p->SetRotate(ROTATE_270);

	ESP_LOGI("EPD", "e-Paper init and clear");
	epd.LDirInit();
	ESP_LOGI("EPD", "before clear");
	epd.Clear();
	ESP_LOGI("EPD", "Initialized");

	//p->Clear(UNCOLORED);
    //sprintf(str,"Welcome");
    //    paint.DrawStringAt(20, yy, "Welcome", &Font20, COLORED);
    // p->DrawStringAt(20, 0, "Welcome", &Font20, COLORED);
    // epd.Display(frame);
	// vTaskDelay(pdMS_TO_TICKS(2000));

}

extern "C" void epaperShow(int x, int y, char* text,int fontsize){

    sFONT *font;

    if (fontsize==0) font=&Font8;
    if (fontsize==1) font=&Font12;
    if (fontsize==2) font=&Font16;
    if (fontsize==3) font=&Font20;
    if (fontsize==4) font=&Font24;
    

    p->DrawStringAt(x, y, text, font, COLORED);
 
}

extern "C" void epaperClear(){
	p->Clear(UNCOLORED);
}

extern "C" void epaperUpdate(){
    epd.Display(frame);
}

extern "C" void epaperSleep(){
    epd.Sleep();
}

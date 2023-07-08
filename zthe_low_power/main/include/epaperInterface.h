#ifndef EPAPERINTERFACE_H
#define EPAPERINTERFACE_H

//extern "C" void epaperInit();
void epaperInit();
void epaperShow(int x, int y, char* text, int fontsize);
void epaperClear();
void epaperUpdate();
void epaperSleep();


#endif
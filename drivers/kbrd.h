#ifndef KBRD_H
#define KBRD_H

//#include "../kernel/interrupt.h"
void keybuf_push(char val);
char keybuf_pop();

//void keyboard_handler(struct interrupt_info *r);
void keyboard_init();
#endif
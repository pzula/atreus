#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define ROW_COUNT 4
#define COL_COUNT 11

#define FN_PRESSED (~PINB & 128)

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

void reset(void);



// Outputs
// |------------+----+----+----+----|
// | row number |  0 |  1 |  2 |  3 |
// |------------+----+----+----+----|
// | pin number | D0 | D1 | D2 | D3 |
// |------------+----+----+----+----|

// Inputs
// |---------------+----+----+----+----+----+----+----+----+----+----+----|
// | column number |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 |
// |---------------+----+----+----+----+----+----+----+----+----+----+----|
// | pin number    | B0 | B1 | B2 | B3 | B4 | B5 | B6 | B7 | F4 | F5 | F6 |
// |---------------+----+----+----+----+----+----+----+----+----+----+----|



// numbers under 100: normal keys
// numbers between 100 and 200: shifted keys
// numbers over 200: function invocations

int base_layout[ROW_COUNT][COL_COUNT] = \
  { {20, 26, 8,   21,  23,  0,  28, 24, 12, 18, 19},      \
    {4,  22, 7,   9,   10,  0,  11, 13, 14, 15, 51},      \
    {29, 27, 6,   25,  5,  101, 17, 16, 54, 55, 56},      \
    {41, 43, 108, 102, 42, 104, 44, 0,  52, 47, 40} };

int fn_layout[ROW_COUNT][COL_COUNT] = \
  { {108+30, 108+31, 108+45, 108+46, 108 + 49, 0, 75, 36, 37, 38, 108+37}, \
    {108+32, 108+33, 108+38, 108+39, 53, 0, 78, 33, 34, 35, 108+46},    \
    {108 + 34, 108 + 35, 45, 46, 108+53, 101, 49, 30, 31, 32, 108+47},  \
    {255, 108 + 73, 108, 102, 0, 104, 0, 0, 55, 39, 48} };

int *current_row;

int pressed_count = 0;



void press(int keycode) {
  if(!keycode || pressed_count >= 6) return;
  if(keycode == 255 || keycode == 5) {
    reset(); // TODO: make a table of codes -> functions
  } else if(keycode > 108) {
    keyboard_modifier_keys |= KEY_SHIFT;
    keyboard_keys[pressed_count++] = (keycode - 108);
  } else if(keycode > 100) {
    keyboard_modifier_keys |= (keycode - 100);
  } else {
    keyboard_keys[pressed_count++] = keycode;
  };
};

void activate_row(int row) {
  PORTD = (char)(~(1 << row)) | 32; // leave the LED on
};

void scan_row(int row) {
  int cols = (~(PINB + ((PINF >> 4) << 8))) & 2047;
  for(int col = 0; col < 8; col++) {
    if(cols & 1) {
      press(current_row[col]);
    };
    cols = cols >> 1;
  };
};



void init() {
  CPU_PRESCALE(0);
  DDRD = 255;
  DDRB = 0;
  DDRF = 0;
  PORTB = 255;
  PORTF = 255;
  usb_init();
  while (!usb_configured()) /* wait */ ;
  _delay_ms(1000);
};

void clear_keys() {
  keyboard_modifier_keys = 0;
  pressed_count = 0;
  for(int i = 0; i < 6; i++) {
    keyboard_keys[i] = 0;
  };
};

int main() {
  init();
  while(1) {
    // 4th row is still active from last scan
    current_row = FN_PRESSED ? fn_layout : base_layout;
    for(int i = 0; i < ROW_COUNT; i++) {
      activate_row(i);
      scan_row(i);
      current_row += COL_COUNT;
      _delay_us(50);
    };
    usb_keyboard_send();
    clear_keys();
  };
};

void reset(void) {
  UDCON = 1;
  USBCON = (1<<FRZCLK);
  UCSR1B = 0;
  _delay_ms(5);
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0; TIMSK4 = 0; UCSR1B = 0; TWCR = 0;
  DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0; TWCR = 0;
  PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
  asm volatile("jmp 0x7E00");
};

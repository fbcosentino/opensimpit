/*
   ____                  _____ _           ____  _ __ 
  / __ \____  ___  ____ / ___/(_)___ ___  / __ \(_) /_
 / / / / __ \/ _ \/ __ \\__ \/ / __ `__ \/ /_/ / / __/
/ /_/ / /_/ /  __/ / / /__/ / / / / / / / ____/ / /_  
\____/ .___/\___/_/ /_/____/_/_/ /_/ /_/_/   /_/\__/  
    /_/                                              

    https://github.com/fbcosentino/opensimpit
    Fernando Cosentino



   OpenSimPit is a set of resources for building simpits (simulation cockpits),
   for both flight simulators and space games (realistic or fantasy), using
   Arduino IDE and boards and modules from aliexpress.

   The strategy in OpenSimPit is to have a one-size-fits-all package of
   firmware/software, where the main Arduino is entirely configurable
   via PC software to match your cockpit design.

   This sketch is for a secondary Arduino board working as a radio module.
   This is not the main OpenSimPit board sketch. If that is what you're
   looking for, check the repository at github.com/fbcosentino/opensimpit.
   
   The radio module is designed to work specifically on an Arduino Nano.
   It should work with Arduino Uno as well (because it's the same chip), 
   but no other model was tested. 

   This module can be used with the following types of display:
   (Check repository for pictures)
   
     - 8 digits 7-segment LED displays (often sold as "tube" display),
       based on MAX7219 chips or similar. Uses 2 displays (one for active
       frequency and one for standby)
       
     - 16x2 characters LCD display connected via I2C adapter board. Uses a
       single display for both frequencies.
       
     - 16x2 characters LCD display directly connected (via RS, EN, D4-D7)
       Uses a single display for both frequencies.
       
  The Sketch also expects a rotary encoder with integrated push button. If using
  an encoder without push-button, then a separate button must be connnected.

  One Arduino board with this sketch works as the entire radio stack, even if 
  information about just one radio is shown on display. During operation one
  of the functions below can be selected:
     - COM1
     - NAV1
     - COM2
     - NAV2

   The function can be changed at any time during operation via push-button,
   so a single radio module can control all four devices in the game/simulator.

   This board can be used as module connected to an OpenSimPit main board via I2C, 
   or it can be used as a standalone radio module connected to PC via serial port.
   Check repository for connection and drivers.

   For full immersion, if the radio module is directly connected to a PC, for now 
   four Arduinos can be used, each set as one function.
   The plan is to expand this Sketch to work as a full 4-radio stack, using eight
   7-segment displays or 4 LCDs with I2C adapters (direct connection not available)
   controlled via 2 rotary encoders (shared by a COM/NAV pair each).
   To connect to the OpenSimPit main board, a single Arduino must handle all the
   radio stack, which for now means sharing the display.
        
   To select your display of choice, uncomment the corresponding define below.
   LED 7-segment displays are default.
*/

// Uncomment define below for 2x 8 digits 7-segment display using MAX72xx chips
//#define LCD_MAX72XX

// Uncomment define below for 1x 16x2 LCD directly connected (RS, EN, D4-D7) 
#define LCD16X2_DIRECT

// Uncomment the 2 defines below for 1x 16x2 LCD connected via I2C adapted board
// Second define is for adapter board I2C address
//#define LCD16X2_I2C
//#define LCD16X2_I2C_ADDR 0x27



// Minimum frequency allowed
#define RADIO_MIN_VALUE 10.0
// maximum frequency allowed
#define RADIO_MAX_VALUE 999.999



// ==============================================================
// DO NOT TOUCH BELOW THIS LINE
//
// (Unless you know what you're doing)
// ==============================================================


// If nothing was selected, select LCD_MAX72XX as default
#if (!defined(LCD_MAX72XX) && !defined(LCD16X2_DIRECT) && !defined(LCD16X2_I2C))
#define LCD_MAX72XX
#endif

#define BUTTON_PIN 9
#define IRQ_OUTPUT A3

#define LCD_MAX72XX_LOAD_PIN1 7
#define LCD_MAX72XX_LOAD_PIN2 8

#define LCD16X2_DIRECT_K A2

#define LCD16X2_I2C_SDA A0
#define LCD16X2_I2C_SCL A1

#define ENCODER1_A 2
#define ENCODER1_B 4
#define ENCODER2_A 3 // TODO
#define ENCODER2_B 5 // TODO

#define AMOUNT_COARSE   1.0
#define AMOUNT_FINE_COM 0.025
#define AMOUNT_FINE_NAV 0.05

#include <Wire.h>

#ifdef LCD_MAX72XX
#include "DigitLed72xx.h"
DigitLed72xx ledd_a = DigitLed72xx(LCD_MAX72XX_LOAD_PIN1);
DigitLed72xx ledd_s = DigitLed72xx(LCD_MAX72XX_LOAD_PIN2);
#endif

#ifdef LCD16X2_DIRECT
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 7, d5 = 6, d6 = A0, d7 = A1;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#endif

#ifdef LCD16X2_I2C
#include "SoftWire.h"
#include "LiquidCrystal_I2C.h"
SoftWire lcd_wire(LCD16X2_I2C_SDA, LCD16X2_I2C_SCL) 
LiquidCrystal_I2C lcd(LCD16X2_I2C_ADDR, 16, 2);
#endif

#define MODE_COARSE 0
#define MODE_FINE   1
#define NUM_MODES   2
int number_mode = MODE_COARSE;

#define MODE_NUMBER   0
#define MODE_FUNCTION 1
int function_mode = MODE_NUMBER;

#define FUNCTION_COM1 0
#define FUNCTION_COM2 1
#define FUNCTION_NAV1 2
#define FUNCTION_NAV2 3
#define NUM_FUNCTIONS 4
uint16_t current_function = FUNCTION_COM1;

#define IRQ_ACTIVE 1
#define IRQ_IDLE 0

typedef struct radio_values_t {
  float active;
  float standby;
  //float amount_fine;
} radio_values_t;
radio_values_t radio_values[4]; // index is current_function


//float number_active = 123.00;
//float number_standby = 124.25;
bool prev_btn = 1;
uint32_t btn_press_timer = 0; // Used to identify short press/long press click
uint32_t btn_click_timer = 0; // Used to identify single/double (short) click
uint8_t number_of_clicks = 0;


// ------------------------------------------------


void setup() {
  set_IRQ(IRQ_IDLE); // First thing
  pinMode(LCD_MAX72XX_LOAD_PIN1, INPUT_PULLUP);
  pinMode(LCD_MAX72XX_LOAD_PIN2, INPUT_PULLUP);

  for (int i = 0; i < NUM_FUNCTIONS; i++) {
    radio_values[i].active = 100.0 + i;
    radio_values[i].standby = 100.5 + i;
  }
  
  Serial.begin(115200);
  Serial.println("{\"msg\":\"INIT\",\"board\":\"Radio\"}");

  pinMode(ENCODER1_A, INPUT_PULLUP);
  pinMode(ENCODER1_B, INPUT_PULLUP);
  pinMode(ENCODER2_A, INPUT_PULLUP);
  pinMode(ENCODER2_B, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER1_A), encoder1_interrupt, FALLING);
  //attachInterrupt(digitalPinToInterrupt(ENCODER2_A), encoder2_interrupt, FALLING); // TODO

  // I2C slave (to be controlled from the OpenSimpit main board) 
  uint8_t i2c_addr = 0x15;
  
  Wire.begin(i2c_addr);
  Wire.onReceive(i2c_receive);
  Wire.onRequest(i2c_request);

  // Display setup:
  
  #ifdef LCD_MAX72XX
    ledd_a.begin();
    ledd_s.begin();
    ledd_a.on();
    ledd_s.on();
  #endif

  #ifdef LCD16X2_I2C
    lcd.begin(&lcd_wire);
    lcd.blink();
    lcd_clear();
    lcd_set_light(1);
  #endif
  
  #ifdef LCD16X2_DIRECT
    lcd.begin(16,2);
    lcd.blink();
    lcd_clear();
    lcd_set_light(1);
  #endif 

  print_number_screen();
  
  update_display();
 
}






// ------------------------------------------------
// RADIO APPLICATION


void update_display() {
  lcd_write_number(0, radio_values[current_function].active);
  lcd_write_number(1, radio_values[current_function].standby);
}


void change_mode() {
  // Method below is forward compatible to more modes
  number_mode++;
  if (number_mode >= NUM_MODES) number_mode = 0;
}


void swap_numbers() {
  //Serial.print("Swapping ");
  //Serial.println(current_function);
  float tmp = radio_values[current_function].active;
  radio_values[current_function].active = radio_values[current_function].standby;
  radio_values[current_function].standby = tmp;
  update_display();
  set_IRQ(IRQ_ACTIVE);
  print_frequencies();
}



void double_click() {
    //Serial.println("double click");
    function_mode = MODE_FUNCTION;
    print_function_screen();
}

void long_click() {
  //Serial.println("long click");
  if (function_mode == MODE_FUNCTION) {
  }
  
  else if (function_mode == MODE_NUMBER) {
    swap_numbers();
  }
}

void short_click() {
  //Serial.println("short click");
  if (function_mode == MODE_FUNCTION) {
    // Go back to number
    function_mode = MODE_NUMBER;
    print_number_screen();
    update_display();
    number_of_clicks = 0;
  }
  
  else if (function_mode == MODE_NUMBER) {
    change_mode();
    if (number_of_clicks >= 2) {
      number_of_clicks = 0;
      double_click();
    }
  }
}



void loop() {
  if (Serial.available()) process_serial();
  
  bool btn = digitalRead(BUTTON_PIN);
  if (!btn && prev_btn) {
    // Debounce
    delay(10);
    btn = digitalRead(BUTTON_PIN);
    // If btn survived debounce (=0) -> real click
  }
  
  if (!btn) {
    if (prev_btn) {
      btn_press_timer = 0;
    }
    else if (btn_press_timer < 60) {
        btn_press_timer++;
        if (btn_press_timer >= 60) {
          // >= 600ms, long click, don't even wait for release
          long_click();
          number_of_clicks = 0;
        }
    }
  }

  else if (!prev_btn) {
    // Just released
    if (btn_press_timer < 60) {
      // < 600ms, short click
      number_of_clicks++;
      btn_click_timer = 0;
      short_click();
    }
  }
  
  if (number_of_clicks > 0) {
    btn_click_timer++;
    if (btn_click_timer > 60) {
      // 600ms of silence, invalidate click counting (it was single click);
      number_of_clicks = 0;
      btn_click_timer = 0;
    }
  }

  
  
  prev_btn = btn;
  delay(10);

}





// ------------------------------------------------
// INPUT HANDLING

void encoder1_interrupt() {
  delay(1);
  bool in1 = digitalRead(ENCODER1_A);
  bool in2 = digitalRead(ENCODER1_B);

  if (in1 == 0) {
    if (in2 == 0) {
      // Forward
      dial_next(0);
    }
    else {
      // Reverse
      dial_prev(0);
    }
  }
}

void dial_next(uint8_t encoder_index) {
  if (function_mode == MODE_FUNCTION) {
      current_function++;
      if (current_function >= NUM_FUNCTIONS) current_function = 0;
      print_function_screen();
  }
  else if (function_mode == MODE_NUMBER) {
      switch (number_mode) {
        case MODE_COARSE: 
          radio_values[current_function].standby += AMOUNT_COARSE;
          break;
          
        case MODE_FINE:
          radio_values[current_function].standby += (current_function >= 2)? AMOUNT_FINE_NAV : AMOUNT_FINE_COM;
          break;
      }
      if (radio_values[current_function].standby > RADIO_MAX_VALUE) radio_values[current_function].standby = RADIO_MAX_VALUE;
      update_display();
      set_IRQ(IRQ_ACTIVE);
      print_frequencies();
  }
}
void dial_prev(uint8_t encoder_index) {
  if (function_mode == MODE_FUNCTION) {
      if (current_function == 0) current_function = NUM_FUNCTIONS-1; else current_function--;
      print_function_screen();
  }
  else if (function_mode == MODE_NUMBER) {
      switch (number_mode) {
        case MODE_COARSE: 
          radio_values[current_function].standby -= AMOUNT_COARSE;
          break;
          
        case MODE_FINE:
          radio_values[current_function].standby -= (current_function >= 2)? AMOUNT_FINE_NAV : AMOUNT_FINE_COM;
          break;
      }
      if (radio_values[current_function].standby < RADIO_MIN_VALUE) radio_values[current_function].standby = RADIO_MIN_VALUE;
      update_display();
      set_IRQ(IRQ_ACTIVE);
      print_frequencies();
  }
}





// ------------------------------------------------
// DISPLAY LOW LEVEL
// This section abstracts a model-independent display API


void lcd_clear() {
  #ifdef LCD_MAX72XX
    ledd_a.clear();
    ledd_s.clear();
  #endif
  
  #if (defined(LCD16X2_I2C) || defined(LCD16X2_DIRECT))
    lcd.clear();
  #endif
}


void lcd_set_light(bool light_on) {
  #ifdef LCD_MAX72XX
    if (light_on) {
      ledd_a.on();
      ledd_s.on();
    }
    else {
      ledd_a.off();
      ledd_s.off();
    }
  #endif

  #ifdef LCD16X2_DIRECT
    digitalWrite(LCD16X2_DIRECT_K, (light_on)?0:1);
  #endif
  
  #ifdef LCD16X2_I2C
    lcd.setBacklight(light_on);
  #endif
}

void lcd_write_number(uint8_t num_index, float number) {
  uint16_t part_int = floor(number);
  uint16_t part_dec = round((number - part_int)*1000);
  
  #ifdef LCD_MAX72XX
    DigitLed72xx *ledd;
    // using a switch to be forward compatible to more displays
    switch(num_index) {
      case 0: ledd = &ledd_a; break;
      case 1: ledd = &ledd_s; break;
    }
    unsigned long full_num = ((unsigned long)part_int)*1000 + part_dec; // Dot will be automatic
    ledd->printDigit(full_num, 0, 0);
    if (full_num < 100000) ledd->setDigit(6, 0x00);
    if (full_num <  10000) ledd->setDigit(5, 0x00);
    if (full_num <   1000) ledd->setDigit(4, 0x0b11111110); // With dot
  #endif
  
  #if (defined(LCD16X2_I2C) || defined(LCD16X2_DIRECT))
    lcd.setCursor(8, num_index);
    char buf[8];
    sprintf(buf, "%4d.%03d", part_int, part_dec);
    lcd.print(buf);
  #endif
    
}


void print_function_name(uint8_t function) {
  #ifdef LCD_MAX72XX
    if ((function == FUNCTION_COM1) || (function == FUNCTION_COM2)) {
      ledd_a.setDigit(8, 0b01000011); // 'c'
      ledd_s.setDigit(8, 0b01000011); // 'c'
    }
    else if ((function == FUNCTION_NAV1) || (function == FUNCTION_NAV2)) {
      ledd_a.setDigit(8, 0b01100010); // 'c'
      ledd_s.setDigit(8, 0b01100010); // 'c'
    }
    if ((function == FUNCTION_COM1) || (function == FUNCTION_NAV1)) {
      ledd_a.setDigit(7, 0b00000010); // '
      ledd_s.setDigit(7, 0b00000010); // '
    }
    else if ((function == FUNCTION_COM2) || (function == FUNCTION_NAV2)) {
      ledd_a.setDigit(7, 0b00100010); // ''
      ledd_s.setDigit(7, 0b00100010); // ''
    }
  #endif
  
  #if (defined(LCD16X2_I2C) || defined(LCD16X2_DIRECT))
    switch (function) {
      case FUNCTION_COM1: lcd.print("COM1"); break;
      case FUNCTION_COM2: lcd.print("COM2"); break;
      case FUNCTION_NAV1: lcd.print("NAV1"); break;
      case FUNCTION_NAV2: lcd.print("NAV2"); break;
    }
  #endif
}


void print_number_screen() {
  #ifdef LCD_MAX72XX
    ledd_a.clear();
    ledd_s.clear();
    print_function_name(current_function);
  #endif
    
  #if (defined(LCD16X2_I2C) || defined(LCD16X2_DIRECT))
    lcd.clear();
    lcd.noCursor();
    lcd.setCursor(0, 0);
    print_function_name(current_function);
    
    lcd.setCursor(5, 0);
    lcd.print("ACT        ");
    lcd.setCursor(5, 1);
    lcd.print("SBY        ");
  #endif
}


void print_function_screen() {
  #ifdef LCD_MAX72XX
    ledd_a.clear();
    ledd_s.clear();
    if ((current_function == FUNCTION_COM1) || (current_function == FUNCTION_COM2)) {
      ledd_a.setDigit(8, 0b01001110); // 'C'
      ledd_a.setDigit(7, 0b01111110); // 'O'
      ledd_a.setDigit(6, 0b00010101); // |"
      ledd_a.setDigit(5, 0b00010101); // |"|
    }
    else if ((current_function == FUNCTION_NAV1) || (current_function == FUNCTION_NAV2)) {
      ledd_a.setDigit(8, 0b01110110); // |"|
      ledd_a.setDigit(7, 0b01110111); // 'A'
      ledd_a.setDigit(6, 0b00111110); // 'U' -> best attempt at V 
    }
    if ((current_function == FUNCTION_COM1) || (current_function == FUNCTION_NAV1)) {
      ledd_a.setDigit(1, 0b00110000); // 1
    }
    else if ((current_function == FUNCTION_COM2) || (current_function == FUNCTION_NAV2)) {
      ledd_a.setDigit(1, 0b01101101); // 2
    }
  #endif
  
  #if (defined(LCD16X2_I2C) || defined(LCD16X2_DIRECT))
    lcd.clear();
    lcd.cursor();
    
    lcd.setCursor(0, 0);
    print_function_name(FUNCTION_COM1);

    lcd.setCursor(8, 0);
    print_function_name(FUNCTION_COM2);

    lcd.setCursor(0, 1);
    print_function_name(FUNCTION_NAV1);

    lcd.setCursor(8, 1);
    print_function_name(FUNCTION_NAV2);

    switch(current_function) {
      case FUNCTION_COM1: lcd.setCursor(0, 0); break;
      case FUNCTION_COM2: lcd.setCursor(8, 0); break;
      case FUNCTION_NAV1: lcd.setCursor(0, 1); break;
      case FUNCTION_NAV2: lcd.setCursor(8, 1); break;
    }
  #endif
}






// ------------------------------------------------
// I2C HANDLING

/**
 * Sets the IRQ output to signal an OpenSimPit master board we have new data.
 */
void set_IRQ(bool active) {
  if (active) {
    // Active low
    digitalWrite(IRQ_OUTPUT, 0);
    pinMode(IRQ_OUTPUT, OUTPUT);
  }
  else {
    // Idle high
    pinMode(IRQ_OUTPUT, INPUT);
    //digitalWrite(IRQ_OUTPUT, 1);
  }
}

void i2c_receive(int num_bytes) {
  // TODO: accept frequency writing via I2C
}

void i2c_request()
{
  set_IRQ(IRQ_IDLE);
  char * radio_data_as_char = (char *)&radio_values;

  for (int i=0; i<32; i++) Wire.write(radio_data_as_char[i]);
}





// ------------------------------------------------
// SERIAL HANDLING



void print_frequencies() {
  // This function is used for PC direct connection
  // OpenSimPit main board connection uses I2C
  
  // Start JSON
  Serial.print("{\"rad\":{");
  
  // Code to output all frequencies per serial message:
  // (Uncomment if desired, but this prevents parallel modules)
  /*
  for (int i=0; i<4; i++) {
    if (i > 0) Serial.print(",");
    Serial.print("\"");
    Serial.print(i);
    Serial.print("\":[");
    Serial.print(radio_values[i].active);
    Serial.print(",");
    Serial.print(radio_values[i].standby);
    Serial.print("]");
  }
  */

  // Outputs only selected function
  Serial.print("\"");
  Serial.print(current_function);
  Serial.print("\":[");
  Serial.print(radio_values[current_function].active);
  Serial.print(",");
  Serial.print(radio_values[current_function].standby);
  Serial.print("]");
  
  // End JSON
  Serial.println("}}");
}


void process_serial() {
  // TODO: accept frequency writing over serial port
}

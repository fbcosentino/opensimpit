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

   This is the main Arduino sketch, designed to be used in a Bluepill/Maple
   board, but is compatible (without changes) to the following boards:

     - Bluepill
     - Arduino Mega
     - Arduino Uno
     - Arduino Nano

   Bluepill provides all the features, while the other boards will lack some
   of them (but still run).

   Some features (such as the Open-Smart battery display) mich make use of
   a separate small Arduino communicating with this one. Check the
   corresponding sketch files in the repository for more information.
   

   === COMPILATION ===
   
   To compile this sketch for the Bluepill, you need STM32F103C8T6 driver:
     File -> Preferences
     On "Additional Boards Manager URLs", insert:
         http://dan.drown.org/stm32duino/package_STM32duino_index.json
     If there are more URLs there already, separate them with a , (comma)

   Then go to:
     Tools -> Board(...) -> Boards Manager...
     Search for "STM32" and install STM32F1xx

   Then go to:
     Tools -> Board(...) -> STM32F1 -> "Generic STM32F103C"
     (or possibly "Generic STM32F103C6/fake STM32F103C8" depending on your board)
     Tools -> Variant -> STM32F103C8
     Tools -> Upload method -> Serial
     Tools -> CPU speed -> 72MHz

   For the other Arduino models, nothing special is needed, just set the correct
   model/variant in the Arduino IDE.


   === USAGE ===

   Check the official repository at https://github.com/fbcosentino/opensimpit

   None of the features are hardcoded in this file, the Arduino sketch itself 
   is not aware of your cockpit design, and will remain this way in all official
   versions. The PC software communicates with this sketch and configures the
   system to match your simpit installation. If you want to make suggestions or
   even contribute to this project, keep this in mind at all times.

*/



#define USE_HID 1
#define JOYAXIS_MIN 0
#define JOYAXIS_MAX 1023
#define SERIAL_BUFFER_SIZE 64

#define NUMBER_OF_BUILTIN_AXES      6
#define NUMBER_OF_JOYSTICK_BUTTONS  32
#define NUMBER_MAX_EXPANDERS        8 // Can't be more than 8 due to I2C addressing
#define NUMBER_MAX_LCD16X2          8 // Can't be more than 8 due to I2C addressing
                                      // Independent on Bluepill, but clashes with expanders on other boards

#define EEPROM_ADDRESS_OFFSET        256 // In case you don't want to use the very firs bytes

// ============================================================================
// CONFIG VARS

#if defined(_BOARD_GENERIC_STM32F103C_H_)
  #define BLUEPILL
  #define EXPANDER_IRQ_PIN PA7
  uint8_t AXIS_PINS[NUMBER_OF_BUILTIN_AXES] = {PA0, PA1, PA2, PA3, PA4, PA5};

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define ARDUINO_MEGA
  #define EXPANDER_IRQ_PIN 4
  uint8_t AXIS_PINS[NUMBER_OF_BUILTIN_AXES] = {A0, A1, A2, A3, A4, A5};

#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  #define ARDUINO_UNO
  #define EXPANDER_IRQ_PIN 4
  uint8_t AXIS_PINS[NUMBER_OF_BUILTIN_AXES] = {A0, A1, A2, A3, A4, A5};

#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
  #define ARDUINO_LEONARDO
  #define EXPANDER_IRQ_PIN 4
  uint8_t AXIS_PINS[NUMBER_OF_BUILTIN_AXES] = {A0, A1, A2, A3, A4, A5};

#endif 

// ============================================================================
// TYPES / VARS

typedef struct AxisConfig_t {
  uint16_t board_pin;
  uint16_t joystick_axis;
  uint16_t min_value;
  uint16_t max_value;
  uint16_t status; // 16 bits so all members have same size and struct has even size (uint16 aligned)
} AxisConfig_t;
// size: 10 bytes per axis (5 * uint16_t) -> THIS SIZE MUST BE EVEN (uint16 aligned)

uint8_t used_axes = 0;


typedef struct ButtonConfig_t {
  uint8_t expander_index;
  uint8_t expander_pin;
  uint8_t joystick_button;
  uint8_t status; // [ xxxx | LAST_STATE | TOGGLE | INVERTED | ENABLED ]
} ButtonConfig_t; 
// size: 4 bytes per button (2 * uint16_t) -> THIS SIZE MUST BE EVEN (uint16 aligned)


typedef struct Lcd16x2Config_t {
  uint8_t lcd_index;
  uint8_t status; // [ xxxxxx | INITIALIZED | ENABLED ]
} Lcd16x2Config_t;
// size: 2 bytes -> THIS SIZE MUST BE EVEN (uint16 aligned)




typedef struct EEPROM_storage_object_t {
  ButtonConfig_t buttons[NUMBER_OF_JOYSTICK_BUTTONS];
  AxisConfig_t axes[NUMBER_OF_BUILTIN_AXES];
  Lcd16x2Config_t lcds16x2[8];
  uint16_t used_buttons;
  uint8_t used_axes;
  uint8_t used_expanders;
} EEPROM_storage_object_t;
#define EEPROM_VARIABLES_SIZE  4 // Number of bytes not including the arrays -> MUST BE EVEN (uint16 aligned)
// Size example: for 32 buttons, 6 axes, 8 lcds16x2 = 28 bytes = 104 * uint16
// Keep in mind maximum storage for Arduino Uno is 1024 bytes!

#define SERIAL_PARAM_EMPTY_END    0
#define SERIAL_PARAM_EMPTY_FOLLOW 1
#define SERIAL_PARAM_INT          2
#define SERIAL_PARAM_OTHER        3
typedef struct serial_param_t {
  uint32_t int_value;
  char * string_value;
  uint16_t next_param_index;
  uint8_t status;
} serial_param_t;

// Unlike LCDs (which are 1:1 to I2C addresses), 16 servos share a same I2C address, which
// means there will be a very limited range of addresses allocated to servos, possibly
// just one. Therefore, servos do not occupy EEPROM in the config, and their I2C addresses
// are expected to be sequential and directly related to their ID in the simpit (that is,
// servo board 0 is address 0x80, servo board 1 is address 0x81, etc). Servo numbers are
// expected to be continuously assigned to boards (e.g. servo 15 is servo 15 in board 0, 
// while servo 16 is servo 0 in board 1).
// PCA9685 ICs are initialized the first time they are used, on the fly.
uint8_t is_servo_expanders_initialized = 0; // Bitpacked, one bit per expander

bool is_joystick_connected = 0;
bool must_read_io = 0;

// ============================================================================
// INCLUDES / INITIALIZATION

// USB
#if defined(BLUEPILL) || defined(ARDUINO_LEONARDO)
  #include <USBComposite.h>
  USBHID HID;
  HIDJoystick Joystick(HID);
#endif

// Port expanders
#include <Wire.h>

// I2C Outputs (LCDs and Servos)
#include "LiquidCrystal_I2C.h"
#if defined(BLUEPILL)
  TwoWire WireLCDs(2, I2C_FAST_MODE);
  #define WireServos WireLCDs
#else
  #define WireLCDs Wire
  #define WireServos Wire
#endif


// Class declaration is static and hardcoded, but they are only 
// initialized (lcd_x.begin(&Wire2)) when used
LiquidCrystal_I2C lcd16x2_0(0x27, 16, 2);
LiquidCrystal_I2C lcd16x2_1(0x26, 16, 2);
LiquidCrystal_I2C lcd16x2_2(0x25, 16, 2);
LiquidCrystal_I2C lcd16x2_3(0x24, 16, 2);
LiquidCrystal_I2C lcd16x2_4(0x23, 16, 2);
LiquidCrystal_I2C lcd16x2_5(0x22, 16, 2);
LiquidCrystal_I2C lcd16x2_6(0x21, 16, 2);
LiquidCrystal_I2C lcd16x2_7(0x20, 16, 2);


// Storage
#include <EEPROM.h>
EEPROM_storage_object_t config_object;
#define EEPROM_STORAGE_SIZE  (EEPROM_VARIABLES_SIZE + 10*NUMBER_OF_BUILTIN_AXES + 4*NUMBER_OF_JOYSTICK_BUTTONS + 2*NUMBER_MAX_LCD16X2) / 2





// ============================================================================
// SETUP


void setup() {

  #if defined(BLUEPILL) || defined(ARDUINO_LEONARDO)
  // Setup Joystick HID
  HID.begin(HID_JOYSTICK);
  uint8_t i = 0;
  while (!USBComposite) {
    delay(10);
    i++;
    if (i > 95) {
      break;
    }
  }
  is_joystick_connected = (USBComposite != 0);
  if (is_joystick_connected) Joystick.setManualReportMode(true);
  #endif
  
  // Setup Serial
  Serial.begin(115200);

  Serial.print("{\"msg\":\"INIT\",\"board\":\"");
  print_board();
  Serial.print("\"");

  #if defined(BLUEPILL)
  EEPROM.PageSize  = 0x400;
  EEPROM.init();
  #endif

  // Initialize config
  config_clear();
  
  // Expanders
  Wire.begin();
  pinMode(EXPANDER_IRQ_PIN, INPUT_PULLUP); 
    
  // Temporary:
  //add_axis(PA0, 0, 1150, 2750); // pin, axis, min, max
  //add_button(0, 2, 0, 0, 1); // exp_i, exp_p, j_btn, inv, tog
  

  // Initialize LCDs
  #if defined(BLUEPILL)
    WireLCDs.begin();
  #endif
  delay(50); // LCDs need at least 40ms after power up to be able to interact

  // Servos share I2C with other devices and therefore WireServos needs not be initialized
  
  set_battery_display(0, 0, 0, 0);

  Serial.print(",\"usb_connected\":");
  Serial.print(is_joystick_connected);
  Serial.println("}");


  config_load();
}


void print_board() {
  #if defined(BLUEPILL)
  Serial.print("Bluepill");
  #elif defined(ARDUINO_UNO)
  Serial.print("Arduino Uno");
  #elif defined(ARDUINO_MEGA)
  Serial.print("Arduino Mega");
  #elif defined(ARDUINO_LEONARDO)
  Serial.print("Arduino Leonardo");
  #else
  Serial.print("Undefined board");
  #endif
}


LiquidCrystal_I2C * get_lcd16x2_pointer_by_index(uint8_t index) {
  if ((index >= NUMBER_MAX_LCD16X2) || (index >= 8)) return 0; // Invalid index

  switch(index) {
    case 0: return &lcd16x2_0;
    case 1: return &lcd16x2_1;
    case 2: return &lcd16x2_2;
    case 3: return &lcd16x2_3;
    case 4: return &lcd16x2_4;
    case 5: return &lcd16x2_5;
    case 6: return &lcd16x2_6;
    case 7: return &lcd16x2_7;
  }
  
}

// ============================================================================
//         JOYSTICK
// ============================================================================


void set_joystick_axis(uint8_t axis_num, uint16_t value) {
  if (is_joystick_connected) {
    #if defined(BLUEPILL) || defined(ARDUINO_LEONARDO)
    switch(axis_num) {
      case 0: Joystick.X(value); break;
      case 1: Joystick.Y(value); break;
      case 2: Joystick.Xrotate(value); break;
      case 3: Joystick.Yrotate(value); break;
      case 4: Joystick.sliderRight(value); break;
      case 5: Joystick.sliderLeft(value); break;
    }
    #endif
  }
  else {
    Serial.print("{\"axis\":{\"");
    Serial.print(axis_num);
    Serial.print("\":");
    Serial.print(value);
    Serial.println("}}");
  }
}


void set_joystick_button(uint8_t btn_index, bool state) {
  if (is_joystick_connected) {
    #if defined(BLUEPILL) || defined(ARDUINO_LEONARDO)
    Joystick.button(btn_index+1, state); // Joystick.button indices start at 1, not 0
    #endif
  }
  else {
    Serial.print("{\"btn\":{\"");
    Serial.print(btn_index);
    Serial.print("\":");
    Serial.print(state? 1 : 0);
    Serial.println("}}");
  }
}













// ============================================================================
//         I2C
// ============================================================================


uint8_t read_expander(uint8_t exp_index) {
  uint8_t result = 0;
  Wire.requestFrom(0x20 + exp_index, 1);

  while (Wire.available()) {
    result = Wire.read();
  }
  
  return result;
}


void set_battery_display(uint8_t value, bool frame, bool blink_value, bool blink_frame) {
  uint8_t val = (blink_frame << 7) | (blink_value << 6) | (frame << 3) | value;
  Wire.beginTransmission(0x12);
  Wire.write(val);
  Wire.endTransmission();
}


void lcd16x2_begin(uint8_t lcd_index) {
  LiquidCrystal_I2C *lcd = get_lcd16x2_pointer_by_index(lcd_index); 
  if (!lcd) return;
  
  lcd->begin(&WireLCDs);
}


void lcd16x2_set_backlight(uint8_t lcd_index, uint8_t state) {
  LiquidCrystal_I2C *lcd = get_lcd16x2_pointer_by_index(lcd_index);
  if (!lcd) return;
  
  lcd->setBacklight(state? 1 : 0);
}


void lcd16x2_clear(uint8_t lcd_index) {
  LiquidCrystal_I2C *lcd = get_lcd16x2_pointer_by_index(lcd_index); 
  if (!lcd) return;
  
  lcd->clear();
}


void lcd16x2_write_message(uint8_t lcd_index, uint8_t row, uint8_t col, char * message) {
  LiquidCrystal_I2C *lcd = get_lcd16x2_pointer_by_index(lcd_index); 
  if (!lcd) return;
  
  lcd->setCursor(col, row);
  lcd->print(message);
}


// Servos are controlled by PCA9685 expander boards, such as https://www.aliexpress.com/item/32466332558.html
// IC Datasheet: https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf

void servos_initialize_board(uint8_t servo_expander_index) {
  uint8_t board_address = 0x40 | servo_expander_index; // Arduino uses 7-bit address
  
  // Board needs to be on sleep state to set frequency
  WireServos.beginTransmission(board_address);
  WireServos.write(0x00); // Register: MODE1
  WireServos.write(0b00110000); // internal clock, auto-increment, sleep, all other addresses disabled
  WireServos.endTransmission();  

  // Set the prescaler for a frequency of 50Hz (20ms update rate for servos)
  WireServos.beginTransmission(board_address);
  WireServos.write(0xFE); // Register: PRE_SCALE
  WireServos.write(121); // (25MHz / (4096 * frequency)) - 1 = 121 for a frequency of 50Hz (20ms period)
  WireServos.endTransmission();  

  // Bring the board to operational state
  WireServos.beginTransmission(board_address);
  WireServos.write(0x00); // Register: MODE1
  WireServos.write(0b00100000); // internal clock, auto-increment, osc on, all other addresses disabled
  WireServos.endTransmission();

  delay(1); // PCA9685 needs 500us for clock startup
  
  is_servo_expanders_initialized |= (1 << servo_expander_index); // Flag state as initialized
}


void servos_set_position(uint8_t servo_num, uint8_t servo_value_percent) {
  // Lower nibble in servo_num is board output index, higher nibble is board index
  uint8_t servo_expander_index = (servo_num >> 4) & 0x0F;

  uint8_t servo_index = (servo_num) & 0x0F;
  
  bool is_this_board_initialized = is_servo_expanders_initialized & (1 << servo_expander_index);

  if (!is_this_board_initialized) {
    // This board still needs to be initialized
    servos_initialize_board(servo_expander_index);
  }

  // [i2c address] [register base address] [ON_L] [ON_H] [OFF_L] [OFF_H]
  uint8_t register_base = 0x06 + servo_index*4; // First of the 4 bytes for this servo, as per datasheet

  // With the 50Hz prescaler, 1ms corresponds to 205 units (20ms = 4096)
  // Servos usually operate in the range 1ms - 2ms (where 1.5ms is center)
  // Cheap ones from aliexpress accept 0.5ms - 2.5ms, but it is wise to avoid the end of scale
  // as trying to move a servo past its limit might cause overcurrent
  uint16_t time_active = 205 + ((uint32_t)205*(uint32_t)servo_value_percent)/100;
  // Each servo is delayed 200us respective to the previous, to avoid 
  // current spikes from all turning on at the same time
  uint16_t time_delay_on = 41*servo_num; 
  // Turns off time_active after turning on
  uint16_t time_off = time_delay_on + time_active;
  // Write settings to board
  WireServos.beginTransmission(0x40 | servo_expander_index);
  WireServos.write(register_base); 
  WireServos.write((time_delay_on     ) & 0xFF); // LEDn_ON_L
  WireServos.write((time_delay_on >> 8) & 0xFF); // LEDn_ON_H
  WireServos.write((time_off          ) & 0xFF); // LEDn_OFF_L
  WireServos.write((time_off      >> 8) & 0xFF); // LEDn_OFF_H
  WireServos.endTransmission();
}









// ============================================================================
//         CONFIG
// ============================================================================


uint8_t add_axis(uint16_t pin, uint16_t axis, uint16_t vmin, uint16_t vmax) {
  if (config_object.used_axes >= NUMBER_OF_BUILTIN_AXES) return 0;

  config_object.axes[config_object.used_axes].status = 1;
  config_object.axes[config_object.used_axes].board_pin = pin;
  config_object.axes[config_object.used_axes].joystick_axis = axis;
  config_object.axes[config_object.used_axes].min_value = vmin;
  config_object.axes[config_object.used_axes].max_value = vmax;
  config_object.used_axes++;
  
  return 1;
}

uint8_t add_button(uint8_t exp_index, uint8_t exp_pin, uint8_t btn_index, bool invert, bool toggle) {
  if (config_object.used_buttons >= NUMBER_OF_JOYSTICK_BUTTONS) return 0;
  if ((exp_index >= NUMBER_MAX_EXPANDERS) || (exp_pin >= 8)) return 0;

  uint8_t status = 0x01;
  if (invert) status |= 0x02 | 0x08;
  if (toggle) status |= 0x04;

  config_object.buttons[config_object.used_buttons].status = status;
  config_object.buttons[config_object.used_buttons].expander_index = exp_index;
  config_object.buttons[config_object.used_buttons].expander_pin = exp_pin;
  config_object.buttons[config_object.used_buttons].joystick_button = btn_index;
  config_object.used_buttons++;

  must_read_io = 1;
  
  return 1;
}


uint8_t add_lcd16x2(uint8_t lcd_address_index, uint8_t lcd_num) {
  if ((lcd_address_index >= 8) || (lcd_num >= NUMBER_MAX_LCD16X2)) return 0;

  Lcd16x2Config_t * lcd_data = (config_object.lcds16x2 + lcd_num);

  lcd_data->lcd_index = lcd_address_index;
  lcd16x2_begin(lcd_address_index);
  lcd_data->status = 0x03;

  return 1;
}


void config_clear() {
  for (uint8_t i=0; i<NUMBER_OF_BUILTIN_AXES; i++) {
    config_object.axes[i].status = 0;
  }
  config_object.used_axes = 0;
  
  for (uint8_t i=0; i<NUMBER_OF_JOYSTICK_BUTTONS; i++) {
    config_object.buttons[i].status = 0;
  }
  config_object.used_buttons = 0;

  for (uint8_t i=0; i<8; i++) {
    config_object.lcds16x2[i].status = 0;
  }
}

void config_dump() {
  Serial.print("{\"ver\":\"0.2\",\"board\":\"");
  print_board();
  Serial.print("\",\"usb_connected\":");
  Serial.print(is_joystick_connected);
  Serial.print(",\"used_axes\":");
  Serial.print(config_object.used_axes);
  Serial.print(",\"axes\":{");
  for (uint8_t i=0; i<config_object.used_axes; i++) {
    Serial.print('"');
    Serial.print(i);
    Serial.print("\":[");
    //Serial.print(config_object.axes[i].status); Serial.print(",");
    Serial.print(config_object.axes[i].board_pin); Serial.print(",");
    Serial.print(config_object.axes[i].joystick_axis); Serial.print(",");
    Serial.print(config_object.axes[i].min_value); Serial.print(",");
    Serial.print(config_object.axes[i].max_value); 
    Serial.print("]");
    
    if (i < (config_object.used_axes-1)) Serial.print(",");
  }

  Serial.print("},\"used_expanders\":");
  Serial.print(config_object.used_expanders);
  Serial.print(",\"used_buttons\":");
  Serial.print(config_object.used_buttons);
  Serial.print(",\"buttons\":{");
  for (uint8_t i=0; i<config_object.used_buttons; i++) {
    Serial.print('"');
    Serial.print(i);
    Serial.print("\":[");
    Serial.print(config_object.buttons[i].expander_index); Serial.print(",");
    Serial.print(config_object.buttons[i].expander_pin); Serial.print(",");
    Serial.print(config_object.buttons[i].joystick_button); Serial.print(",");
    Serial.print( (config_object.buttons[i].status & 0x02)? '1' : '0'); Serial.print(",");
    Serial.print( (config_object.buttons[i].status & 0x04)? '1' : '0');
    Serial.print("]");
    
    if (i < (config_object.used_buttons-1)) Serial.print(",");
  }

  Serial.print("},\"lcd16x2\":{");
  for (uint8_t i=0; i<8; i++) {
    Serial.print('"');
    Serial.print(i);
    Serial.print("\":[");
    Serial.print(config_object.lcds16x2[i].status & 0x01); Serial.print(",");
    Serial.print(config_object.lcds16x2[i].lcd_index);
    Serial.print("]");

    if (i < 7) Serial.print(",");
  }
  

  Serial.println("}}");
}



bool config_load() {
  // ---------------- BLUEPILL ----------------
  #if defined(BLUEPILL)
  // Bluepill uses emulated EEPROM which makes life a bit harder
  uint16_t status;
  uint16_t data16;
  bool had_read_error = 0;
  uint16_t * eeprom_buf = (uint16_t *)&config_object;
  uint16_t eeprom_buf_len = 0xFFFF;

  // Address 0 contains length (in uint16) of stored object
  // Address 1 onwards contains data

  // Read address 0 and compare to expected
  status = EEPROM.read(EEPROM_ADDRESS_OFFSET, &eeprom_buf_len);
  if (status != EEPROM_OK) {
    if (status == EEPROM_BAD_ADDRESS) {
      Serial.println("{\"msg\":\"Config was never saved\"}");
    }
    else {
      Serial.print("{\"msg\":\"EEPROM read error: ");
      Serial.print(status);
      Serial.println("\"}");
    }
    return 0; // EEPROM error
  }
  
  if (eeprom_buf_len != EEPROM_STORAGE_SIZE) {
    Serial.println("{\"msg\":\"Config was never saved\"");
    return 0; 
  }

  // Read binary data
  for (uint16_t i=0; i<EEPROM_STORAGE_SIZE; i++) {
    if (EEPROM.read(EEPROM_ADDRESS_OFFSET + 1 + i, &data16) != EEPROM_OK) {
      had_read_error = 1;
      break;
    }
    else eeprom_buf[i] = data16; // config_object struct overwritten one uint16 at a time
  }

  // If reading failed, reset config (but don't overwrite save)
  if (had_read_error || (config_object.used_axes >= NUMBER_OF_BUILTIN_AXES) || (config_object.used_buttons >= NUMBER_OF_JOYSTICK_BUTTONS)) {
    config_clear();
    return 0;
  }


  // ---------------- ARDUINOS ----------------
  #else
  // Native Arduino boards have a much simpler and more straight-forward EEPROM
  uint8_t * eeprom_buf = (uint8_t *)&config_object;
  uint16_t eeprom_buf_len = 0xFFFF;  
  uint16_t eeprom_storage_size_bytes = EEPROM_STORAGE_SIZE*2;

  // Addresses 0 and 1 have the uint16 size of config object
  // Address 2 onward has binary data
  
  eeprom_buf_len = ((uint16_t)EEPROM.read(EEPROM_ADDRESS_OFFSET) << 8) | EEPROM.read(EEPROM_ADDRESS_OFFSET + 1);

  if (eeprom_buf_len != eeprom_storage_size_bytes) {
    Serial.println("{\"msg\":\"Config was never saved\"");
    return 0; 
  }

  // Read binary data
  for (uint16_t i=0; i<eeprom_storage_size_bytes; i++) {
    eeprom_buf[i] = EEPROM.read(EEPROM_ADDRESS_OFFSET + 2 + i);
  }
  
  #endif


  // Reset state flags from the config which are contextual
  // and could have been saved as garbage
  for (uint8_t i=0; i<config_object.used_buttons; i++) {
    // Reset LAST_STATE flag from buttons
    config_object.buttons[i].status = (config_object.buttons[i].status & ~(0x08)); // clear old value
    config_object.buttons[i].status |= (config_object.buttons[i].status & 0x02)? 0x08 : 0; // If INVERT, LAST_STATE = 1
  }

  // Initialize LCDs regardless of their previous states
  for (uint8_t i=0; i<8; i++) {
    if ((config_object.lcds16x2[i].status & 0x01)) {
      lcd16x2_begin(config_object.lcds16x2[i].lcd_index);
      config_object.lcds16x2[i].status |= 0x02;
    }
  }
  
  
  // All is sorted
  must_read_io = 1;
  Serial.println("{\"msg\":\"Config loaded\"}");
  return 1;
}



void config_dump_eeprom() {
  #if defined(BLUEPILL)
  uint16_t status;
  uint16_t data16;
  
  for (uint16_t i=0; i<EEPROM_STORAGE_SIZE+1; i++) {
    status = EEPROM.read(EEPROM_ADDRESS_OFFSET + i, &data16);
    if (status != EEPROM_OK) {
      Serial.print(" - error: ");
      Serial.println(status);
      break;
    }
    else {
      char buf[12];
      sprintf(buf, "0x%04x", data16);
      Serial.println(buf);
    }
  }

  #else
  uint16_t eeprom_storage_size_bytes = EEPROM_STORAGE_SIZE*2;
  
  for (uint16_t i=0; i<eeprom_storage_size_bytes+2; i++) {
      Serial.println(EEPROM.read(EEPROM_ADDRESS_OFFSET + i), HEX);
  }

  #endif
}




bool config_save() {
  // ---------------- BLUEPILL ----------------
  #if defined(BLUEPILL)
  // Bluepill uses emulated EEPROM which makes life a bit harder
  uint16_t status;
  uint16_t data16;
  uint16_t * eeprom_buf = (uint16_t *)&config_object;

  // Address 0 contains length (in uint16) of stored object
  // Address 1 onwards contains data

  status = EEPROM.write(EEPROM_ADDRESS_OFFSET, EEPROM_STORAGE_SIZE);
  if (status != EEPROM_OK) {
    Serial.print("{\"msg\":\"Error saving config: ");
    Serial.print(status);
    Serial.println("\"");
    return 0;
  }

  // Write binary data
  for (uint16_t i=0; i<EEPROM_STORAGE_SIZE; i++) {
    if (EEPROM.write(EEPROM_ADDRESS_OFFSET + 1 + i, eeprom_buf[i]) != EEPROM_OK) {
      Serial.println("{\"msg\":\"Error saving config\"");
      return 0;
    }
  }

  // All is sorted
  Serial.println("{\"msg\":\"Config saved\"");
  return 1;


  // ---------------- ARDUINOS ----------------
  #else
  // Native Arduino boards have a much simpler and more straight-forward EEPROM
  uint8_t * eeprom_buf = (uint8_t *)&config_object;
  uint16_t eeprom_storage_size_bytes = EEPROM_STORAGE_SIZE*2;

  EEPROM.write(EEPROM_ADDRESS_OFFSET, (eeprom_storage_size_bytes >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDRESS_OFFSET + 1, eeprom_storage_size_bytes & 0xFF);
  
  // Write binary data
  for (uint16_t i=0; i<eeprom_storage_size_bytes; i++) {
    EEPROM.write(EEPROM_ADDRESS_OFFSET + 2 + i, eeprom_buf[i]);
  }

  // All is sorted
  Serial.println("{\"msg\":\"Config saved\"}");
  return 1;
  
  #endif
  
}











// ============================================================================
//         SERIAL PORT TERMINAL
// ============================================================================


char serial_buffer[SERIAL_BUFFER_SIZE];
uint16_t serial_buffer_cursor = 0;


bool starts_with(char * str_buffer, char * str_search) {
  while (*str_search != 0) {
    if (*str_search != *str_buffer) return 0;

    str_buffer++;
    str_search++;
  }
  return 1;
}

bool is_equal_to(char * str_buffer, char * str_search) {
  while (*str_search != 0) {
    if (*str_search != *str_buffer) return 0;

    str_buffer++;
    str_search++;
  }
  if (*str_search != *str_buffer) return 0; // null termination must also match
  
  return 1;
}

/**
 * Called periodically to check the serial port and process the terminal tasks.
 * If receiving \r or \n, process what already is in the buffer (without adding
 * the \r or \n). For any other character, adds to buffer.
 */
void serial_terminal_check() {
  //Serial.print(".");
  while (Serial.available()) {
    char recv = Serial.read();

    if ((recv == 13) || (recv == 10)) {
      if (serial_buffer_cursor > 0) { 
        serial_buffer[serial_buffer_cursor] = 0; // Make a null-terminated string before proceeding
        serial_process_buffer();
        serial_buffer_cursor = 0;
      }
    }
    else {
      serial_add_to_buffer(recv);
    }
    
  }
}

/**
 * Adds a character to buffer
 */
void serial_add_to_buffer(char c) {
  if (serial_buffer_cursor >= SERIAL_BUFFER_SIZE) return;
  serial_buffer[serial_buffer_cursor++] = c;
}

/**
 * Extract a value from a starting index in the serial buffer,
 * ending when a ',' is found or end of string.
 * Returns a serial_param_t struct where the int_value will
 * have the corresponding numerical value if the param is a number
 * otherwise the param string length
 */
serial_param_t serial_extract_param(uint16_t start_index) {
  char param_buf[12];
  uint8_t param_cur = 0;
  uint8_t i = start_index;
  bool found_comma = 0;
  bool has_int_only = 1;
  uint8_t status;
  char * str_address;

  // Case 1: empty param with end of string
  if (!serial_buffer[i]) {
    status = SERIAL_PARAM_EMPTY_END;
    found_comma = 0;
    has_int_only = 0;
  }
  
  // Case 2: empty param followed by others
  else if (serial_buffer[i] == ',') {
    status = SERIAL_PARAM_EMPTY_FOLLOW;
    found_comma = 1;
    has_int_only = 0;
  }

  // Case 3: there is content
  else {
    str_address = serial_buffer + i;
    
    while (serial_buffer[i]) {
      char c = serial_buffer[i++];
      if (c == ',') {
        found_comma = 1;
        break;
      }
      else {
        param_buf[param_cur++] = c;
        if ( ((c < '0') || (c > '9')) && (c != '-')) has_int_only = 0;
      }
    }
    param_buf[param_cur] = 0; // null-terminate

    status = (has_int_only) ? SERIAL_PARAM_INT : SERIAL_PARAM_OTHER;
  }

  uint32_t value = (has_int_only)? atoi(param_buf) : param_cur; // If value is not int, use string length

    
  serial_param_t result;
  result.next_param_index = (found_comma)? i : 0;
  result.status = status;
  result.int_value = value;
  result.string_value = str_address;
  return result;
}


bool serial_add_axis(uint16_t start_index) {
  uint8_t pin;
  uint8_t axis;
  uint16_t vmin;
  uint16_t vmax;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  pin = param.int_value;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT)) return 0; // Invalid value
  axis = param.int_value;

  if (param.next_param_index) {
    // Ranges are specified, both must be present

    param = serial_extract_param(param.next_param_index);
    if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
    vmin = param.int_value;
    
    param = serial_extract_param(param.next_param_index);
    if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
    vmax = param.int_value;
  }

  else {
    vmin = 0;
    vmax = 1023;
  }

  if ((pin >= NUMBER_OF_BUILTIN_AXES) || (axis >= NUMBER_OF_BUILTIN_AXES) || (vmin > 1023) || (vmax > 1023)) 
    return 0; // Invalid values

  add_axis(pin, axis, vmin, vmax);
  
  return 1;
}

bool serial_add_button(uint16_t start_index) {
  uint8_t exp_index;
  uint8_t exp_pin;
  uint8_t btn_index;
  bool invert;
  bool toggle;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  exp_index = param.int_value;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  exp_pin = param.int_value;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT)) return 0; // Invalid value
  btn_index = param.int_value;

  if (param.next_param_index) {
    // Invert and toggle are specified

    param = serial_extract_param(param.next_param_index);
    if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
    invert = (param.int_value)? 1 : 0;
    
    param = serial_extract_param(param.next_param_index);
    if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
    toggle = (param.int_value)? 1 : 0;
  }

  else {
    invert = 0;
    toggle = 0;
  }

  if ((exp_index >= NUMBER_MAX_EXPANDERS) || (exp_pin >= 8) || (btn_index >= NUMBER_OF_JOYSTICK_BUTTONS)) 
    return 0; // Invalid values

  add_button(exp_index, exp_pin, btn_index, invert, toggle);
  
  return 1;
}


bool serial_add_lcd16x2(uint16_t start_index) {
  uint8_t lcd_address_index;
  uint8_t lcd_num;

  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  lcd_address_index = param.int_value;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
  lcd_num = param.int_value;

  if ((lcd_address_index >= 8) || (lcd_num >= NUMBER_MAX_LCD16X2)) return 0;

  add_lcd16x2(lcd_address_index, lcd_num);
  
  return 1;
}

bool serial_lcd16x2_clear(uint16_t start_index) {
  uint8_t lcd_num;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
  lcd_num = param.int_value;
  if (lcd_num >= NUMBER_MAX_LCD16X2) return 0;

  Lcd16x2Config_t * lcd_data = (config_object.lcds16x2 + lcd_num);
  
  if ((lcd_data->status & 0x03) != 0x03) return 0; // LCD not initialized
  
  lcd16x2_clear(lcd_data->lcd_index);
  
  return 1;
}

bool serial_lcd16x2_set_backlight(uint16_t start_index) {
  uint8_t lcd_num;
  bool backlight_state;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  lcd_num = param.int_value;
  if (lcd_num >= NUMBER_MAX_LCD16X2) return 0;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
  backlight_state = (param.int_value)? 1 : 0;

  Lcd16x2Config_t * lcd_data = (config_object.lcds16x2 + lcd_num);
  
  if ((lcd_data->status & 0x03) != 0x03) return 0; // LCD not initialized
  
  lcd16x2_set_backlight(lcd_data->lcd_index, backlight_state);
  
  return 1;
}

bool serial_lcd16x2_write_message(uint16_t start_index) {
  uint8_t lcd_num;
  uint8_t col;
  uint8_t row;
  char * message;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  lcd_num = param.int_value;
  if (lcd_num >= NUMBER_MAX_LCD16X2) return 0;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  row = param.int_value;
  if (row >= 2) return 0;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  col = param.int_value;
  if (row >= 16) return 0;

  param = serial_extract_param(param.next_param_index);
  if ( ((param.status != SERIAL_PARAM_OTHER) && (param.status != SERIAL_PARAM_INT)) || (param.next_param_index)) return 0; // Invalid value or not last value
  message = param.string_value;
  if (!message) return 0;
  
  Lcd16x2Config_t * lcd_data = (config_object.lcds16x2 + lcd_num);
  
  if ((lcd_data->status & 0x03) != 0x03) return 0; // LCD not initialized
  
  lcd16x2_write_message(lcd_data->lcd_index, row, col, message);
  
  return 1;
}


bool serial_servo_set_value(uint16_t start_index) {
  uint8_t servo_num;
  uint8_t value;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  if ( (param.int_value < 0) || (param.int_value >= 256) ) return 0; // Must fit a uint8_t, apart from this any value is valid
  servo_num = param.int_value;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
  if ( (param.int_value < 0) || (param.int_value > 100) ) return 0; // Must be a round percentage
  value = param.int_value;

  servos_set_position(servo_num, value);
  
  return 1;
}


// This refers to the module <https://www.aliexpress.com/item/2025558433.html> connected to a driver arduino pro mini
// Firmware for this driver pro mini should also be included in the project repository
bool serial_set_bat_driver(uint16_t start_index) {
  uint8_t value;
  bool frame;
  bool blink;
  bool frame_blink;
  
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
  value = param.int_value;

  param = serial_extract_param(param.next_param_index);
  if ((param.status != SERIAL_PARAM_INT)) return 0; // Invalid value or last value
  frame = (param.int_value)? 1 : 0;

  if (param.next_param_index) {
    // Blinks specified

    param = serial_extract_param(param.next_param_index);
    if ((param.status != SERIAL_PARAM_INT) || (!param.next_param_index)) return 0; // Invalid value or last value
    blink = (param.int_value)? 1 : 0;
    
    param = serial_extract_param(param.next_param_index);
    if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not last value
    frame_blink = (param.int_value)? 1 : 0;
  }

  else {
    blink = 0;
    frame_blink = 0;
  }

  if (value >= 6) return 0; // Invalid value

  set_battery_display(value, frame, blink, frame_blink);
  
  return 1;
}


bool serial_set_num_expanders(uint16_t start_index) {
  serial_param_t param = serial_extract_param(start_index);
  if ((param.status != SERIAL_PARAM_INT) || (param.next_param_index)) return 0; // Invalid value or not single
  if (param.int_value >= NUMBER_MAX_EXPANDERS) return 0; // Invalid number
  
  config_object.used_expanders = param.int_value;
  return 1;
}


void serial_print_message(char * message) {
  Serial.print("{\"msg\":\"");
  Serial.print(message);
  Serial.println("\"}");
}

void serial_print_error(char * received) {
  Serial.print("{\"msg\":\"ERROR\",\"received\":\"");
  Serial.print(received);
  Serial.println("\"}");
}


void serial_print_ok() {
  serial_print_message("OK");
}

void serial_print_error() {
  serial_print_message("ERROR");
}


/**
 * Process the contents in the serial buffer, performing the corresponding actions synchronously
 */
void serial_process_buffer() {
  uint16_t buflen = serial_buffer_cursor;

  // ==================================================
  // CONFIGURATION MESSAGES
  
  if (is_equal_to(serial_buffer, "AT")) {
    serial_print_message("AT OK");
  }
  
  else if (is_equal_to(serial_buffer, "CLEAR")) {
    config_clear();
    serial_print_ok();
  }
  
  else if (is_equal_to(serial_buffer, "DUMP")) {
    config_dump();
    serial_print_ok();
  }

  else if (is_equal_to(serial_buffer, "DUMPEEPROM")) {
    config_dump_eeprom();
    serial_print_ok();
  }

  else if (is_equal_to(serial_buffer, "RELOAD")) {
    config_load();
    serial_print_ok();
  }

  else if (is_equal_to(serial_buffer, "SAVE")) {
    config_save();
    serial_print_ok();
  }

  else if (starts_with(serial_buffer, "AXIS=")) {
    if (serial_add_axis(5)) serial_print_ok();
    else                    serial_print_error();    
  }
  
  else if (starts_with(serial_buffer, "BTN=")) {
    if (serial_add_button(4)) serial_print_ok();
    else                      serial_print_error();    
  }
  
  else if (starts_with(serial_buffer, "EXP=")) {
    if (serial_set_num_expanders(4)) serial_print_ok();
    else                             serial_print_error();    
  }

  else if (starts_with(serial_buffer, "LCD16X2=")) {
    if (serial_add_lcd16x2(8)) serial_print_ok();
    else                       serial_print_error();    
  }


 
  // ==================================================
  // PROPERTY WRITING MESSAGES
  
  // Property writing messages are as short as possible to save bandwidth, as they 
  // are heavily used in-game. They all start with !

  else if (starts_with(serial_buffer, "!LCB=")) {
    if (serial_lcd16x2_set_backlight(5)) serial_print_ok();
    else                                  serial_print_error();    
  }

  else if (starts_with(serial_buffer, "!LCC=")) {
    if (serial_lcd16x2_clear(11)) serial_print_ok();
    else                          serial_print_error();    
  }

  else if (starts_with(serial_buffer, "!LC=")) {
    if (serial_lcd16x2_write_message(4)) serial_print_ok();
    else                                  serial_print_error();    
  }

  else if (starts_with(serial_buffer, "!S=")) {
    if (serial_servo_set_value(3)) serial_print_ok();
    else                           serial_print_error();    
  }

  else if (starts_with(serial_buffer, "!BT=")) {
    if (serial_set_bat_driver(4)) serial_print_ok();
    else                          serial_print_error();    
  }

  // Unknown command. We fail silently to allow garbage to be flushed
  // by sending a '\n' at any time.
}

// ============================================================================
// MAIN LOOP


bool toggle_needs_reset = 0; // is set to 1 when we have to turn a toggled button off next frame
void loop() {
  
  serial_terminal_check();


  
  //int axis_0 = analogRead(PA0);
  //Serial.println(axis_0);

  // AXES
  // TODO: instead of sending a JSON message for each axis, group them and send a single message with all axes
  for (uint8_t axis_i = 0; axis_i < config_object.used_axes; axis_i++) {
    AxisConfig_t* axis_data = (config_object.axes + axis_i); // points to axes[axis_i]
    if (axis_data->status == 0) continue; // unused axis
    
    uint16_t axis_raw = analogRead(AXIS_PINS[axis_data->board_pin]);
    uint16_t axis_value = (axis_raw > axis_data->min_value) ? map(axis_raw, axis_data->min_value, axis_data->max_value, JOYAXIS_MIN, JOYAXIS_MAX) : 0;

    set_joystick_axis(axis_data->joystick_axis, axis_value);
    
  }

  bool has_to_reset_toggles = toggle_needs_reset; // dual variable to cause a 1 frame delay
  toggle_needs_reset = 0;
  
  // EXPANDERS
  if (digitalRead(EXPANDER_IRQ_PIN) == 0) must_read_io = 1; // must_read_io may be set from other sources as well
  
  if (must_read_io || has_to_reset_toggles) {
    must_read_io = 0;
    uint8_t exp_data[4];
    for (uint8_t exp_i=0; exp_i<config_object.used_expanders; exp_i++) {
      exp_data[exp_i] = read_expander(exp_i);
    }
    

    for (uint8_t btn_i=0; btn_i < config_object.used_buttons; btn_i++) {
      ButtonConfig_t * btn_data = (config_object.buttons + btn_i); // boints to buttons[btn_i]
      if (btn_data->status == 0) continue; // unused button    status = [ xxxx | LAST_STATE | TOGGLE | INVERTED | ENABLED ]

      bool invert     = (btn_data->status & 0x02) ? 1 : 0;
      bool toggle     = (btn_data->status & 0x04) ? 1 : 0;
      bool last_state = (btn_data->status & 0x08) ? 1 : 0;
      bool is_pressed = (exp_data[ btn_data->expander_index ] & (1 << btn_data->expander_pin))? 0 : 1;
      uint8_t btn_index = btn_data->joystick_button;
      
      if (invert) is_pressed = !is_pressed;

      if (toggle) {
        // Button works in toggle mode
        
        if (is_pressed != last_state) {
          // We have a pin change
          set_joystick_button(btn_index, 1);
          toggle_needs_reset = 1;
        }
        
        else if (has_to_reset_toggles) {
          // No pin change, but we have to reset the toggle
          set_joystick_button(btn_index, 0);
        }
        
      }
      
      else if (is_pressed != last_state) {
        // Button works in continuous (normal) mode
        set_joystick_button(btn_index, is_pressed);
      }

      // For all button types:
      if (is_pressed) {
        btn_data->status |= 0x08; // set last_pressed = 1 
      }
      else {
        btn_data->status &= ~0x08; // set last_pressed = 0 
      }
    
    } // end of for each button map
   
  }





  if (is_joystick_connected) {
    #if defined(BLUEPILL) || defined(ARDUINO_LEONARDO)
    Joystick.send();
    delay(20); // 50 Hz joystick frame rate
    #endif
  }
  else delay(50); // 20 Hz Serial frame rate - lower as the Serial speed is inherently lower as well (115 kbps versus several Mbps for USB)

}

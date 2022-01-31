#include <avr/sleep.h>

#include <DS3231.h>
#include <Wire.h>

#define VSENSE_BAT   A0
#define VSENSE_NTC   A1
#define VSENSE_SOLAR A2
#define VSENSE_SUB   A3

#define RTC_INTERRUPT 2
#define LOAD_SWITCH 13

volatile int transaction_complete = true;
volatile int new_command = false;
volatile uint8_t command_response = 0x0;

#define CMD_BUF_LEN 2
volatile uint8_t cmd_buffer[CMD_BUF_LEN];

void receive_command(int bytes_in) {
  transaction_complete = false;

  for (int i = 0; i < bytes_in; i++) {
    if (i < CMD_BUF_LEN) {
      cmd_buffer[i] = Wire.read();
    } else {
      // drop
      Wire.read();
    }
  }

  new_command = true;
}

void respond_command(void) {
  Wire.write(command_response);
  command_response = 0;
  transaction_complete = true;
}

void switch_load_on(void) {
  digitalWrite(LOAD_SWITCH, LOW);
}

void setup() {
  pinMode(RTC_INTERRUPT, INPUT_PULLUP);
  pinMode(LOAD_SWITCH, OUTPUT);
  digitalWrite(LOAD_SWITCH, HIGH);
  
  attachInterrupt(digitalPinToInterrupt(RTC_INTERRUPT), switch_load_on, RISING);

  Serial.begin(115200);

  Wire.begin(0x8);
  Wire.onReceive(receive_command);
  Wire.onRequest(respond_command);
 
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void loop() {

  sleep_mode();
  // Code resumes here when woken by I2C address match interrupt
  Serial.println("up!");

  if (new_command == true) {
    process_command(cmd_buffer);
    clear_buffer();
    new_command = false;
  }
  
  while(!transaction_complete);
  delay(10);
}

#define CMD_READ_VOLTAGE  0xa1
#define CMD_WRITE_PIN     0xa2

uint16_t read_pin(int analog_pin_num) {
  int analog_pin;
  
  switch (analog_pin_num) {
    case 0:
      analog_pin = VSENSE_BAT;
      break;
    case 1:
      analog_pin = VSENSE_BAT;
      break;
    case 2:
      analog_pin = VSENSE_NTC;
      break;
    case 3:
      analog_pin = VSENSE_SUB;
      break;
    default:
      analog_pin = A0;
  }

  return analogRead(analog_pin);
}

int process_command(volatile uint8_t* cmd_buffer) {
  int rc = 0;
  // Serial.println(cmd_buffer[0]);
  switch(cmd_buffer[0]) {
      case CMD_READ_VOLTAGE:
      {
        // adc val is 10 bits, round down to 8 bits for command
        uint16_t pin_voltage = analogRead(VSENSE_BAT);
        command_response = (uint8_t) pin_voltage;
        break;
      }
      case CMD_WRITE_PIN:
      {
        digitalWrite(LOAD_SWITCH, cmd_buffer[1]);
        transaction_complete = true;
        break;
      }
      default:
        // stats.unknown_cmd++;
        transaction_complete = true;
        break;
  }
  
  return rc;
}

void clear_buffer(void) {
  for (int i = 0; i < CMD_BUF_LEN; i++) {
    cmd_buffer[i] = 0;
  }
}
    

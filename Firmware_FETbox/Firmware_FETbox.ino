/* 
  Firmware for PlatePerf FETbox hardware controller. 
  Target PCB rev.0, Arduino Nano V3
  
  Copyright Robert Pazdzior (2020-2021)
  github.com/robertpazdzior/PlateFlo-FETbox
  rpazdzior@protonmail.com
  
  Licensed under CERN Open Hardware License (CERN-OHL-W)
*/

/*======CONFIG=====*/
#define BAUD 115200 // USB serial baud rate
#define ID 0		    // Controller ID [0-9], change when using multiple units
/*=================*/


/* Macros for serial command parsing */
#define CMD_START     '@'   // start of command character
#define CMD_END       '\n'  // end of command character

#define CMD_ID        '#'   // query device ID
#define CMD_HEARTBEAT '?'   // ping device heartbeat
#define CMD_ENABLE    'H'   // enable channel output
#define CMD_DISABLE   'I'   // disable channel output
#define CMD_HIT_HOLD  'V'   // 'hit and hold' channel output
#define CMD_PWM       'S'   // set channel output PWM duty (0-255)
#define CMD_DIG_READ  'D'   // read digital pin
#define CMD_ANA_READ  'A'   // read analog pin
#define CMD_DIG_WRITE 'E'   // write value to digital pin
#define CMD_ANA_WRITE 'B'   // write 10-bit value to analog pin

/*
Command structure:
    @[CMD]<[PARAM_1]><[PARAM_2]>...<[PARAM_n]>\n
    |  |    |             |             |      |
    |  |    |             |             |      Line feed (LF)
    |  |    |             |             
    |  |    |             Additional parameters, variable length
    |  |    Additional parameter, eg. channel #
    |  Command, single ASCII character
    Command start
*/

#define PIN_CHAN1 3
#define PIN_CHAN2 5
#define PIN_CHAN3 6
#define PIN_CHAN4 9
#define PIN_CHAN5 10

int chan_pin[5] = {PIN_CHAN1, PIN_CHAN2, PIN_CHAN3, PIN_CHAN4, PIN_CHAN5};
char inCmd[20];

void hit_n_hold(int _pin, int _duty = 127, int _delay = 100) {
  /* Reduces heat in solenoid valves, might require tuning of *_duty* */
  digitalWrite(_pin, HIGH); // Hit
  delay(_delay);
  analogWrite(_pin, _duty); // Hold
}

void ack() {
  Serial.write("*\n");
}

void ack_err() {
  Serial.write("#\n");
}

void serial_listen(char* cmdArray) {
    unsigned int lenCmd = 20;
    char inCmd[lenCmd];
    bool cmdStarted = false, cmdFinished = false;
    unsigned int idx;

    while(Serial.available() > 0) {
        char inChar = Serial.read();
        if( (inChar == CMD_END) && cmdStarted) {
            cmdFinished = true;
        }

        else if (inChar == CMD_START) {
            cmdStarted = true;
            cmdFinished = false;
            idx = 0;
        }

        else if (cmdStarted && !cmdFinished && ( (idx+1) <= lenCmd) ) {
            inCmd[idx] = inChar;
            idx++;
            inCmd[idx] = '\0';
        }

        if (cmdStarted && cmdFinished) {
            idx = 0;
            cmdStarted = false;
            cmdFinished = false;

            /* Write to cmd array when finished */
            memcpy(cmdArray, inCmd, lenCmd);

            for(uint16_t i = 0; i < sizeof(inCmd); i++) {
               inCmd[i] = '\0';
            }
        }
    }
}

void cmd_interpret(char* cmd) {
    /* Module ID query */
    if(cmd[0] == CMD_ID) {
      Serial.print("fetbox");
      Serial.print(ID);
      Serial.write("\n");
    }

    /* Heartbeat */
    else if(cmd[0] == CMD_HEARTBEAT) {
        ack();
    }

    /* Enable channel output */
    else if(cmd[0] == CMD_ENABLE) {
      int _chan = (cmd[1]-'0');
      if( (_chan > 0) & (_chan < 6) ) {
        digitalWrite(chan_pin[_chan-1], HIGH);
        ack();
      }
      else {
        ack_err();
      }
    }

    /* Enable channel output with hit/hold power reduction */
    else if(cmd[0] == CMD_HIT_HOLD) {
      int _chan = (cmd[1]-'0');
      bool _validParams = true;
      
      char _tempPWM[4];

      for(int i=2; i < 5; i++) {
        if(cmd[i] == '\0') _validParams = false;
        _tempPWM[i-2] = cmd[i];
      }
      _tempPWM[4] = '\0';
      int _pwm = atoi(_tempPWM);
      if( (_pwm < 0) | (_pwm > 255) ) _validParams = false;

      if( (_chan > 0) & (_chan < 6) & _validParams) {
        hit_n_hold(chan_pin[_chan-1], _pwm);
        ack();
      }
      else {
        ack_err();
      }
    }

    /* Enable channel output at given PWM duty cycle */
    else if(cmd[0] == CMD_PWM) {
      bool _validParams = true;
      int _chan = (cmd[1]-'0');
      char _tempPWM[4];

      for(int i=2; i < 5; i++) {
        if(cmd[i] == '\0') _validParams = false;
        _tempPWM[i-2] = cmd[i];
      }
      _tempPWM[4] = '\0';

      int _pwm = atoi(_tempPWM);
      if( (_pwm < 0) | (_pwm > 255) ) _validParams = false;

      if( (_chan > 0) & (_chan < 6) & _validParams) {
        analogWrite(chan_pin[_chan-1], _pwm);
        ack();
      }
      else {
        ack_err();
      }
    }

    /* Disable channel output */
    else if(cmd[0] == CMD_DISABLE) {
      int _chan = (cmd[1]-'0');
      if( (_chan > 0) & (_chan < 6) ) {
        digitalWrite(chan_pin[_chan-1], LOW);
        ack();
      }
      else {
        ack_err();
      }
    }

    /* Read digital pin */
    else if (cmd[0] == CMD_DIG_READ) {
      char _tempPin[3];

      // Parse command for pin, convert to integer
      for(int i=1; i < 3; i++) {
        _tempPin[i-1] = cmd[i];
      }
      _tempPin[3] = '\0';
      int _pin = atoi(_tempPin);

      // Validate pin number: 0-13 => D0-D13; 14-19 => A0-A5 (A6/A7 analog-only)
      if( (_pin >= 0) & (_pin < 20) ) {
        int reading = digitalRead(_pin);
        Serial.print(reading);
        Serial.print('\n');
      }
      else {
        ack_err();
      }
    }

    /* Read analog pin */
    else if (cmd[0] == CMD_ANA_READ) {
      char _tempPin[3];

      // Parse command for pin, convert to integer
      for(int i=1; i < 3; i++) {
        _tempPin[i-1] = cmd[i];
      }
      _tempPin[3] = '\0';
      int _pin = atoi(_tempPin);

      // Validate pin number: 14-21 => A0-A7
      if( (_pin > 13) & (_pin < 22) ) {
        int reading = analogRead(_pin);
        Serial.print(reading);
        Serial.print('\n');
      }
      else {
        ack_err();
      }
    }

    /* Write digital pin */
    else if (cmd[0] == CMD_DIG_WRITE) {
      char _tempPin[3];
      char _tempVal[2];

      // Parse command for pin, convert to integer
      for(int i=1; i < 3; i++) {
        _tempPin[i-1] = cmd[i];
      }
      _tempPin[3] = '\0';
      int _pin = atoi(_tempPin);

      // Parse command for output value, convert to integer
      _tempVal[0] = cmd[4];
      _tempVal[1] = '\0';
      int _val = atoi(_tempVal);

      // Validate pin number: 0-13 => D0-D13; 14-19 => A0-A5 (A6/A7 analog-only)
      // and value (0-1)
      if( ( (_pin >= 0) & (_pin < 22) ) & ( (_val == 1) | (_val == 0) )) {
        digitalWrite(_pin, _val);
        ack();
      }
      else {
        ack_err();
      }
    }


    /* Write PWM pin */
    else if (cmd[0] == CMD_ANA_WRITE) {
      char _tempPin[3];
      char _tempVal[4];

      // Parse command for pin, convert to integer
      for(int i=1; i < 3; i++) {
        _tempPin[i-1] = cmd[i];
      }
      _tempPin[3] = '\0';
      int _pin = atoi(_tempPin);

      // Parse command for PWM value, convert to integer
      for(int i=3; i < 6; i++) {
        _tempVal[i-3] = cmd[i];
      }
      _tempVal[4] = '\0';
      int _pwm = atoi(_tempVal);

      // Validate pin number: Nano PWM pins = 3,5,6,9,10,11. PWM value 0-255.
      if( ( (_pin == 3) | (_pin == 5) | (_pin == 6) | ( (_pin >= 9) & (_pin < 12) ) ) & ( (_pwm >= 0) & (_pwm < 256) ) ) {
        analogWrite(_pin, _pwm);
        ack();
      }
      else {
        ack_err();
      }
    }

    /* Malformed/invalid command */
    else if(cmd[0] != '\0') {
      ack_err();
    }

    /* Reset cmd array */
    for(uint16_t i = 0; i < sizeof(cmd); i++) {
        cmd[i] = '\0';
    }
}

void setup() {
  Serial.begin(BAUD);
  pinMode(PIN_CHAN1, OUTPUT);
  pinMode(PIN_CHAN2, OUTPUT);
  pinMode(PIN_CHAN3, OUTPUT);
  pinMode(PIN_CHAN4, OUTPUT);
  pinMode(PIN_CHAN5, OUTPUT);

  // Adjust PWM frequencies to above audible range. Less annoying.
  TCCR2B = TCCR2B & B11111000 | B00000001; // Pin 3 PWM freq. to 31kHz
  //TCCR0B = TCCR0B & B11111000 | B00000001; // Pin 5&6 PWM freq. to 60kHz. AFFECTS GLOBAL TIMER (delay, baud, etc.)!!!
  TCCR1B = TCCR1B & B11111000 | B00000001; // Pin 9&10 PWM freq. to 31kHz
}

void loop() {
  serial_listen(inCmd);
  cmd_interpret(inCmd);
  delay(50);
}

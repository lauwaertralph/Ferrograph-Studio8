// ############################################################################################################
// Description
// ############################################################################################################
// Arduino MEGA PORTA

// PORTA_0 = MEGA PIN 22 <-TO-> PIN 5 of IC425 - ECTR4010 (PIN 7 of IC420 SN7448N)
// PORTA_1 = MEGA PIN 23 <-TO-> PIN 4 of IC425 - ECTR4010 (PIN 1 of IC420 SN7448N)
// PORTA_2 = MEGA PIN 24 <-TO-> PIN 3 of IC425 - ECTR4010 (PIN 2 of IC420 SN7448N)
// PORTA_3 = MEGA PIN 25 <-TO-> PIN 2 of IC425 - ECTR4010 (PIN 6 of IC420 SN7448N)
// PORTA_4 = MEGA PIN 26 <-TO-> Base of decade driver transistor TR415 for Display D413
// PORTA_5 = MEGA PIN 27 <-TO-> Base of decade driver transistor TR418 for Display D414
// PORTA_6 = MEGA PIN 28 <-TO-> Base of decade driver transistor TR421 for Display D415
// PORTA_7 = MEGA PIN 29 <-TO-> Base of decade driver transistor TR426 for Display D416

// The Display/Decade selection is ACTIVE LOW
// 1110
// 1101
// 1011
// 0111

// ############################################################################################################
// Constants
// ############################################################################################################

// I/O Signal Pins
const byte IMP = 21; // PORTD_0 - Arduino Pin 21 - INT0
const byte DIR = 20; // PORTD_1 - Arduino Pin 20 - INT1
const byte RST = 19; // PORTD_2 - Arduino Pin 19 - INT2

// DEBUG Pins
const byte DBG_IMP = 2; // PORTE_4 - Arduino Pin 2

// ############################################################################################################
// Global variables
// ############################################################################################################

// LED display
//
// D413-D416 are 7-segment LED displays, model TIL302 and D412 is a
// limited-character 7-segment display (-, + and 1), model TIL304.
// They have the folowing function:
// D412 - displays 1 hour, positive or negative.
// D413 - displays minutes 10s
// D414 - displays minutes 1s
// D415 - displays seconds 10s
// D416 - displays seconds 1s
//
// The folowing byte array "display" is for storing
// the value of each of the D413-D416 displays.
//
// display[0] is for D413 - minutes 10s
// negative is for D414 - minutes 1s
// display[2] is for D415 - seconds 10s
// display[3] is for D416 - seconds 1s
//
//                { minutes   | seconds    }
//                { 10s | 1s  | 10s | 1s   }
//                { D413, D414, D415, D416 }
byte display[4] = { 0,    0,    0,    0    };

// Used for disabling the counter during reset.
bool enableCounter = true;

// Keeps track if the counter is more than one hour.
bool oneHour = false;

// Keeps track if the counter in negative.
bool negative = false;

// Keeps track of the counting direction.
bool countingDirectionForward = true;

// This controls the 7 segment display refresh rate. (microseconds)
const long displayRefreshDelay = 1000;

void setup() {
  // I/O Setup
  // LED Displays Output (PORTF0:7)
  DDRA = 0xFF;   // Set Data Direction Register for PORT A to output.
  PORTA = 0x00;  // Set PORT A output to LOW.

  // Ferrograph Studio 8 Signal I/O
  pinMode(IMP, INPUT);
  pinMode(DIR, INPUT);
	pinMode(RST, INPUT);

  // DEBUG I/O
  pinMode(DBG_IMP, OUTPUT);

  // Interrupts
  attachInterrupt(digitalPinToInterrupt(DIR), determineCountingDirection, CHANGE);
  attachInterrupt(digitalPinToInterrupt(IMP), impulseInterrupt, FALLING);

  // Determine the initial counting direction.
  determineCountingDirection();
}

// ############################################################################################################
// Main Loop
// ############################################################################################################
void loop() {
  ledDisplay();
  checkReset();

  // Debug Counter
  //        (increment/decrement, interval)
  // debugCount(true,                50000   );
  // debugImpulseInterrupt(50000);
}

// ############################################################################################################
// Functions
// ############################################################################################################
void ledDisplay() {
  unsigned long timeNow = micros();
  static unsigned long timelast = 0;
	static byte decadeDriverSelect = 0x01;  // Select the first decade driver.
  static byte displayValueSelect = 0;     // Select the first display.
	byte output = 0x00;

  if ((timelast + displayRefreshDelay) <= timeNow) {
    timelast = timeNow;
    
    // Make output = inverted(decadeDriverSelect) because the Ferrograph decade drivers are ACTIVE LOW.
    output = decadeDriverSelect ^ 0xFF;
    // output = decadeDriverSelect; // For debugging on the AL15A

    // Bitshift output left 4 positions.
    output = output << 4;

    // Bitwise OR output with the selected display value.
    output = output | display[displayValueSelect];

    // Output the result.
		PORTA = output;

    // If the selected decade driver is not the last one, move to the next decade driver and next and display.
    // Else, reset to the first decade driver and first display. 
    if (decadeDriverSelect < 0x08) {
      decadeDriverSelect = decadeDriverSelect << 1;   // Bitshift decadeDriverSelect 1 position, move to the next decade driver.
      displayValueSelect ++;                          // Increment displayValueSelect, move to the next display.
    } else {
      decadeDriverSelect = 0x01;                      // Reset to the first decade driver.
      displayValueSelect = 0;                         // Reset to the first display.
    }
  }
}

void count(bool increment) {
  if (increment){
    if (negative == true) {
      if (display[3] > 0) {
        display[3]--;
      } else if (display[2] > 0) {
        display[3] = 9;
        display[2]--;
      } else if (display[1] > 0) {
        display[3] = 0;
        display[2] = 5;
        display[1]--;
      } else if (oneHour == true) {
        if (display[0] < 0) {
          display[3] = 9;
          display[2] = 5;
          display[1] = 9;
          display[0]--;
        } else {
          display[3] = 9;
          display[2] = 5;
          display[1] = 9;
          display[0] = 5;
          oneHour = false;
        }
      } else if (display[0] > 0) {
        display[3] = 9;
        display[2] = 5;
        display[1] = 9;
        display[0]--;
      } else {
        display[3]++;
        negative = false;
      }
    } else {
      if (display[3] < 9) {
        display[3]++;
      } else if (display[2] < 5) {
        display[3] = 0;
        display[2]++;
      } else if (display[1] < 9) {
        display[3] = 0;
        display[2] = 0;
        display[1]++;
      } else if (oneHour == false) {
        if (display[0] < 5) {
          display[3] = 0;
          display[2] = 0;
          display[1] = 0;
          display[0]++;
        } else {
          display[3] = 0;
          display[2] = 0;
          display[1] = 0;
          display[0] = 0;
          oneHour = true;
        }
      } else if (display[0] < 9) {
          display[3] = 0;
          display[2] = 0;
          display[1] = 0;
          display[0]++;
      } else {
        display[3]--;
        negative = true;
      }
    }
  } else {
    if (negative == true) {
      if (display[3] < 9) {
        display[3]++;
      } else if (display[2] < 5) {
        display[3] = 0;
        display[2]++;
      } else if (display[1] < 9) {
        display[3] = 0;
        display[2] = 0;
        display[1]++;
      } else if (oneHour == false) {
        if (display[0] < 5) {
          display[3] = 0;
          display[2] = 0;
          display[1] = 0;
          display[0]++;
        } else {
          display[3] = 0;
          display[2] = 0;
          display[1] = 0;
          display[0] = 0;
          oneHour = true;
        }
      } else if (display[0] < 9) {
          display[3] = 0;
          display[2] = 0;
          display[1] = 0;
          display[0]++;
      } else {
        display[3]--;
        negative = false;
      }
    } else {
      if (display[3] > 0) {
        display[3]--;
      } else if (display[2] > 0) {
        display[3] = 9;
        display[2]--;
      } else if (display[1] > 0) {
        display[3] = 0;
        display[2] = 5;
        display[1]--;
      } else if (oneHour == true) {
        if (display[0] < 0) {
          display[3] = 9;
          display[2] = 5;
          display[1] = 9;
          display[0]--;
        } else {
          display[3] = 9;
          display[2] = 5;
          display[1] = 9;
          display[0] = 5;
          oneHour = false;
        }
      } else if (display[0] > 0) {
        display[3] = 9;
        display[2] = 5;
        display[1] = 9;
        display[0]--;
      } else {
        display[3]++;
        negative = true;
      }
    }
  }
}

void determineCountingDirection() {
  // Called when the DIR signal changes.
  // Counting direction is forward => DIR goes HIGH.
  // Counting direction is reverse => DIR goes LOW.
  
  if (digitalRead(DIR) == HIGH) {
    countingDirectionForward = true;
  }
  else {
    countingDirectionForward = false;
  }
}

void impulseInterrupt() {
  // Called when the IMP signal triggers interrupt.
  if (enableCounter) {
    if (countingDirectionForward) {
      count(true);
    } else {
      count(false);
    }
  }
}

void checkReset() {
  // If the counter reset input is HIGH, disable and reset,
  // the counter, else enable the counter.
  if (digitalRead(RST) == HIGH) {
    enableCounter = false;
    resetCounter();
  } else {
    enableCounter = true;
  }
}

void resetCounter() {
  // Resets the couter by setting the entire display array
  // to zero, the bools negative and oneHour to false.
  display[3] = 0;
  display[2] = 0;
  display[1] = 0;
  display[0] = 0;
  oneHour = false;
  negative = false;
}

// ############################################################################################################
// Debugging Functions
// ############################################################################################################

void debugCount(bool increment, long interval) {
  unsigned long timeNow = micros();
  static unsigned long timelast = 0;

  if ((timelast + interval) <= timeNow) {
    timelast = timeNow;
    count(increment);
  }
}

void debugImpulseInterrupt(long interval) {
  unsigned long timeNow = micros();
  static unsigned long timelast = 0;

  if (timelast + (interval / 2) <= timeNow) {
    timelast = timeNow;
    if (digitalRead(DBG_IMP) == LOW) {
      digitalWrite(DBG_IMP, HIGH);
    } else {
      digitalWrite(DBG_IMP, LOW);
    }
  }
}

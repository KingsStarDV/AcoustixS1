#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Rotary Encoder Pins
#define ENC_A 14   // CLK pin
#define ENC_B 15   // DT pin
#define ENC_SW 16  // Switch pin

// Output Pin for Square Wave
#define OUTPUT_PIN 13

// Frequency Variables
volatile int frequency = 50;  // Start at 50Hz
const int MIN_FREQ = 20;
const int MAX_FREQ = 100;
const int FREQ_STEP = 2;

// Timer Variables
unsigned long halfPeriodUs;
unsigned long lastToggleTime = 0;
bool outputState = false;

// Rotary Encoder Variables
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Interrupt Service Routine for Rotary Encoder
void updateEncoder() {
  int MSB = digitalRead(ENC_A);
  int LSB = digitalRead(ENC_B);
  
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;
  
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderValue++;
  }
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderValue--;
  }
  
  lastEncoded = encoded;
}

void setup() {
  // Initialize Serial Monitor (for debugging)
  Serial.begin(115200);
  
  // Initialize OLED Display
  Wire.setSDA(8);   // Default SDA pin for Pico
  Wire.setSCL(9);   // Default SCL pin for Pico
  Wire.begin();
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Initialize Rotary Encoder Pins
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  
  // Initialize Output Pin
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
  
  // Attach interrupts for rotary encoder
  attachInterrupt(digitalPinToInterrupt(ENC_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), updateEncoder, CHANGE);
  
  // Calculate initial half period
  calculateHalfPeriod();
  
  // Show initial frequency
  updateDisplay();
}

void loop() {
  // Check rotary encoder button
  checkButton();
  
  // Update frequency based on encoder value
  updateFrequency();
  
  // Generate square wave
  generateSquareWave();
}

void calculateHalfPeriod() {
  // Calculate half period in microseconds for the current frequency
  // Period (in seconds) = 1 / frequency
  // Half period (in microseconds) = (1 / (2 * frequency)) * 1,000,000
  halfPeriodUs = (1000000L / (2L * frequency));
}

void updateFrequency() {
  static long lastEncoderValue = 0;
  
  if (encoderValue != lastEncoderValue) {
    int steps = encoderValue - lastEncoderValue;
    
    if (steps > 0) {
      // Clockwise rotation
      frequency += FREQ_STEP;
    } else if (steps < 0) {
      // Counter-clockwise rotation
      frequency -= FREQ_STEP;
    }
    
    // Constrain frequency to limits
    frequency = constrain(frequency, MIN_FREQ, MAX_FREQ);
    
    // Recalculate half period
    calculateHalfPeriod();
    
    // Update display
    updateDisplay();
    
    // Update last encoder value
    lastEncoderValue = encoderValue;
    
    // Debug output
    Serial.print("Frequency: ");
    Serial.print(frequency);
    Serial.println(" Hz");
  }
}

void checkButton() {
  bool reading = digitalRead(ENC_SW);
  
  // Check for button press with debouncing
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
      // Button pressed - reset to 50Hz
      frequency = 50;
      encoderValue = 0;
      calculateHalfPeriod();
      updateDisplay();
      
      Serial.println("Button pressed - Reset to 50Hz");
      
      // Wait for button release
      delay(300);
    }
  }
  
  lastButtonState = reading;
}

void generateSquareWave() {
  unsigned long currentTime = micros();
  
  // Check if half period has elapsed
  if (currentTime - lastToggleTime >= halfPeriodUs) {
    outputState = !outputState;
    digitalWrite(OUTPUT_PIN, outputState);
    lastToggleTime = currentTime;
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Display title
  display.setTextSize(2);
  display.setCursor(20, 0);
  display.print("SQUARE");
  display.setCursor(35, 18);
  display.print("WAVE");
  
  // Display frequency
  display.setTextSize(3);
  display.setCursor(25, 35);
  display.print(frequency);
  display.setTextSize(2);
  display.print(" Hz");
  
  // Display limits
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("MIN:20Hz");
  display.setCursor(SCREEN_WIDTH - 50, 56);
  display.print("MAX:100");
  
  display.display();
}

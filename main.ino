#include <Adafruit_NeoPixel.h>

#define PIXEL_DAT 5
#define PIXEL_NUM 8
#define RED       200,0,0
#define ORANGE    255,69,0
#define YELLOW    255,234,0
#define GREEN     124,252,0
#define BLUE      0,191,255
#define INDIGO    75,0,130
#define VIOLET    255,0,255
#define MODE_KEY  2    
#define COLOR_KEY 3
#define BRIGHTNESS_POT A2

#define lengthof(a) (sizeof(a) / sizeof(*(a)))

void heartbeat();
void spin();
void randomGlow();
void brightnessSetter();

int colors[7][3] = {{RED}, {ORANGE}, {YELLOW}, {GREEN}, {BLUE}, {INDIGO}, {VIOLET}};
typedef void (*fp)(void);
fp modes[] = {heartbeat, spin, randomGlow, brightnessSetter};

int currentColor = 0;
int currentMode = 1;
int gotInterrupt = 0;

void interruptDelay(int amount) {
  int delayResolution = 10;
  int delayLength = round(amount / delayResolution);
  for(int i=0; i < delayResolution; i++) {
    if (gotInterrupt == 1) {
      Serial.println("stop delay!");
      break;
    }
    delay(delayLength);
  }
}

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXEL_NUM, PIXEL_DAT, NEO_GRB + NEO_KHZ800);

void setup() {  
  randomSeed(analogRead(0));
  analogReference(EXTERNAL);
  pinMode(COLOR_KEY, INPUT_PULLUP);
  pinMode(MODE_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COLOR_KEY), inc_color_irq, RISING);
  attachInterrupt(digitalPinToInterrupt(MODE_KEY), inc_mode_irq, RISING);
  pixels.begin();
  Serial.begin(9600);
  pixels.setBrightness(64);
}

void brightnessSetter() {
  for (int i=0; i<PIXEL_NUM; i++) {
    pixels.setPixelColor(i, 255,255,255);
  }
  pixels.show();
}

void randomGlow() {
  int colorVelocity = 2 + (3 * currentColor);
  Serial.println(colorVelocity);
  rainbowCycle(colorVelocity);
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    if (gotInterrupt == 1) { goto finish; }
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    if (wait < 10) {
      delay(wait);
    } else {
      interruptDelay(wait);    
    }
  }
  finish:
    return;
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void spin() {
  int *c = getCurrentColor();
  for (int i=0; i<PIXEL_NUM; i++) {
    if (gotInterrupt == 1) { goto finish; }
    colorFadeIndividual(i, c[0], c[1], c[2], 1);
  }
  for (int i=0; i<PIXEL_NUM; i++) {
    if (gotInterrupt == 1) { goto finish; }
    colorFadeIndividual(i, 0, 0, 0, 1);
  }
  finish:
    return;
}

void heartbeat() {
  //beat twice
  int *c = getCurrentColor();
  for (int i=0; i<2; i++) {
    if (gotInterrupt == 1) { goto finish; }
    colorFadeAll(c[0], c[1], c[2], 3);
    interruptDelay(150);
    colorFadeAll(0,0,0, 2);
    interruptDelay(150);
    colorFadeAll(c[0], c[1], c[2], 3);
  }
  interruptDelay(1200);
  finish:
    return;
}

void colorFadeAll(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
    uint8_t curr_r, curr_g, curr_b;
    uint32_t curr_col = pixels.getPixelColor(0); // get the current colour
    curr_b = curr_col & 0xFF; curr_g = (curr_col >> 8) & 0xFF; curr_r = (curr_col >> 16) & 0xFF;  // separate into RGB components

    while ((curr_r != r) || (curr_g != g) || (curr_b != b)){  // while the curr color is not yet the target color
      if (curr_r < r) curr_r++; else if (curr_r > r) curr_r--;  // increment or decrement the old color values
      if (curr_g < g) curr_g++; else if (curr_g > g) curr_g--;
      if (curr_b < b) curr_b++; else if (curr_b > b) curr_b--;
      for(uint16_t i = 0; i < pixels.numPixels(); i++) {
        pixels.setPixelColor(i, curr_r, curr_g, curr_b);  // set the color
      }
      delay(wait);
      pixels.show();
    }
}

void colorFadeIndividual(int i, uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
    uint8_t curr_r, curr_g, curr_b;
    uint32_t curr_col = pixels.getPixelColor(i); // get the current colour
    curr_b = curr_col & 0xFF; curr_g = (curr_col >> 8) & 0xFF; curr_r = (curr_col >> 16) & 0xFF;  // separate into RGB components
    while ((curr_r != r) || (curr_g != g) || (curr_b != b)){  // while the curr color is not yet the target color
      if (curr_r < r) curr_r++; else if (curr_r > r) curr_r--;  // increment or decrement the old color values
      if (curr_g < g) curr_g++; else if (curr_g > g) curr_g--;
      if (curr_b < b) curr_b++; else if (curr_b > b) curr_b--;
      pixels.setPixelColor(i, curr_r, curr_g, curr_b);  // set the color
      delay(wait);
      pixels.show();
    }
}

int debounce(int pin) {
  Serial.print("Got interrupt");
  if (digitalRead(pin) == HIGH) {
    delay(100);
    if (digitalRead(pin) == HIGH) {
      return 1;
    }    
  }
  return 0;
}

void inc_color_irq() {
  if (debounce(COLOR_KEY) == 0) { return; }
  gotInterrupt = 1;
  currentColor = (currentColor + 1) % lengthof(colors);
  Serial.print("currentColor: ");
  Serial.print(currentColor);
  Serial.print("\n");
}

void inc_mode_irq() {
  if (debounce(MODE_KEY) == 0) { return; }
  gotInterrupt = 1;
  currentMode = (currentMode + 1) % lengthof(modes);
  Serial.print("currentMode: ");
  Serial.print(currentMode);
  Serial.print("\n");
}

int *getCurrentColor() {
  return colors[currentColor];
}

int *generateRandomRgb() {
   static int rgb[3];
   for (int i=0; i<3; i++) {
      rgb[i] = random(0, 255);
   }
   return rgb;
}

void setBrightnessFromKnob() {
  int knobVoltage = analogRead(A2);
  Serial.println(knobVoltage);
  float voltage = knobVoltage * (5.0 / 1023.0);
  Serial.println(voltage);
  int brightnessLevel = round(voltage * (255/5));
  pixels.setBrightness(brightnessLevel); 
}

void loop() {
  setBrightnessFromKnob();
  modes[currentMode]();
  gotInterrupt = 0;
}

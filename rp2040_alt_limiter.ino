// F5L altitude limiter by Gwen Scogin - 2026
#include <Servo.h> 

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP5xx.h"

#include <Adafruit_NeoPixel.h>

// How many internal neopixels do we have? some boards have more than one!
#define NUMPIXELS        1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);


#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP5xx bmp; // Create BMP5xx object
bmp5xx_powermode_t desiredMode = BMP5XX_POWERMODE_NORMAL; // Cache desired power mode

Servo t_out;

#define DELAY_TIME 20  // this is the 20ms between servo pulses from the receiver
#define ALT_HISTORY_SIZE 25
#define OVERSHOOT_FACTOR 1.2

#define READY_COLOR (0x00f030)
#define ARMED_COLOR (0x900090)
#define DONE_COLOR (0x909020)
#define FOUL_COLOR (0xf00000)

int color = READY_COLOR;
int armed = 0;
int can_arm = 1;
unsigned long timer = 30000;
int altitude=50;
int altitude_list[] = {80,100,150};
int timer_list[] = {15000,30000,30000};
int base_altitude=0;
unsigned long base_timer = 0;
unsigned long loop_counter = 0;
int arm_count = 0;
// We are maintaining a history of the past readings of altitude.  When 
// we compute the vertical speed (vspd) we will look further back in time
// than just the previous reading. This is because loop runs fast enough
// that there might not be a detectable change in altitude.  
float previous_altitude_arr[ALT_HISTORY_SIZE];
int pa_idx = 0; 
// there is no reason to do this every time through the loop
float vspd_correction = 1000.0 / (DELAY_TIME * ALT_HISTORY_SIZE);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
 //  while (!Serial) delay(10); 

  pinMode(2,INPUT);
  pinMode(11,INPUT);
  pinMode(4,OUTPUT);
  // attach() starts sending pulses right away.  Without the last argument
  // it sends 1500µs (half throttle) until the first write in loop().
  t_out.attach(4, 1000, 2000, 1000);

  if (!bmp.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire)) {
  // For SPI mode (uncomment the line below and comment out the I2C line above):
  // if (!bmp.begin(BMP5XX_CS_PIN, &SPI)) {
    Serial.println(F("Could not find a valid BMP5xx sensor, check wiring!"));
    // The throttle output stays at 1000µs.  Show a solid red led so the
    // failure is visible without a serial connection.
    pixels.begin();
    pixels.setBrightness(20);
    pixels.fill(FOUL_COLOR);
    pixels.show();
    while (1) delay(10);
  }
  
  Serial.println(F("Setting temperature oversampling to 2X..."));
  bmp.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
Serial.println(F("Setting pressure oversampling to 16X..."));
  bmp.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
  Serial.println(F("Setting IIR filter to coefficient 3..."));
  bmp.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
  Serial.println(F("Setting output data rate to 50 Hz..."));
  bmp.setOutputDataRate(BMP5XX_ODR_50_HZ);
  Serial.println(F("Setting power mode to normal..."));
  desiredMode = BMP5XX_POWERMODE_NORMAL;
  bmp.setPowerMode(desiredMode);
  Serial.println(F("Enabling pressure measurement..."));
  bmp.enablePressure(true);
  Serial.println(F("Configuring interrupt pin with data ready source..."));
  bmp.configureInterrupt(BMP5XX_INTERRUPT_LATCHED, BMP5XX_INTERRUPT_ACTIVE_HIGH, BMP5XX_INTERRUPT_PUSH_PULL, BMP5XX_INTERRUPT_DATA_READY, true);

  
  
//  bmp.setMetricSystem(InternationalSystem());
  bmp.readTemperature(); // Without the readTemperature call we get bad values for altitude
  float cur_altitude = bmp.readAltitude();
  for(int i = 0 ; i < ALT_HISTORY_SIZE ; i++) previous_altitude_arr[i]= cur_altitude;


#if defined(NEOPIXEL_POWER)
  // If this board has a power control pin, we must set it to output and high
  // in order to enable the NeoPixels. We put this in an #if defined so it can
  // be reused for other boards without compilation errors
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(20); // not so#if defined(NEOPIXEL_POWER)
  // If this board has a power control pin, we must set it to output and high
  // in order to ena bright


  Serial.print("setup done \n");
}


// states
// 0x001  80m ready
// 0x002 100m ready
// 0x004 150m ready
// 0x008  80m alt
// 0x010 100m alt
// 0x020 150m alt
// 0x040  80m time
// 0x080 100m time
// 0x100 150m time
// 0x200 foul


int patterns[] = {
  0b1111111111,
  0b1111111000,
  0b1111111110,
  0b1000000000,
  0b1111000100,
  0b1111000000,
  0b1111000000,
  0b1000000000,
  0b1111111111,
  0b1000000000,
  0b1110110110,
  0b1000000000,
  0b1100100100,
  0b1000000000,
  0b1000000000,
  0b1000000000
};
  
int alt_state = 2;


void blink(uint32_t ms) {
  uint32_t phase = (ms >> 8) & 15;
  int light = (patterns[phase] >> alt_state) & 1;  
  if (light) {
    pixels.fill(color);
  pixels.show();
  } else {
    pixels.fill(0x000000);
  pixels.show();
  }
}

int button_state = 0;
int read_button(int pin) {
  int r = digitalRead(pin);
  if (r) {
    if (button_state == 0) {
      button_state = 1;
      return 1;
    } else {
      return 0;
    }
  } else {
    button_state = 0;
    return 0;
  }
}

void loop() {
  int out_value = 1000; // by default we are going to output 1000µs 
  int in_value = pulseIn(2,HIGH,50000);
  bmp.readTemperature();  // needed to get accurate readings
  float cur_altitude = bmp.readAltitude();
  uint32_t now = millis();
  blink(now);
  if (read_button(11) && (alt_state < 3)) {
    alt_state = (alt_state + 1) % 3;
    altitude = altitude_list[alt_state];
    timer = timer_list[alt_state];
    Serial.print("alt ");
    Serial.print(altitude);
    Serial.print(" timer ");
    Serial.println(timer);
  }
  float vspd = (cur_altitude - previous_altitude_arr[pa_idx]) * vspd_correction;
  if (armed) {
    // disarm if above target alt  
    if (cur_altitude + (vspd * OVERSHOOT_FACTOR)> base_altitude + altitude) {
       armed = 0;
       alt_state += 3;
       color = DONE_COLOR;
    } 
    // disarm if after time
    if (now > base_timer + timer) { 
      armed = 0;
       alt_state += 6;
       color = DONE_COLOR;
    }
  } else {
    // arm if you can arm and the throttle is above 20%
    if (can_arm && (in_value > 1200)) {
      if(arm_count == 0) base_altitude = cur_altitude;
      Serial.println("starting arm");
      base_timer = now;
      armed = 1;
      color = ARMED_COLOR;
      can_arm = 0;
      arm_count++;
      if (arm_count > 1) {
        color = FOUL_COLOR;
        alt_state = 9;
      }
    }
    // If we are not armed and cant arm  and we are no more than 10m high re enable arming
    if (!can_arm && !armed && 
    ((cur_altitude < base_altitude + 10) || (now > base_timer + timer + timer)) && 
    (in_value < 1150)) {
      can_arm = 1;
    }
  }
  if (armed) {
    out_value = in_value;
  }
  t_out.writeMicroseconds(out_value);
 // Serial.print("Temperature: ");
 //   Serial.println(bmp.readTemperature());
 //  Serial.print("Pressure: ");
 //   Serial.println(bmp.readPressure());
 //   Serial.print("Altitude: ");
 //  Serial.println(cur_altitude);
 previous_altitude_arr[pa_idx] = cur_altitude;
  pa_idx = (pa_idx + 1)/ALT_HISTORY_SIZE;
  loop_counter++;
  // removing the delay because pulseIn function will block until 
  // it gets a pulse.  This will sync us to the 50hz of the receiver
  //delay(DELAY_TIME);
}

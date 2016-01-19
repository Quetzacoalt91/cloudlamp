/* 
Lighting Cloud Mood Lamp By James Bruce
View the full tutorial and build guide at http://www.makeuseof.com/

Sound sampling code originally by Adafruit Industries.  Distributed under the BSD license.
This paragraph must be included in any redistribution.
*/

#include <Wire.h>
#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 150
#define DATA_PIN 6


// Mode enumeration - if you want to add additional party or colour modes, add them here; you'll need to map some IR codes to them later; 
// and add the modes into the main switch loop
enum Mode { CLOUD,ACID,OFF,ON,RED,GREEN,BLUE,
            CUSTOM1,CUSTOM2,CUSTOM3,CUSTOM4,CUSTOM5,CUSTOM6,CUSTOM7,CUSTOM8,CUSTOM9,
            CUSTOM10,CUSTOM11,CUSTOM12,CUSTOM13,CUSTOM14,CUSTOM15,CUSTOM16,FADE};
Mode mode = FADE;  
Mode lastMode = FADE;

// Mic settings, shouldn't need to adjust these. 
#define MIC_PIN   0  // Microphone is attached to this analog pin
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   10  // Length of buffer for dynamic level adjustment
byte
  volCount  = 0;      // Frame counter for storing past volume data
int
  vol[SAMPLES];       // Collection of prior volume samples
int      n, total = 30;
float average = 0;

int brightness = 255;

bool quick_transition = false;
  
// used to make basic mood lamp colour fading feature
int fade_h;
int fade_direction = 1;


// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
  // this line sets the LED strip type - refer fastLED documeantion for more details https://github.com/FastLED/FastLED
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  // starts the audio samples array at volume 15. 
  memset(vol, 15, sizeof(vol));
  Serial.begin(115200);
  Wire.begin(9);                // Start I2C Bus as a Slave (Device Number 9)
  Wire.onReceive(receiveEvent); // register event
}



void receiveEvent(int bytes) {
  
  // Here, we set the mode based on the IR signal received. Check the debug log when you press a button on your remote, and 
  // add the hex code here (you need 0x prior to each command to indicate it's a hex value)
  while(Wire.available())
   { 
      unsigned int received = Wire.read();
      Serial.print("Receiving IR hex: ");
      Serial.println(received,HEX);
      lastMode = mode;
      switch(received){
        case 0x7D:
          mode = ON; brightness=255; break;
        case 0xFD:
          mode = OFF; break;
        case 0x2F:
          mode = CLOUD; break;
        case 0xF:
          mode = ACID; break;
        case 0x37:
          mode = FADE; break;
        case 0xC5:
          brightness += 10;break;
        case 0x45:
          brightness -= 10;break;
        case 0x5D:
          mode = BLUE; break;
        case 0x65:
          mode = GREEN; break;
        case 0xE5:
          mode = RED; break;
        case 0xD5:
          mode = CUSTOM1; break;
        case 0x55:
          mode = CUSTOM2; break;
        case 0x6D:
          mode = CUSTOM3; break;
        case 0xED:
          mode = CUSTOM4; break;
        case 0xF5:
          mode = CUSTOM5; break;
        case 0x75:
          mode = CUSTOM6; break;
        case 0x4D:
          mode = CUSTOM7; break;
        case 0xCD:
          mode = CUSTOM8; break;
        case 0xC7:
          mode = CUSTOM9; break;
        case 0x47:
          mode = CUSTOM10; break;
        case 0x87:
          mode = CUSTOM11; break;
        case 0x7:
          mode = CUSTOM12; break;
        case 0xE7:
          mode = CUSTOM13; break;
        case 0x67:
          mode = CUSTOM14; break;
        case 0xA7:
          mode = CUSTOM15; break;
        case 0x27:
          mode = CUSTOM16; break;
      }
   }

}
 
void loop() { 
  
  // Maps mode names to code functions. 
  switch(mode){
    case CLOUD: detect_thunder();reset();break;
    case ACID: acid_cloud();reset();break;
    case OFF:reset();break;
    case ON: mode = lastMode;break;
    case RED: single_colour(0);break;
    case BLUE: single_colour(160);break;
    case GREEN: single_colour(96);break;
    case CUSTOM1: single_colour(12);break;
    case CUSTOM2: single_colour(128);break;
    case CUSTOM3: single_colour(150);break;
    case CUSTOM4: single_colour(224);break;
    case CUSTOM5: single_colour(25);break;
    case CUSTOM6: single_colour(130);break;
    case CUSTOM7: single_colour(192);break;
    case CUSTOM8: single_colour(224);break;
    case CUSTOM9: single_colour(38);break;
    case CUSTOM10: single_colour(110);break;
    case CUSTOM11: single_colour(203);break;
    case CUSTOM12: single_colour(140);break;
    case CUSTOM13: single_colour(51);break;
    case CUSTOM14: single_colour(100);break;
    case CUSTOM15: single_colour(214);break;
    case CUSTOM16: single_colour(140);break;
    case FADE: colour_fade();break;
    default: detect_thunder(); reset();break; 
  }
  
}

// Makes all the LEDs a single colour, see https://raw.githubusercontent.com/FastLED/FastLED/gh-pages/images/HSV-rainbow-with-desc.jpg for H values
void single_colour(int H){
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = CHSV( H, 255, brightness);
  }
  //avoid flickr which occurs when FastLED.show() is called - only call if the colour changes
  if(lastMode != mode){
    FastLED.show(); 
    lastMode = mode;
  }
  delay(50);
}

void colour_fade(){
  //mood mood lamp that cycles through colours
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = CHSV( fade_h, 255, brightness);
  }
  if(fade_h >254){
    fade_direction = -1; //reverse once we get to 254
  }
  else if(fade_h < 0){
    fade_direction = 1;
  }
    
  fade_h += fade_direction;
  FastLED.show();
  delay(100);
}



void detect_thunder() {
  
  n   = analogRead(MIC_PIN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  vol[volCount] = n;                      // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  total = 0;
  for(int i=0; i<SAMPLES; i++) {
    total += vol[i];
  }
  
  // If you're having trouble getting the cloud to trigger, uncomment this block to output a ton of debug on current averages. 
  // Note that this WILL slow down the program and make it less sensitive due to lower sample rate.
  
  /*
  for(int t=0; t<SAMPLES; t++) {
    //initial data is zero. to avoid initial burst, we ignore zero and just add the current l
    Serial.print("Sample item ");
    Serial.print(t);
    Serial.print(":");
    Serial.println(vol[t]);
  }
  Serial.print("total");
  Serial.println(total);
  Serial.print("divided by sample size of ");
  Serial.println(SAMPLES);
  
 
  Serial.print("average:");
  Serial.println(average);

  Serial.print("current:");
  Serial.println(n);
  */
  
  average = (total/SAMPLES)+2;
  if(n>average){
    Serial.println("TRIGGERED");
    reset();
     
   
    //I've programmed 3 types of lightning. Each cycle, we pick a random one. 
    switch(random(1,3)){
       //switch(3){
  
      case 1:
        thunderburst();
        delay(random(10,500));
         Serial.println("Thunderburst");
        break;
       
      case 2:
        rolling();
        Serial.println("Rolling");
        break;
        
      case 3:
        crack();
        delay(random(50,250));
        Serial.println("Crack");
        break;
        
      
    }
  }
}
 
 
// utility function to turn all the lights off.  
void reset(){
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = CHSV( 0, 0, 0);
  }
  FastLED.show();
   
}

void acid_cloud(){
    // a modification of the rolling lightning which adds random colour. trippy. 
    //iterate through every LED
    for(int i=0;i<NUM_LEDS;i++){
      if(random(0,100)>90){
        leds[i] = CHSV( random(0,255), 255, brightness); 

      }
      else{
        leds[i] = CHSV(0,0,0);
      }
    }
    FastLED.show();
    delay(random(5,100));
    reset();
    
  //}
}

void rolling(){
  // a simple method where we go through every LED with 1/10 chance
  // of being turned on, up to 10 times, with a random delay wbetween each time
  for(int r=0;r<random(2,10);r++){
    //iterate through every LED
    for(int i=0;i<NUM_LEDS;i++){
      if(random(0,100)>90){
        leds[i] = CHSV( 0, 0, brightness); 

      }
      else{
        //dont need reset as we're blacking out other LEDs her 
        leds[i] = CHSV(0,0,0);
      }
    }
    FastLED.show();
    delay(random(5,100));
    reset();
    
  }
}

void crack(){
   //turn everything white briefly
   for(int i=0;i<NUM_LEDS;i++) {
      leds[i] = CHSV( 0, 0, brightness);  
   }
   FastLED.show();
   delay(random(10,100));
   reset();
}

void thunderburst(){

  // this thunder works by lighting two random lengths
  // of the strand from 10-20 pixels. 
  int rs1 = random(0,NUM_LEDS/2);
  int rl1 = random(10,20);
  int rs2 = random(rs1+rl1,NUM_LEDS);
  int rl2 = random(10,20);
  
  //repeat this chosen strands a few times, adds a bit of realism
  for(int r = 0;r<random(3,6);r++){
    
    for(int i=0;i< rl1; i++){
      leds[i+rs1] = CHSV( 0, 0, brightness);
    }
    
    if(rs2+rl2 < NUM_LEDS){
      for(int i=0;i< rl2; i++){
        leds[i+rs2] = CHSV( 0, 0, brightness);
      }
    }
    
    FastLED.show();
    //stay illuminated for a set time
    delay(random(10,50));
    
    reset();
    delay(random(10,50));
  }
  
}

// basically just a debug mode to show off the lightning in all its glory, no sound reactivity. 
void constant_lightning(){
  switch(random(1,10)){
   case 1:
        thunderburst();
        delay(random(10,500));
         Serial.println("Thunderburst");
        break;
       
      case 2:
        rolling();
        Serial.println("Rolling");
        break;
        
      case 3:
        crack();
        delay(random(50,250));
        Serial.println("Crack");
        break;
        
    
  }  
}
  


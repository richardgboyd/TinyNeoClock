#include <USI_TWI_Master.h>
#include <TinyWireM.h>
#include <TinyRTClib.h>
#include <Adafruit_NeoPixel.h>

#define PIN 4
#define SDA 0  // Provided for refrence
#define SCL 2  // Provided for refrence

Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 RTC;// = RTC_DS1307();
uint8_t RTC_isRunning;

uint32_t milli_color  = strip.Color (  2,  0,  0);
uint32_t milli_color1 = strip.Color (  4,  0,  0);
uint32_t milli_color2 = strip.Color (  8,  0,  0);

uint32_t off_color    = strip.Color (  0,  0,  0);
uint32_t second_color = strip.Color ( 15,  0,  0);
uint32_t minute_color[] =
{
   strip.Color (  0,  0, 20),
   strip.Color (  0, 20,  0)
};

/* TODO 0.0 to 1.0 percent between current and next value. for color fading */
/* or event based lerping? */

/* TODO gamut values from goggles */
/* TODO smooth sub pixel rendering of bar/pixel ends (careful to avoid 50% average brightness in middle for a pixel */


/* CLOCK */
class ClockPositions
{
public:
   uint32_t cp_milli;
   uint8_t  cp_second;
   uint8_t  cp_minute;
   uint8_t  cp_hour;

   ClockPositions ();
   void readTime ();
   boolean update ();
};


ClockPositions::ClockPositions()
{
   cp_milli = 0;
   cp_second = 0;
   cp_minute = 0;
   cp_hour = 0;
}

void ClockPositions::readTime()
{     
   DateTime myTime = RTC.now();
   
   cp_second = myTime.second();
   cp_minute = myTime.minute();
   cp_hour = myTime.hour();
}

boolean ClockPositions::update()
{
   if (!RTC_isRunning)
      return false;
      
   if (millis() % 1000 != cp_milli)
   {   
      
      cp_milli = millis() % 1000;
      
      if ((cp_milli % 1000) == 0)
         readTime();
      
      return true;
   }
   
   return false;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

/* CLOCK VIEW */

class ClockSegments
{
public:
   ClockPositions    &positions;
   Adafruit_NeoPixel &strip;
   uint32_t last_milli;
   uint8_t  last_second;

   ClockSegments (Adafruit_NeoPixel&, ClockPositions&);
   void draw  ();
   void clear ();
   void set_color (uint8_t position, uint32_t color);
   void add_color (uint8_t position, uint32_t color);
   uint32_t blend (uint32_t color1, uint32_t color2);
};

const int clock_pos[12][2] =
{
   {  0, 15 }, { 14, 14 }, { 13, 13 }, 
   { 12, 11 }, { 10, 10 }, {  9,  9 },
   {  8,  7 }, {  6,  6 }, {  5,  5 }, 
   {  4,  3 }, {  2,  2 }, {  1,  1 }
};

ClockSegments::ClockSegments(
   Adafruit_NeoPixel& n_strip, 
   ClockPositions& n_positions): strip (n_strip), positions (n_positions)
{
   last_milli = 0;
   last_second = 0;
}


void ClockSegments::draw()
{ 
   uint32_t local_milli;
   uint8_t sec0, sec1;
   uint8_t min0, min1;
   uint8_t hour0, hour1;
   uint32_t hour_color;
   
   local_milli = map(positions.cp_milli, 0, 1000, 0, 32);
   if (local_milli != last_milli)
   {
      last_milli = local_milli;
      
      clear();
  
      sec0 = clock_pos[positions.cp_second / 5][0];
      sec1 = clock_pos[positions.cp_second / 5][1];
   
      min0 = clock_pos[positions.cp_minute / 5][0];
      min1 = clock_pos[positions.cp_minute / 5][1];
   
      hour0 = clock_pos[positions.cp_hour % 12][0];
      hour1 = clock_pos[positions.cp_hour % 12][1];

      add_color (sec0, second_color);
      add_color (sec1, second_color);
     
      add_color (min0, minute_color[positions.cp_minute % 2]);
      add_color (min1, minute_color[positions.cp_minute % 2]);
   
      add_color (15 - (local_milli/2     % 16),  milli_color);
      add_color (15 - ((local_milli/2+1) % 16),  milli_color1);
      add_color (15 - ((local_milli/2+2) % 16),  milli_color2);

      // Set hour color last since it is not a blend of the other colors     
      hour_color = Wheel(map(positions.cp_second * 32 + local_milli, 0, 1920, 0, 256));
      set_color (hour0,  hour_color);
      set_color (hour1,  hour_color);

      strip.show ();
   }
}


void ClockSegments::add_color (uint8_t position, uint32_t color)
{
   uint32_t blended_color = blend (strip.getPixelColor (position), color);

   /* Gamma mapping */
   uint8_t r,b,g;

   r = (uint8_t)(blended_color >> 16),
   g = (uint8_t)(blended_color >>  8),
   b = (uint8_t)(blended_color >>  0);

   strip.setPixelColor (position, blended_color);
}

void ClockSegments::set_color (uint8_t position, uint32_t color)
{
   strip.setPixelColor (position, color);
}

uint32_t ClockSegments::blend (uint32_t color1, uint32_t color2)
{
   uint8_t r1,g1,b1;
   uint8_t r2,g2,b2;
   uint8_t r3,g3,b3;

   r1 = (uint8_t)(color1 >> 16),
   g1 = (uint8_t)(color1 >>  8),
   b1 = (uint8_t)(color1 >>  0);

   r2 = (uint8_t)(color2 >> 16),
   g2 = (uint8_t)(color2 >>  8),
   b2 = (uint8_t)(color2 >>  0);

   return strip.Color(
            constrain (r1+r2, 0, 255), 
            constrain (g1+g2, 0, 255), 
            constrain (b1+b2, 0, 255));
}


void ClockSegments::clear ()
{
   for(uint16_t i=0; i<strip.numPixels (); i++) {
      strip.setPixelColor (i, off_color);
   }
}


/* APP */
ClockPositions positions;
ClockSegments  segments(strip, positions);

void setup(void)
{
    TinyWireM.begin();
    RTC.begin();
    RTC_isRunning = RTC.isrunning();
    if (RTC_isRunning) {
      positions.readTime();
    }
    strip.begin();
    strip.show();
}

void loop(void)
{
   if (positions.update())
   {
       segments.draw();
   }
}





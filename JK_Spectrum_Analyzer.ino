
// This code was implemented from a codebender user by the name of worynim.
//(Link:https://codebender.cc/sketch:76819#16%20Band%20led%20spectrum%20analyzer.ino) 
// The code was adjusted to fit the project's requirements of a 10 by 10 LED matrix
// Other features such as different matrix colors were added.

//If changing Matrix size, use the find feature (control+f) to search for the key word matrix7

// 16 Band led spectrum analyzer
// neopixel led strip 16 * 16 + hc-06 + simple mic amp
// code by worynim@gmail.com

unsigned char brightness = 50;
unsigned char line_color = 0;    // 0 : rainbow
unsigned char dot_dir =  0;      // 1:up,    0:down
unsigned char dot_color = 0  ;   // rainbow, This is chosen further down the code, feel free to change it to what you like
unsigned char line_dir = 0 ;     // 1: up    0: down
unsigned char random_flag = 0;
unsigned char dot_on = 1; //1:on 0:off
unsigned char axis = 10; //DO NOT CHANGE!!!
#include <Adafruit_NeoPixel.h>
#define PIN 7
Adafruit_NeoPixel strip = Adafruit_NeoPixel(100, PIN, NEO_GRB);    //Remember to add leds if you are using more than 100.


#define LOG_OUT 1 // use the log output function
#define FFT_N 256 // set to 256 point fft


#include <math.h>
#include <FFT.h> // include the library

signed int sel_freq[10] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20}; //matrix7 If you change your matrix size, lets say a 3x10, change the number to 4, and make sure you have a range of 0-20 in here, and only 4 numbers in the array.

signed int freq_offset[10] = {
  50, 30, 25, 25, 25, 25, 20, 20, 20, 20  // Edit this if there are false lights coming on. I recommend starting at all 15s and increrase if lights dont go off when source is turned all the way down.
};                                        //matrix7 change this to your actual matrix size, do not add 1, and adjust the offset for the new peramiters.
unsigned char freq_div[10] = {
  15, 17, 17, 17, 17, 17, 17, 17, 17, 16 //if it keeps peaking the top led, then add 1 to the specific row.
};                                        //matrix7 do the same thing as the freq_offset.

unsigned char band[10] = {  //matrix7 change this to the matrix width
  10
};
unsigned char display_band[10] = {      //matrix7 change this to the matrix height.
  10
};
unsigned char dot_band[10] = {        //matrix7 change this to your width of the matrix.
  0
};


void setup() {
  Serial.begin(9600); // use the serial port
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0b01000111; // use adc7
  DIDR0 = 0x01; // turn off the digital input for adc0
  
  analogReference(EXTERNAL); //set aref to external
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  strip.setBrightness(brightness);
}

void loop() {

  while (1) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
      while (!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i + 1] = 0; // set odd bins to 0

      
    }
    
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();

    //Serial.write(255); // send a start byte
    //Serial.write(fft_log_out, 10); // send out the data
    
   
    volatile static unsigned char line_delay_count = 0;
    volatile static signed char max_dot_count = 0;

    if (++line_delay_count >= 2) { ///  2
      line_delay_count = 0;
      for (int ii = 0; ii < 11; ii++)if ( display_band[ii] > 0 )  display_band[ii] -= 1;
    }

    //     line_delay_count=100;
    //  for(int ii=0; ii<10; ii++)if( display_band[ii] > 0 )  display_band[ii] -=1;    //These three lines allow you to have a custom refresh speed so, only mess with the line delay. I just keep it out.

    // for(int ii=0; ii<10; ii++) display_band[ii] =0;


    if (++max_dot_count >= 10 ) { ////8      //matrix7 this you have to do some thinking, but if you are keeping a 10 height, then change this lines max_dot_count to whatever your width is.
      max_dot_count = 0;
      for (int ii = 0; ii < 10; ii++)
      {
        if (dot_dir) {
          if ( dot_band[ii] > 0 )dot_band[ii] -= 1;
        }        /// up
        else {
          if ( dot_band[ii] > 0 )dot_band[ii] -= 1;
        }       /// down
      }
    }

    static unsigned char color_offset = 0;
    color_offset++;

    if (dot_on == 1)axis = 10;                      //matrix7 change axis to your height, and the line under to your height+1.
    else if (dot_on == 0)axis = 11;
    for (int ii = 0; ii < axis; ii++) //X-axis height
    {
      int fft_value = 0;
      long fft_sum = 0;
      // ?????? ???????????? ?????? ??????
      unsigned char sum_count = sel_freq[ii + 1] - sel_freq[ii];
      for (int z = sum_count - 1 ;  z > 0; z--) //  2,4,7
      {
        fft_sum += fft_log_out[ sel_freq[ii] + z ];
      }
      fft_value = fft_sum / sum_count - freq_offset[ii];

      fft_value = fft_log_out[ sel_freq[ii] ] - freq_offset[ii];     // freq select

      if ( fft_value < 0 ) fft_value = 0;

      band[ii] = fft_value / freq_div[ii];                  // current value update  range 0~15
      if (band[ii] > axis) band[ii] = 0;

      if ( display_band[ii] < band[ii] )  display_band[ii] = band[ii];   // line update

      for (int jj = 0; jj < axis; jj++) //Y Axis Height
      {
        int address = 0;
        // line direction LED 16ea
        if (dot_on == 0) {
          
          //0 + k + 10 * i
          //if ( (ii % 2) == (line_dir)) address = 0 + jj - 1 + (10 * ii);
          
          if ( (ii % 2) == (line_dir)) address = 0 + jj + (10 * ii);
          else address = jj - 1 + (10 * ii);
        }
        if (dot_on == 1) {

          //19 - k + 10 * (i - 1)
          //if ( (ii % 2) == (line_dir)) address = 0 + jj + (10 * ii);

          if ( (ii % 2) == (line_dir)) address = 19 - jj + (10 * (ii - 1));
          else address = jj + (10 * ii);
        }

        // dot color line color
        if ((dot_band[ii] == jj) && (dot_on == 1))
        {
          if (dot_color == 0)strip.setPixelColor(address, Wheel(9 + jj + (10 * ii) + color_offset)  ); // dot color Rainbow Moving
          else if (dot_color == 1)strip.setPixelColor(address, 0xFF0000); // dot color Red
          else if (dot_color == 2)strip.setPixelColor(address, 0xFFFF00); // dot color Yellow
          else if (dot_color == 3)strip.setPixelColor(address, 0x00FF00); // dot color Lime Green    
          else if (dot_color == 4)strip.setPixelColor(address, 0x00FFFF); // dot color Aqua
          else if (dot_color == 5)strip.setPixelColor(address, 0x0000FF); // dot color Blue
          else if (dot_color == 6)strip.setPixelColor(address, 0xFF00FF); // dot color Magenta/pink
          else if (dot_color == 7)strip.setPixelColor(address, 0xFFFFFF); // dot color White
          else if (dot_color == 8)strip.setPixelColor(address, Wheel(9 - jj + (10 * ii))); // dot color Rainbow still
          else ;
        }
        else if (display_band[ii] > jj)
        {
          if (line_color == 0)strip.setPixelColor(address, Wheel(9 - jj + (10 * ii) + color_offset )   ); // line color : rainbow moving
          else if (line_color == 1)strip.setPixelColor(address, 0xFF0000); // line color : Red
          else if (line_color == 2)strip.setPixelColor(address, 0xFFFF00 ); // line color : Yellow
          else if (line_color == 3)strip.setPixelColor(address, 0x00FF00 ); // line color : Lime Green
          else if (line_color == 4)strip.setPixelColor(address, 0x00FFFF ); // line color : Aqua
          else if (line_color == 5)strip.setPixelColor(address, 0x0000FF ); // line color : Blue
          else if (line_color == 6)strip.setPixelColor(address, 0xFF00FF ); // line color : Magenta/pink
          else if (line_color == 7)strip.setPixelColor(address, 0xFFFFFF ); // line color : White
          else if (line_color == 8)strip.setPixelColor(address, Wheel(9 - jj + (10 * ii))   ); // line color : rainbow still
          else if (line_color == 9) { //green to red
            if ((jj >= 1) && (jj <= 2))strip.setPixelColor(address, 0x00FF00 );
            else if ((jj > 2) && (jj <= 3))strip.setPixelColor(address, 0x247600 );
            else if ((jj > 3) && (jj <= 4))strip.setPixelColor(address, 0x366400 );
            else if ((jj > 4) && (jj <= 5))strip.setPixelColor(address, 0x585200 );
            else if ((jj > 5) && (jj <= 6))strip.setPixelColor(address, 0x623800 );
            else if ((jj > 6) && (jj <= 7))strip.setPixelColor(address, 0x742600 );
            else if ((jj > 7) && (jj <= 8))strip.setPixelColor(address, 0x861400 );
            else if ((jj > 8) && (jj <= 12))strip.setPixelColor(address, 0xFF0000 );
            else;
          }
          else if (line_color == 10) { //blue to red
            if ((jj >= 1) && (jj <= 2))strip.setPixelColor(address, 0x0000FF );
            else if ((jj > 2) && (jj <= 3))strip.setPixelColor(address, 0x240076 );
            else if ((jj > 3) && (jj <= 4))strip.setPixelColor(address, 0x360064 );
            else if ((jj > 4) && (jj <= 5))strip.setPixelColor(address, 0x580052 );
            else if ((jj > 5) && (jj <= 6))strip.setPixelColor(address, 0x620038 );
            else if ((jj > 6) && (jj <= 7))strip.setPixelColor(address, 0x740026 );
            else if ((jj > 7) && (jj <= 8))strip.setPixelColor(address, 0x860014 );
            else if ((jj > 8) && (jj <= 12))strip.setPixelColor(address, 0xFF0000 );
            else;
          }
          else if (line_color == 11) { //aqua to red
            if ((jj >= 1) && (jj <= 2))strip.setPixelColor(address, 0x00FFFF );
            else if ((jj > 2) && (jj <= 3))strip.setPixelColor(address, 0x247676 );
            else if ((jj > 3) && (jj <= 4))strip.setPixelColor(address, 0x366464 );
            else if ((jj > 4) && (jj <= 5))strip.setPixelColor(address, 0x585252 );
            else if ((jj > 5) && (jj <= 6))strip.setPixelColor(address, 0x623838 );
            else if ((jj > 6) && (jj <= 7))strip.setPixelColor(address, 0x742626 );
            else if ((jj > 7) && (jj <= 8))strip.setPixelColor(address, 0x861414 );
            else if ((jj > 8) && (jj <= 12))strip.setPixelColor(address, 0xFF0000 );
            else;
          }

          else ;
        }
        else   strip.setPixelColor(address, 0  );
      }
    }
    strip.show();

    if ( Serial.available() > 0 ) //For Bluetooth Control
    {
      static char rx_data = 0;
      rx_data = Serial.read();
      if ( rx_data == '1' ) {
        line_color++;
        if (line_color > 9)line_color = 0;
        Xprint();

      }
      else if ( rx_data == '2' ) {
        dot_color++;
        if (dot_color > 8)dot_color = 0;    //go up through list for dot colors.
        Xprint();

      }
      else if ( rx_data == '3')
      {
        line_color--;
        if (line_color < 0)line_color = 9;  //go down through line colors
        Xprint();

      }
      else if ( rx_data == '4')
      {
        dot_color--;
        if (dot_color < 0)dot_color = 8;
        Xprint();
      }
      else if ( rx_data == 'b') {
        brightness += 25;
        strip.setBrightness(brightness);
        if ( brightness < 25)brightness = 25;
        Serial.print("brightness: ");
        Serial.println(brightness);
      }
      else if ( rx_data == 'r')
      {
        random_flag ^= 1;
        if (random_flag) Serial.print("R ON");
        else Serial.print("R OFF");
      }
    }

    if ( random_flag )
    {
      static unsigned int random_count = 0;
      random_count++;
      if (random_count > 1000)
      {
        random_count = 0;
        dot_dir = random(2);
        line_dir = random(2);
        line_color = random(9);
        dot_color = random(9);
      }
    }
  }
}

void Xprint() {
  lineprint();
  Serial.print("|");
  Dotprint();
}

void Dotprint() {
  if (dot_color == 0)Serial.print("D.RainbowMov"); // dot color Rainbow Moving
  else if (dot_color == 1)Serial.println("D.Red"); // dot color Red
  else if (dot_color == 2)Serial.println("D.Yellow"); // dot color Yellow
  else if (dot_color == 3)Serial.println("D.Lime Green"); // dot color Lime Green
  else if (dot_color == 4)Serial.println("D.Aqua"); // dot color Aqua                                       
  else if (dot_color == 5)Serial.println("D.Blue");  // dot color Blue
  else if (dot_color == 6)Serial.println("D.Magenta/pink"); // dot color Magenta/pink
  else if (dot_color == 7)Serial.println("D.White"); // dot color White
  else if (dot_color == 8)Serial.println("D.RainbowStill"); // dot color Rainbow still
  else;
}
void lineprint() {
  if (line_color == 0)Serial.print("RainbowMov"); // dot color Rainbow Moving
  else if (line_color == 1)Serial.print("Red"); // dot color Red
  else if (line_color == 2)Serial.print("Yellow"); // dot color Yellow
  else if (line_color == 3)Serial.print("Lime Green"); // dot color Lime Green
  else if (line_color == 4)Serial.print("Aqua"); // dot color Aqua
  else if (line_color == 5)Serial.print("Blue"); // dot color Blue
  else if (line_color == 6)Serial.print("Magenta/pink"); // dot color Magenta/pink
  else if (line_color == 7)Serial.print("White"); // dot color White
  else if (line_color == 8)Serial.print("RainbowStill"); // dot color Rainbow still
  else ;
}



uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }

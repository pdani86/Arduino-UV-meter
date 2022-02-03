//UV meter - Neo pixel

#include <Adafruit_NeoPixel.h>

//NEO LED definitions
#define VU_LED_PIN_1 2 //VU meter 1 datapin
#define VU_PIX_NUM 16 //VU meter segment/pixel number
#define ANALOG_R_PIN A2 //Audio Right channel pin ADC2
#define ANALOG_L_PIN A3 //Audio Left channel pin ADC3

#define EFFECT_TICK 50 //Effect timming in milliseconds
#define MEASURE_TICK 10 //Measure timing in milliseconds

#define ANALOG_SAMPLES 5 //Number of samples from analog measurements to average

Adafruit_NeoPixel pixels1(VU_PIX_NUM, VU_LED_PIN_1, NEO_RGB + NEO_KHZ800);

unsigned long time_effect;
unsigned long time_measure;
unsigned int led_flag;

unsigned int x;
byte VU_brightness[VU_PIX_NUM];

unsigned int analog_r[ANALOG_SAMPLES]; //Analog right channel variable
unsigned int analog_l[ANALOG_SAMPLES]; //Analog left channel variable

unsigned int average_r;
unsigned int average_l;

struct Color {
	byte r;
	byte g;
	byte b;
};

Color colorStates[7] = {
	{20, 0, 0},
	{20, 0, 0},
	{20, 0, 0},
	{0, 20, 0},
	{0, 20, 0},
	{0, 0, 20},
	{0, 0, 20}
};

void setup() {
  pinMode(ANALOG_R_PIN  , INPUT);
  pinMode(ANALOG_L_PIN  , INPUT);

  Serial.begin(115200);
  Serial.println("Connecting");

  pixels1.begin();
  Serial.println("Initialize NEO pixels");
  
  pixels1.clear();
  Serial.println("Clear NEO pixels");
  
  pixels1.show();
  
  time_effect=millis()+EFFECT_TICK;
  time_measure=millis()+MEASURE_TICK;
  
  led_flag=1;
  
  x=0;

  for (int i=0; i<ANALOG_SAMPLES; i++){
    analog_r[i]=0;
    analog_l[i]=0;
  }

}

void updatePixels(const Color& color) {
	for(int i=0 ; i<VU_PIX_NUM ; i++){
		pixels1.setPixelColor(i , color.r*VU_brightness[i]/255 , color.g*VU_brightness[i]/255 , color.b*VU_brightness[i]/255 );
	}
	pixels1.show();
}

void stepLedFlag() {
	led_flag = (led_flag%6) + 1;
}

void updateState() {
	if(led_flag == 0) {
		convert_VU(average_r*8);
	} else {
		convert_VU(x*16);
	}
	
	updatePixels(colorStates[led_flag]);
	
	if(led_flag%2) {
		// paratlan 'led_flag'
		if(x<255){
			x=x+1;
		} else {
			stepLedFlag();
		}
	} else {
		// paros 'led_flag'
		if(x>0) {
			x=x-1;
		} else {
			stepLedFlag();
		}
	}
}

void measure() {
	time_measure=time_measure+MEASURE_TICK;

    for (int i=0; i<(ANALOG_SAMPLES-1); i++){
      analog_r[i]=analog_r[i+1];
      analog_l[i]=analog_l[i+1];
    }
    
    analog_r[4]=abs(511-analogRead(ANALOG_R_PIN));
    analog_l[4]=abs(511-analogRead(ANALOG_L_PIN));

    average_r=0;
    average_l=0;
    
    for (int i=0; i<ANALOG_SAMPLES; i++){
      average_r=average_r+analog_r[i];
      average_l=average_l+analog_l[i];
    }

    average_r=average_r/ANALOG_SAMPLES;
    average_l=average_l/ANALOG_SAMPLES;
    
    Serial.print("Audio R:");
    Serial.println(average_r);
    Serial.print("Audio L:");
    Serial.println(average_l);
    Serial.println("");
}

void loop() {
  if(millis()>=time_measure){
	  measure();
  }

  if(millis()>=time_effect){
    //Serial.print("Time:");
    //Serial.println(time_effect);
    time_effect=time_effect+EFFECT_TICK;
	updateState();
    //Serial.println(time_effect);
  }
}

void convert_VU(unsigned long VU_meter){
  long conversion_temp;
  for (unsigned int u=0; u<VU_PIX_NUM; u++){
  
    conversion_temp=VU_meter-(u*255);
    if(conversion_temp >=0){
      if(conversion_temp < 255){
        VU_brightness[u]=conversion_temp & 0xFF;
      }
      else{
        VU_brightness[u]=255;
      }
      }
      else{
        VU_brightness[u]=0;
      }
  }
}

/*
//Convert Pixel color to R G B
      
 unsigned int RGB_temp[3];
for (int p=0;p<VU_PIX_NUM; p++){
  RGB_temp[2]=((pixels1.getPixelColor(p) >> 16) & 0xFF);
  RGB_temp[1]=((pixels1.getPixelColor(p) >> 8) & 0xFF);
  RGB_temp[0]=(pixels1.getPixelColor(p) & 0xFF);
  Serial.print(RGB_temp[0]);
  Serial.print(",");
  Serial.print(RGB_temp[1]);
  Serial.print(",");
  Serial.println(RGB_temp[2]);
  }
Serial.println("");*/
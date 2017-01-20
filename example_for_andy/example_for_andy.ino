/*
  AnalogReadSerial
  Reads an analog input on pin 0, prints the result to the serial monitor.
  Graphical representation is available using serial plotter (Tools > Serial Plotter menu)
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.
*/
#include <math.h>

#define MAX_VAL 300
#define LED_PIN 13
#define MOTION_PIN 12
#define PLAY_PIN 11
#define FP_SCALE 1000000
#define SQRD_1023 1046529

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(PLAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);
}

unsigned int delay_val=0;
unsigned int scale=(MAX_VAL*FP_SCALE)/SQRD_1023;

#define PRINT_BUFF_LENGTH 100
char print_buff[PRINT_BUFF_LENGTH];

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  unsigned int sensorValue = analogRead(A0);

  //delay_val= (unsigned int)(((unsigned long)sensorValue)*MAX_VAL/1023);
  delay_val= (unsigned int)(    (pow((unsigned long)sensorValue,2)*MAX_VAL)/SQRD_1023);
  
  sprintf(print_buff, "%u    %u\n\r",sensorValue, delay_val);
  Serial.print(print_buff);
  // print out the value you read:

  if(digitalRead(MOTION_PIN)==HIGH)
  {
    digitalWrite(PLAY_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1);                       // wait for a second
    digitalWrite(PLAY_PIN, LOW);    // turn the LED off by making the voltage LOW
    delay(delay_val);   
    
    digitalWrite(LED_PIN, HIGH); 
  }
  else
  {
    digitalWrite(PLAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW); 
  }
  
}


#include <ESP8266WiFi.h>

#define OFF 0
#define ON  1
#define LED_PIN 2

char out_buff[100];
char state=ON;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
}


unsigned char c=0;
void loop() {
  sprintf(out_buff,"%u\n\r",c++);
  Serial.print(out_buff);      

  switch(state)
  {
    case ON:
      digitalWrite(LED_PIN, LOW);
      state=OFF;
    break;
    case OFF:
      digitalWrite(LED_PIN, HIGH);
      state=ON;
    break;
  }
  
  delay(100);  
}

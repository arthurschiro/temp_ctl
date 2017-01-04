/*
  //temp_ctl
*/

#include <string.h>
#include <EEPROM.h>

#define EEPROM_ADR_CAL_KEY 0
#define EEPROM_ADR_CAL_KEY_SIZE 4


#define AREF 5000
#define ADC_MAX 1023
#define ADC2MV AREF/ADC_MAX

#define POT_PORT A1
#define TEMP_PORT A0

#define ON_PIN 12
#define OFF_PIN 11
#define LED_PIN 13

#define OFF 0
#define ON 1
#define NUM_ACTIONS 3
#define STATE_CHANGE_PERIOD 100
#define READ_VALUES_PERIOD 10
#define SAFTEY_TIMER_PERIOD 600000
#define ACTION_TIMER_PERIOD 100
#define TRANSMIT_DURATION 300

#define IDLE_STATE  0
#define ON_ACTIVE   1
#define NULL_ACTIVE 2
#define OFF_ACTIVE  3

#define AVG_GROWTH .01
#define AVG_DECAY .01
#define DEG_PER_SEC 300
#define LINEAR_SLOPE_THRESH 300
#define DEG_PER_STEP (DEG_PER_SEC*READ_VALUES_PERIOD/1000)

#define SETPOINT_GROWTH .1
#define SETPOINT_DECAY .1

#define CONT_HYST 1500
#define MIN_T 65000
#define MAX_T 80000
#define TEMP_BORDER_BUFF 20
#define CONVERSION_SLOPE ((MAX_T-MIN_T)/AREF)

#define POT_CTL 0
#define SERIAL_CTL 1

unsigned long temp_val = 0;
char print_buff[100];
#define INPUT_BUFF_LENGTH 100
char input_buff[INPUT_BUFF_LENGTH];
char input_buff_ctr = 0;


unsigned long temp_hyst=CONT_HYST;
unsigned long serial_temp_setpoint = 0;
unsigned long pot_temp_setpoint = 0;
char setpoint_ctl_state = POT_CTL;
unsigned long last_pot_temp_setpoint = 0;


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(OFF_PIN, OUTPUT);
  pinMode(ON_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(OFF_PIN, LOW);
  digitalWrite(ON_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  //check to see if this micro has been calibrated
//  if(EEPROM.read(0)!=1 || EEPROM.read(0)!=9 || EEPROM.read(0)!=8 || EEPROM.read(0)~=3)
//  {
//    EEPROM.write(0,1);
//    EEPROM.write(1,9);
//    EEPROM.write(2,8);
//    EEPROM.write(3,3);
//
//    //set state
//    
//  }
}

// the loop routine runs over and over again forever:
void loop() {
  while (1)
  {
    handle_state_machine();
    handle_serial_ctl();
    delay(1);
  }
}

#define SET_TEMP_CMD "temp "
#define SET_HYST_CMD "hyst "

void handle_serial_ctl(void)
{
  char c;
  char *p;
  long num;

  if (Serial.available() > 0)
  {
    c = Serial.read();
    if (c == '\n' | c == '\r') //command receive complete
    {
      input_buff[input_buff_ctr] = '\0';
      input_buff_ctr = 0;
        
      if (strncmp(input_buff, SET_TEMP_CMD, strlen(SET_TEMP_CMD)) == 0)
      {
        //get the number
        num=str2long(input_buff + strlen(SET_TEMP_CMD));
        serial_temp_setpoint=num;
        last_pot_temp_setpoint=pot_temp_setpoint;
        setpoint_ctl_state=SERIAL_CTL;
        
//        sprintf(print_buff, "new setpoint/10 %u \n\r", (unsigned int)(num/10));
//        Serial.print(print_buff);
      }
      else if (strncmp(input_buff, SET_HYST_CMD, strlen(SET_HYST_CMD)) == 0)
      {
        //get the number
        num=str2long(input_buff + strlen(SET_TEMP_CMD));
        temp_hyst=num;
        
//        sprintf(print_buff, "new hist value %u \n\r", (unsigned int)(num/10));
//        Serial.print(print_buff);
      }
    }
    else
    {
      input_buff[input_buff_ctr] = c;
      input_buff_ctr = ((input_buff_ctr + 1) >= INPUT_BUFF_LENGTH) ? 0 : ++input_buff_ctr;
    }
  }
}

long str2long(char * input)
{
  char num_dig=0,i;
  long val=0, multiplier=1;

  //find number of digits
  for(i=0;input[i]>='0' && input[i]<='9';++i)
    num_dig=num_dig+1;
        
  //compute value
  for(i=0;i<num_dig;++i)
  {
    val=val+multiplier*(input[num_dig-1-i]-'0');
    multiplier=multiplier*10;
  }
  
  return val;
}

void handle_state_machine(void)
{
  static unsigned long state_change_timer = 0;
  static unsigned long read_values_timer = 0;
  static unsigned long action_timer = 0;
  static unsigned long transmit_timer = 0;
  static unsigned long saftey_timer = 0;
  static unsigned char action_counter = 0;
  static unsigned char action_state = IDLE_STATE;
  static unsigned char state = OFF;
  unsigned long temp_setpoint,state_print_val;

  //
  if (elapsed_time(read_values_timer) > READ_VALUES_PERIOD)
  {
    read_values_timer = millis();
    refresh_setpoint_and_temp();
  }

  //
  if (elapsed_time(saftey_timer) > SAFTEY_TIMER_PERIOD)
  {
    saftey_timer = millis();
    action_counter = NUM_ACTIONS;
  }

  //
  if (elapsed_time(state_change_timer) > ACTION_TIMER_PERIOD)
  {
    state_change_timer = millis();

    if(setpoint_ctl_state==POT_CTL)
      temp_setpoint=pot_temp_setpoint;
    else
      temp_setpoint=serial_temp_setpoint;

    if(state==OFF)
      state_print_val=temp_setpoint-temp_hyst+((unsigned long)(((double)temp_hyst)*.3));
    else
      state_print_val=temp_setpoint-((unsigned long)(((double)temp_hyst)*.3));
      
      sprintf(print_buff,"%u %u %u %u\r\n",
                (unsigned int)(temp_val/10),
                (unsigned int)(temp_setpoint/10),
                (unsigned int)((temp_setpoint-temp_hyst)/10),
                (unsigned int)(state_print_val/10));
      Serial.print(print_buff);

    switch (state)
    {
      case OFF:
        if (temp_val < (temp_setpoint-temp_hyst))
        {
          state = ON;
          digitalWrite(LED_PIN, HIGH);
          action_counter = NUM_ACTIONS;
          saftey_timer = millis();
        }
        break;
      case ON:
        if (temp_val > temp_setpoint)
        {
          state = OFF;
          digitalWrite(LED_PIN, LOW);
          action_counter = NUM_ACTIONS;
          saftey_timer = millis();
        }
        break;
    }
  }

  //
  if (elapsed_time(action_timer) > ACTION_TIMER_PERIOD)
  {
    action_timer = millis();

    switch (action_state)
    {
      case IDLE_STATE:
        if (action_counter)
        {
          if (state == ON)
          {
            on_action();
            transmit_timer = millis();
            action_state = ON_ACTIVE;
          }
          else //state==OFF
          {
            off_action();
            transmit_timer = millis();
            action_state = OFF_ACTIVE;
          }
        }
        break;
      case ON_ACTIVE:
        if (elapsed_time(transmit_timer) > TRANSMIT_DURATION)
        {
          no_action();
          transmit_timer = millis();
          action_state = NULL_ACTIVE;
        }
        break;
      case OFF_ACTIVE:
        if (elapsed_time(transmit_timer) > TRANSMIT_DURATION)
        {
          no_action();
          transmit_timer = millis();
          action_state = NULL_ACTIVE;
        }
        break;
      case NULL_ACTIVE:
        if (elapsed_time(transmit_timer) > TRANSMIT_DURATION)
        {
          no_action();
          if (action_counter > 0)
            action_counter--;

          action_state = IDLE_STATE;
        }
        break;
    }
  }

}

void refresh_setpoint_and_temp(void)
{
  unsigned long setpoint_now, temp_now,diff;
  unsigned int offset;

  //read setpoint value
  setpoint_now = ((unsigned long)(((double)read_adc_mv(POT_PORT)) * CONVERSION_SLOPE)) + MIN_T;

  //if calibration needs to be done
  if (pot_temp_setpoint == 0)
    pot_temp_setpoint = setpoint_now;

  //implement exponential average
  if (setpoint_now > pot_temp_setpoint)
    pot_temp_setpoint = pot_temp_setpoint + ((unsigned long)(((double)(setpoint_now - pot_temp_setpoint)) * SETPOINT_GROWTH));
  else
    pot_temp_setpoint = pot_temp_setpoint - ((unsigned long)(((double)(pot_temp_setpoint - setpoint_now)) * SETPOINT_DECAY));

  if(setpoint_ctl_state==SERIAL_CTL)
  {
    if((pot_temp_setpoint>=(MAX_T-TEMP_BORDER_BUFF))||(pot_temp_setpoint<=(MIN_T+TEMP_BORDER_BUFF)))
      setpoint_ctl_state=POT_CTL;
  }
  ///////////////////////////////////////////////////////////////////////////////////////////////

  //read temp value
  temp_now = read_temp_F(TEMP_PORT);
//  temp_now=pot_temp_setpoint;
  
  //if calibration needs to be done
  if (temp_val == 0)
  {
    temp_val = temp_now;
//    clear_plot(temp_now);
  }

  //implement linear averager
  if (temp_now > temp_val)
  {
    diff = temp_now-temp_val;
    diff = diff>LINEAR_SLOPE_THRESH ? (unsigned long)DEG_PER_STEP :(unsigned long)(((double)diff) * AVG_GROWTH);
    temp_val = temp_val + diff;
  }
  else
  {
    diff=temp_val - temp_now;
    diff = diff>LINEAR_SLOPE_THRESH ? (unsigned long)DEG_PER_STEP :(unsigned long)(((double)diff) * AVG_DECAY);
    temp_val = temp_val - diff;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////

//if(setpoint_ctl_state==POT_CTL)
//{
//      sprintf(print_buff,"%u %u %u ",
//            (unsigned int)(setpoint_now/10),
//            (unsigned int)(pot_temp_setpoint/10) +digitalRead(ON_PIN)*200-digitalRead(OFF_PIN)*200,
//            (unsigned int)((pot_temp_setpoint-temp_hyst)/10));
//}
//else
//{
//      sprintf(print_buff,"%u %u %u ",
//            (unsigned int)(serial_temp_setpoint/10) +digitalRead(ON_PIN)*200-digitalRead(OFF_PIN)*200,
//            (unsigned int)((serial_temp_setpoint-temp_hyst)/10));
//}
//      Serial.print(print_buff);
  
//      sprintf(print_buff,"%u %u\r\n",(unsigned int)(temp_now/10),(unsigned int)(temp_val/10));
//      Serial.print(print_buff);

}



void clear_plot(unsigned int val)
{
  unsigned int count;
  for (count = 0; count < 501; count++)
    Serial.println(val);
}

unsigned long elapsed_time(unsigned long then)
{
  unsigned long now;
  now = millis();
  if (now >= then)
    return now - then;
  else
    return now;
}




unsigned long read_adc_mv(char channel)
{
  return (unsigned long)(((double)analogRead(channel)) * ADC2MV);
}

unsigned long read_temp_C(char channel)
{
//  return (unsigned long)((((double)analogRead(channel)) * ADC2MV - 750) * 100 + 25000);
  return (unsigned long)((((double)analogRead(channel)) * ADC2MV - 3010)*9.009 + 23900);
}

unsigned long read_temp_F(char channel)
{
  return (unsigned long)(((double)read_temp_C(channel)) * 1.8 + 32000);
}




void turn_on(void)
{
  on_action();
  delay(200);
  no_action();
}

void turn_off(void)
{
  off_action();
  delay(200);
  no_action();
}

void on_action(void)
{
  digitalWrite(ON_PIN, HIGH);
  digitalWrite(OFF_PIN, LOW);
}

void off_action(void)
{
  digitalWrite(OFF_PIN, HIGH);
  digitalWrite(ON_PIN, LOW);
}

void no_action(void)
{
  digitalWrite(OFF_PIN, LOW);
  digitalWrite(ON_PIN, LOW);

}


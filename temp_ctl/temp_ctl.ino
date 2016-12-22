/*
//temp_ctl
*/
#define AREF 5000
#define ADC_MAX 1023
#define ADC2MV AREF/ADC_MAX

#define POT_PORT A1
#define TEMP_PORT A0

#define AND_PIN 12
#define OFF_PIN 11
#define ON_PIN 10
#define LED_PIN 13

#define AVG_GROWTH .01
#define AVG_DECAY .01

#define SETPOINT_GROWTH .1
#define SETPOINT_DECAY .1

#define CONT_HYST 4000
#define MIN_T 50000
#define MAX_T 100000
#define CONVERSION_SLOPE ((MAX_T-MIN_T)/AREF)

unsigned long temp_setpoint=0;
unsigned long temp_val=0;
char print_buff[100];

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(AND_PIN, OUTPUT);
  pinMode(OFF_PIN, OUTPUT);
  pinMode(ON_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(AND_PIN, LOW);
  digitalWrite(OFF_PIN, LOW);
  digitalWrite(ON_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
}

// the loop routine runs over and over again forever:
void loop() {
  while (1)
  {
    handle_state_machine();
    delay(1);
  }
}

#define OFF 0
#define ON 1
#define NUM_ACTIONS 3
#define STATE_CHANGE_PERIOD 100
#define READ_VALUES_PERIOD 10
#define SAFTEY_TIMER_PERIOD 300000
#define ACTION_TIMER_PERIOD 10
#define TRANSMIT_DURATION 500

#define IDLE_STATE  0
#define ON_ACTIVE   1
#define NULL_ACTIVE 2
#define OFF_ACTIVE  3

void handle_state_machine(void)
{
  static unsigned long state_change_timer=0;
  static unsigned long read_values_timer=0;
  static unsigned long action_timer=0;
  static unsigned long transmit_timer=0;
  static unsigned long saftey_timer=0;
  static unsigned char action_counter=0;
  static unsigned char state=OFF;
  static unsigned char action_state=IDLE_STATE;
  

  //
  if(elapsed_time(read_values_timer)>READ_VALUES_PERIOD)
  {
    read_values_timer=millis();
    refresh_setpoint_and_temp();
  }

  //
  if(elapsed_time(saftey_timer)>SAFTEY_TIMER_PERIOD)
  {
    saftey_timer=millis();
    action_counter=NUM_ACTIONS;
  }
  
  //
  if(elapsed_time(state_change_timer)>ACTION_TIMER_PERIOD)
  {
    state_change_timer=millis();

    switch(state)
    {
      case OFF:
        if(temp_val<temp_setpoint)
        {
          state=ON;
          digitalWrite(LED_PIN, HIGH);
          action_counter=NUM_ACTIONS;
          saftey_timer=millis();
        }
      break;
      case ON:
        if(temp_val>(temp_setpoint+CONT_HYST))
        {
          state=OFF;
          digitalWrite(LED_PIN, LOW);
          action_counter=NUM_ACTIONS;
          saftey_timer=millis();
        }
      break;      
    }
  }

  //
  if(elapsed_time(action_timer)>ACTION_TIMER_PERIOD)
  {
    action_timer=millis();
    
    switch(action_state)
    {
      case IDLE_STATE:
        if(action_counter)
        {
          if(state==ON)
          {
            on_action();
            transmit_timer=millis();
            action_state=ON_ACTIVE;
          }
          else //state==OFF
          {
            off_action();
            transmit_timer=millis();
            action_state=OFF_ACTIVE;
          }
        }
      break;
      case ON_ACTIVE:
        if(elapsed_time(transmit_timer)>TRANSMIT_DURATION)
        {
            no_action();
            transmit_timer=millis();
            action_state=NULL_ACTIVE;
        }
      break; 
      case OFF_ACTIVE:
        if(elapsed_time(transmit_timer)>TRANSMIT_DURATION)
        {
            no_action();
            transmit_timer=millis();
            action_state=NULL_ACTIVE;
        }
      break; 
      case NULL_ACTIVE:
        if(elapsed_time(transmit_timer)>TRANSMIT_DURATION)
        {
            no_action();
            if(action_counter>0)
              action_counter--;
              
            action_state=IDLE_STATE;
        }
      break;      
    }
  }
  
}

void refresh_setpoint_and_temp(void)
{
  unsigned long setpoint_now,temp_now;
  unsigned int val1,val2,vala,valb,valc;
  
  //read setpoint value  
  setpoint_now=((unsigned long)(((double)read_adc_mv(POT_PORT))*CONVERSION_SLOPE))+MIN_T;
  
  //if calibration needs to be done
  if (temp_setpoint == 0)
    temp_setpoint = setpoint_now;
  
  //implement exponential average
  if (setpoint_now > temp_setpoint)
    temp_setpoint = temp_setpoint + ((unsigned long)(((double)(setpoint_now - temp_setpoint)) * SETPOINT_GROWTH));
  else
    temp_setpoint = temp_setpoint - ((unsigned long)(((double)(temp_setpoint - setpoint_now)) * SETPOINT_DECAY));

///////////////////////////////////////////////////////////////////////////////////////////////
    
  //read temp value
  temp_now = read_temp_F(TEMP_PORT);
  
  //if calibration needs to be done
  if (temp_val == 0)
    temp_val = temp_now;
  
  //implement exponential averager
  if (temp_now > temp_val)
    temp_val = temp_val + ((unsigned long)(((double)(temp_now - temp_val)) * AVG_GROWTH));
  else
    temp_val = temp_val - ((unsigned long)(((double)(temp_val - temp_now)) * AVG_DECAY));

///////////////////////////////////////////////////////////////////////////////////////////////
    vala=900;
    valb=800;
    valc=1000;
    val1=1000;
    val2=(temp_val/10)-2*val1;
    sprintf(print_buff,"%u %u %u ",(unsigned int)(digitalRead(AND_PIN)*vala +val2),(unsigned int)(digitalRead(ON_PIN)*valb +val2),(unsigned int)(digitalRead(OFF_PIN)*valc +val2));
    Serial.print(print_buff);
    sprintf(print_buff,"%u %u %u ",(unsigned int)(setpoint_now/10),(unsigned int)(temp_setpoint/10),(unsigned int)((temp_setpoint+CONT_HYST)/10));
    Serial.print(print_buff);
    sprintf(print_buff,"%u %u\r\n",(unsigned int)(temp_now/10),(unsigned int)(temp_val/10));
    Serial.print(print_buff);

}



void clear_plot(unsigned int val)
{
  unsigned int count;
  for(count=0;count<501;count++)
    Serial.println(val);
}

unsigned long elapsed_time(unsigned long then)
{
  unsigned long now;
  now=millis();
  if(now>=then)
    return now-then;
  else
    return now;
}




unsigned long read_adc_mv(char channel)
{
  return (unsigned long)(((double)analogRead(channel)) * ADC2MV);
}

unsigned long read_temp_C(char channel)
{
  return (unsigned long)((((double)analogRead(channel)) * ADC2MV - 750) * 100 + 25000);
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
//  pinMode(ON_PIN, OUTPUT);
  digitalWrite(ON_PIN, HIGH);
  digitalWrite(OFF_PIN, LOW);
  digitalWrite(AND_PIN, HIGH);
}

void off_action(void)
{
//  pinMode(OFF_PIN, OUTPUT);
  digitalWrite(OFF_PIN, HIGH);
  digitalWrite(ON_PIN, LOW);
  digitalWrite(AND_PIN, HIGH);
}

void no_action(void)
{
  digitalWrite(OFF_PIN, LOW);
  digitalWrite(ON_PIN, LOW);
  digitalWrite(AND_PIN, LOW);
  
//  pinMode(ON_PIN, INPUT);
//  pinMode(OFF_PIN, INPUT);
}


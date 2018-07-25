#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
const int buzzer=8;
#define LIMIT 94
int tempPin = A0;
int tempPin2=A1;
int value;
float tempC;
volatile int heart_rate;
volatile int analog_data;              
volatile int time_between_beats = 600;            
volatile boolean pulse_signal = false;    
volatile int beat[10];         //heartbeat values will be sotred in this array    
volatile int peak_value = 512;          
volatile int trough_value = 512;        
volatile int thresh = 525;              
volatile int amplitude = 100;                 
volatile boolean first_heartpulse = true;      
volatile boolean second_heartpulse = false;    
volatile unsigned long samplecounter = 0;   //This counter will tell us the pulse timing
volatile unsigned long lastBeatTime = 0;
int time1=millis();
char mymessage[50];
SoftwareSerial mySerial(9, 10);

byte customChar1[8] = {0b00000,0b00000,0b00011,0b00111,0b01111,0b01111,0b01111,0b01111};
byte customChar2[8] = {0b00000,0b11000,0b11100,0b11110,0b11111,0b11111,0b11111,0b11111};
byte customChar3[8] = {0b00000,0b00011,0b00111,0b01111,0b11111,0b11111,0b11111,0b11111};
byte customChar4[8] = {0b00000,0b10000,0b11000,0b11100,0b11110,0b11110,0b11110,0b11110};
byte customChar5[8] = {0b00111,0b00011,0b00001,0b00000,0b00000,0b00000,0b00000,0b00000};
byte customChar6[8] = {0b11111,0b11111,0b11111,0b11111,0b01111,0b00111,0b00011,0b00001};
byte customChar7[8] = {0b11111,0b11111,0b11111,0b11111,0b11110,0b11100,0b11000,0b10000};
byte customChar8[8] = {0b11100,0b11000,0b10000,0b00000,0b00000,0b00000,0b00000,0b00000};

void lcdsetup()
{
lcd.begin(16, 2);
lcd.createChar(1, customChar1);
lcd.createChar(2, customChar2);
lcd.createChar(3, customChar3);
lcd.createChar(4, customChar4);
lcd.createChar(5, customChar5);
lcd.createChar(6, customChar6);
lcd.createChar(7, customChar7);
lcd.createChar(8, customChar8);
  }
  
void setup()
{  
pinMode(tempPin,INPUT);
pinMode(tempPin2,INPUT);
pinMode(buzzer,OUTPUT);
lcdsetup();  
Serial.begin(9600);
mySerial.begin(9600);
interruptSetup();
delay(100);
}

void interruptSetup()
{    
  TCCR2A = 0x02;  // This will disable the PWM on pin 3 and 11
  OCR2A = 0X7C;   // This will set the top of count to 124 for the 500Hz sample rate
  TCCR2B = 0x06;  // DON'T FORCE COMPARE, 256 PRESCALER
  TIMSK2 = 0x02;  // This will enable interrupt on match between OCR2A and Timer
  sei();          // This will make sure that the global interrupts are enable
}

void loop()
{
int reading;
reading = analogRead(tempPin);
tempC = reading*0.4887;
tempC=tempC*1.8+32;
Serial.println(tempC);
Serial.println(heart_rate);
 lcd.setCursor(0, 0);
 lcd.print("Temp     Pulse");
 lcd.setCursor(0, 1);
 lcd.print(tempC);
 lcd.print("F   ");
 lcd.print(heart_rate);
 lcd.print(" BPM");
 delay(1000);
if((tempC>LIMIT||heart_rate>70)&&millis()>time1+10000){
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(buzzer,LOW);
    SendMessage();
    while(1);
  }
}
ISR(TIMER2_COMPA_vect)
{ 
  cli();                                     
  analog_data = analogRead(tempPin2);            
  samplecounter += 2;                        
  int N = samplecounter - lastBeatTime;      

  if(analog_data < thresh && N > (time_between_beats/5)*3)
    {     
      if (analog_data < trough_value)
      {                       

        trough_value = analog_data;
      }
    }

  if(analog_data > thresh && analog_data > peak_value)
    {        
      peak_value = analog_data;
    }                          

   if (N > 250)
  {                            
    if ( (analog_data > thresh) && (pulse_signal == false) && (N > (time_between_beats/5)*3) )
      {       
        pulse_signal = true;          
//        digitalWrite(led_pin,HIGH);
        time_between_beats = samplecounter - lastBeatTime;
        lastBeatTime = samplecounter;     

       if(second_heartpulse)
        {                        
          second_heartpulse = false;   
          for(int i=0; i<=9; i++)    
          {            
            beat[i] = time_between_beats; //Filling the array with the heart beat values                    
          }
        }

        if(first_heartpulse)
        {                        
          first_heartpulse = false;
          second_heartpulse = true;
          sei();            
          return;           
        }  

      word runningTotal = 0;  

      for(int i=0; i<=8; i++)
        {               
          beat[i] = beat[i+1];
          runningTotal += beat[i];
        }

      beat[9] = time_between_beats;             
      runningTotal += beat[9];   
      runningTotal /= 10;        
      heart_rate = 60000/runningTotal;

    }                      
  }

  if (analog_data < thresh && pulse_signal == true)
    {  
//      digitalWrite(led_pin,LOW); 
      pulse_signal = false;             
      amplitude = peak_value - trough_value;
      thresh = amplitude/2 + trough_value; 
      peak_value = thresh;           
      trough_value = thresh;
    }

  if (N > 2500)
    {                          
      thresh = 512;                     
      peak_value = 512;                 
      trough_value = 512;               
      lastBeatTime = samplecounter;     
      first_heartpulse = true;                 
      second_heartpulse = false;               

    }
  sei();                                
}

void SendMessage()
{
  int temp_int=floor(tempC);

  lcd.setCursor(0, 0);
 lcd.print("Temp     Pulse");
 lcd.setCursor(0, 1);
 lcd.print(tempC);
 lcd.print("F   ");
 lcd.print(heart_rate);
 lcd.print(" BPM");
 // Serial.print("SMS sent");
  //Serial.print(heart_rate);
  sprintf(mymessage,"Patient is critical \r\n Temp:%d F\r\n Pulse:%d BPM ",temp_int,heart_rate);
  mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milli seconds or 1 second
  mySerial.println("AT+CMGS=\"+918308656461\"\r\n"); 
  delay(1000);
  mySerial.println(mymessage);// The SMS text you want to send
  delay(100);
  mySerial.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
}


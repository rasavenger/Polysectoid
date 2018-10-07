#include <Oscillator.h>
#include <BasicLinearAlgebra.h>
#include <math.h>
#include <Pattern.h>
#include <RingBuf.h>

#define scale 1000 //factor 10^3 doe millisecond

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } //To make the " Serial << output " syntax possible
const double dt=0.002;
RingBuf *buf1 = RingBuf_new(sizeof(double), 1);
RingBuf *buf2 = RingBuf_new(sizeof(double), 1);
double testarr[10] = {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0};
int i = 0;
double phaseUpdate(double phase, double trinFreq, double* inPhase, double* inWeight, double* inBias){//current phase, intrinsic frequency, coupled phases, coupled weight, coupled bias
  if(sizeof(inPhase)!=sizeof(inWeight)){
    return -1;//phase 0<=x<360
  }
  int N = sizeof(inPhase);
  double sum = 0;
  
  for(int i = 0; i < N; i++){
    sum += inWeight[i]*sin(inPhase[i] - phase - inBias[i]);//calculate the total weight phase
  }
  double temp = 2*pi*trinFreq + sum;
  temp = fmod(temp, 2*pi);
  return temp;
}

int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor

void setup() {
  // put your setup code here, to run once:

  cli(); //disable interrupts 
  
  //first, set up interrupts
  //set timer0 interrupt at 2kHz
  TCCR0A = 0;// set entire TCCR0A register to 0
  TCCR0B = 0;// same for TCCR0B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR0A = 124;// = (16*10^6) / (2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS01 and CS00 bits for 64 prescaler
  TCCR0B |= (1 << CS01) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);

  //set timer1 interrupt at 4kHz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 499;// = (16*10^6) / (4000*8) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 8 prescaler
  TCCR1B |= (1 << CS11);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei(); //enable interrupts
  
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
}

ISR(TIMER0_COMPA_vect){//timer0 interrupt 2kHz
//This writes to buf1
  buf1->add(buf1, &testarr[i]);
  i = (i + 1) % 10;
}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 4kHz
  if (!buf1->isEmpty) {
    //there is one double in the buffer
    double d;
    buf1->pull(buf1, &d);
    buf2->add(buf2, &d);
  }
}

void loop() {
  //Serial << analogRead(sensorPin) << '\n';
  //delay(10);

  if (!buf2->isEmpty) {
    double d;
    buf2->pull(buf2, &d);
    Serial.println(d);
  }

  long start = micros();
  long end = 0;
  do {
    end = micros();
  } while (start + 250 >= end);
  
  /*// put your main code here, to run repeatedly:
  double weight[5]={3.2,4.5,7.8,9,10};
  Oscillator testSeg(1,0,0,weight);
  //Serial << sizeof(double);
  for(int i=0;i<5;i++){
    Serial << (double) testSeg.getWeight(i) << '\n';
    delay(800);
  }*/
}
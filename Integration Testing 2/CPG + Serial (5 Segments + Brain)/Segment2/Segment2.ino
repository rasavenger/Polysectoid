#include "cpg.cpp"  
#include <AltSoftSerial.h>
#include <RingBuf.h>
#include "Serial_IB.h"

Actuator actuators [3];    // Oscillators 1, 6, 11
bool actuators_set[3] = {false,false,false};
AltSoftSerial altSerial;
RingBuf *brain_buf = RingBuf_new(124 * sizeof(char), 1);
RingBuf *seg_buf = RingBuf_new(14 * sizeof(char), 10);
int actuator_count = 0;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  altSerial.begin(9600);

  // Hardcoding initial phases
  actuators[0].phase = 0.7;
  actuators[1].phase = 0.7;
  actuators[2].phase = 0.7;

  static int read_flag = 0;

  // IMPORT ALL FIELDS FOR EACH ACTUATOR HERE
  while(!actuators_set[0] || !actuators_set[1] || !actuators_set[2] || actuator_count < 12){           // Break out once all 3 actuatos have received all data from brain
    while(brain_buf->isEmpty(brain_buf)){
      readData(altSerial, read_flag, 1);
    }
    
    char *brain_packet = (char *) malloc(124*sizeof(char));
    brain_buf->pull(brain_buf, brain_packet);
    int brain_idx = 0;
    if (brain_packet[brain_idx] == 'b') {//just to verify
      brain_idx += 4;
      if (brain_packet[brain_idx-2] == '0' && brain_packet[brain_idx-1] == '1') {
        Serial.write("ACTUATOR 01\n");
        actuator_count++;
        setActuators(brain_packet, brain_idx, 0);
      } else if (brain_packet[brain_idx-2] == '0' && brain_packet[brain_idx-1] == '6') {
        Serial.write("ACTUATOR 06\n");
        actuator_count++;
        setActuators(brain_packet, brain_idx, 1);
      } else if (brain_packet[brain_idx-2] == '1' && brain_packet[brain_idx-1] == '1') {
        Serial.write("ACTUATOR 11\n");
        actuator_count++;
        setActuators(brain_packet, brain_idx, 2);
      } else {
        actuator_count++;
        //send these values off to the next arduino
        sendData(altSerial, brain_packet, 125);
      }
    }
    free(brain_packet);
  }

  //test to make sure we got the goods
  Serial.write("Weights:\n");
  for (int i = 0; i < SIZE; i++) {
    Serial.print(actuators[0].weights[i],4);
    Serial.write(", ");
    Serial.print(actuators[1].weights[i],4);
    Serial.write(", ");
    Serial.println(actuators[2].weights[i],4);
  }
  Serial.write("Biases:\n");
  for (int i = 0; i < SIZE; i++) {
    Serial.print(actuators[0].bias[i],4);
    Serial.write(", ");
    Serial.print(actuators[1].bias[i],4);
    Serial.write(", ");
    Serial.println(actuators[2].bias[i],4);
  }
  Serial.write("Freq:\n");
  Serial.print(actuators[0].int_freq,4);
  Serial.write(", ");
  Serial.print(actuators[1].int_freq,4);
  Serial.write(", ");
  Serial.println(actuators[2].int_freq,4);
  Serial.write("Phase:\n");
  Serial.print(actuators[0].phase,4);
  Serial.write(", ");
  Serial.print(actuators[1].phase,4);
  Serial.write(", ");
  Serial.println(actuators[2].phase,4);
  Serial.write("Tau:\n");
  Serial.print(actuators[0].tau,4);
  Serial.write(", ");
  Serial.print(actuators[1].tau,4);
  Serial.write(", ");
  Serial.println(actuators[2].tau,4);
    
  while(1);
  delay(1000);      // Chill for a sec to make sure all other arduinos are ready
  
}

void setActuators(char packet[124], int &bi, int actuator_num) {
  //2 digits to specify oscillator number 00-14 - 5
  //space - 6 <--- we start off here
  //7 weights -- decimal precision of 4, so x.xxxx 6 chars with a space after so 7, 7*7 = 49 -- 55
  //7 biases -- same as weights, 6 chars+space, 7*7 = 49 -- 104
  //1 freq -- 6+1 = 7 -- 111
  //1 phase -- 6+1 = 7 -- 118
  //1 tau -- 6+0 (no space needed) = 6 -- 124
  //\n char -- 125
  //subtract all the above by 1 since \t isn't included
  //oscillators 0, 5, 10
  bi++;
  for (int i = 0; i < SIZE; i++) {
    actuators[actuator_num].weights[i] = atof(packet+bi);
    //1.2345 <-- start off pointing to 1, +6 to point at space after, +7 to point at next weight
    bi += 7;
  }
  for (int i = 0; i < SIZE; i++) {
    actuators[actuator_num].bias[i] = atof(packet+bi);
    bi += 7;
  }
  actuators[actuator_num].int_freq = atof(packet+bi);
  bi += 7;
  actuators[actuator_num].phase = atof(packet+bi);
  bi += 7;
  actuators[actuator_num].tau = atof(packet+bi);
  
  actuators_set[actuator_num] = true;
  
  return;
}

void loop() {
    // Step 1: Each arduino sends their phase to 7 other arduinos
    for(int i = 0; i < 3; i++){
      if(i == 0){
        actuators[0].neighbor_phases[0] = actuators[i].phase;
        actuators[1].neighbor_phases[5] = actuators[i].phase;
        actuators[2].neighbor_phases[5] = actuators[i].phase;
        sendData(1,actuators[i].phase);
      }
      else if(i == 1){
        actuators[0].neighbor_phases[5] = actuators[i].phase;
        actuators[1].neighbor_phases[0] = actuators[i].phase;
        actuators[2].neighbor_phases[6] = actuators[i].phase;
        sendData(6,actuators[i].phase);        
      }
      else{
        actuators[0].neighbor_phases[6] = actuators[i].phase;
        actuators[1].neighbor_phases[6] = actuators[i].phase;
        actuators[2].neighbor_phases[0] = actuators[i].phase;
        sendData(11,actuators[i].phase);          
      }
      
    }

    // Step 2: Wait until each arduino has gotten phase data from the other 7 arduinos
    for(int i = 0; i < 3; i++){
      bool ready = false;
      while(!ready){
        // Read phase data here
        receiveData();

        
        ready = true;
        for(int j = 0; j < NUM; j++){
          if(actuators[i].neighbor_phases[j] == 0){ready = false;};
        }
      }
    }
    
/*
    // Step 3: Update the phases of each arduino
    for(int i = 0; i < 3; i++){
      actuators[i].update_phase();
    }
*/
    //Serial.print( (float) phase );    
}

// Send phase data to oscillator index
void sendData(int index, float phase){


  
}

// Receieve phase data from other oscillators
void receiveData(){
  
}

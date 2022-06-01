//uno

#include <SPI.h>          
#include "nRF24L01.h"    
#include "RF24.h"       

RF24 radio(9, 10); 
const uint64_t pipes[2] = { 0xDEDEDEDEE7LL, 0xDEDEDEDEE9LL };

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 15 // номер пина к которому подключен DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long prevnrfmillis, prevnrfRmillis;

long sentByte; // 15 (header) *** (temperature shower) * (full tank) * (ready to fill) * (set end heat) 17 (footer) 
long header = 1500000000;
long temp_out;
long full_tank=0;
int ready_fill=0;
int set_end_heat=0;
int footer = 17;
long gotByte; // 14 (header) *** (temperature set) * (set heat) * (set re-heat) * (reserv) 16 (footer) 

float T;
float a;

boolean startListen=true;


void setup() {
  Serial.begin(9600); 

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(70);
  radio.enableDynamicPayloads();
  radio.setRetries(15,15);
  radio.setCRCLength(RF24_CRC_16);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);  
  radio.startListening();
}

void loop() {


if (( radio.available() )&& startListen) {
       
        radio.read(&gotByte,sizeof(gotByte));
        Serial.print("Recieved: "); Serial.println(gotByte); 
        if (millis() - prevnrfRmillis > 200) {
        startListen=false;
        prevnrfRmillis=millis();
        }
}


 
  if (millis() - prevnrfmillis > 100) 
       {
        sensors.requestTemperatures();
        T=sensors.getTempCByIndex(0);
        temp_out=T*10;
        temp_out=temp_out*100000;
        sentByte= header+temp_out+full_tank+ready_fill+set_end_heat+footer;

        radio.openWritingPipe(pipes[1]);
        radio.openReadingPipe(0,pipes[0]);  
        radio.stopListening();
        radio.write(&sentByte,sizeof(sentByte));
        Serial.print("S:");  
        Serial.print(sentByte);          
        Serial.println();
        // restore TX & Rx addr for reading       
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1,pipes[1]); 
        radio.startListening();  
        prevnrfmillis=millis();
         startListen=true;
       }

 // if (celsius < (val-5)) {digitalWrite(RELAY, HIGH);}
 
//if (celsius > val) {digitalWrite(RELAY, LOW);}
}

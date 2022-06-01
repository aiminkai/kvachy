//nano
//распиновка
#define btn1_pin 0
#define btn1_led 1
#define btn2_pin 3
#define btn2_led 4

#define water_consumption 2  //расходомер (оранжевый пин 1 разъема корпусного)

#define SW 5                 // кнопка энкодера 
#define CLK 6                // энкодер 
#define DT 7                 // энкодер 

#define pump_level 8         // уровень воды в колодце до насоса(бело-коричневый пин 4 разъема корпусного)

//#define cur_sens 14 //датчик тока A0

#define rele_pump 15         // реле насоса (оранжевый пин 2 разъема корпусного)
#define rele_well 16         // реле клапана для ведра (синий пин 3 разъема корпусного)
#define rele_shower 17       // реле клапана для душа (бело-синий пин 4 разъема корпусного)

// 18 sda A4 для lcd
// 19 scl A5 для lcd

#include "GyverButton.h"
GButton butt1(btn1_pin);
GButton butt2(btn2_pin);   
GButton butt3(pump_level);   

#include "GyverEncoder.h"
Encoder enc1(CLK, DT, SW);

//lcd питание, GND к GND, дата идет через одноименные пины
#include "LCD_1602_RUS.h"
LCD_1602_RUS lcd(0x27, 16, 2);

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
RF24 radio(9,10); //радиомодуль CE-9; CSN-10; SCK-13; MO-11; MI-12
const uint64_t pipes[2] = { 0xDEDEDEDEE7LL, 0xDEDEDEDEE9LL };

long gotByte;                     // 15 (header) *** (temperature shower) * (full tank) * (ready to fill) * (reserv) 17 (footer) 
long header = 1400000000;         // хидер (первые 14)
long temp_set;                    // уставка преобразованная в пакет отправки 
long set_heat=0;                  // уставка греть душ
long set_re_heat=0;               // уставка включить догрев 
int reserv=0;                     // резерв
int footer = 16;                  // футер (последние (16)
long sentByte;                    // 14 (header) *** (temperature set) * (set heat) * (set re-heat) * (reserv) 16 (footer) 
int T=32;                         // уставка 

unsigned long prevnrfmillis, prevnrfRmillis;


boolean modeMenu=false;           // 0 работа системы, 1 настройка
byte modeSetings = false;         // 0 ведро или непрерывно, 1 кол-вло л в ведре, 2 режим донагрева, //3 кол-во второго 
boolean modeSave=false;           // флаг для сохранения
boolean modeSave_yes=true;        // флаг подтверждения сохранения
boolean chckSum=false;            // контрольная сумма приемной посылки
boolean wellIsOn=false;           // колодец наливается
boolean shwIsOn=false;            // душ наливается
boolean wellIsFull=false;         // колодец полон (по датчику возле насоса)
boolean shwIsFull=false;          // душ полон
boolean flowing=false;            // влючен насос и открыт какой-нибудь клапан
boolean btn1Led=false;            // подсветка кнопки 1 
boolean btn2Led=false;            // подсветка кнопки 2
boolean startListen=true;         // для выхода из режима чтения эфира 

//************************************************************************************************

void setup(){
  //Serial.begin(9600);
  
 //========radio===================================================================================
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

 
//=============button===============================================================================
 //======= кнопка колодец
  butt1.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  butt1.setTimeout(3000);        // настройка таймаута на удержание (по умолчанию 500 мс)
  butt1.setType(HIGH_PULL);
  butt1.setDirection(NORM_OPEN);
  butt1.setTickMode(AUTO);

  //======= кнопка душ
  butt2.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  butt2.setTimeout(3000);        // настройка таймаута на удержание (по умолчанию 500 мс)
  butt2.setType(HIGH_PULL);
  butt2.setDirection(NORM_OPEN);
  butt2.setTickMode(AUTO);

  //======= датчик уровня клодец, должен быть всегда в 1
  butt3.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  butt3.setTimeout(3000);        // настройка таймаута на удержание (по умолчанию 500 мс)
  butt3.setType(HIGH_PULL);
  butt3.setDirection(NORM_OPEN);
  butt3.setTickMode(AUTO);
  if (butt3.isPress()) wellIsFull=true;  
  else if (butt3.isRelease()) wellIsFull=false;
  
//=============enc===============================================================================
  enc1.setTickMode(AUTO);

//=============LCD===============================================================================
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(L"Привет,");
  lcd.setCursor(5, 1);
  lcd.print(L"Хозяин");

//=============pin mode===============================================================================
  pinMode(rele_pump, OUTPUT);
  pinMode(rele_well, OUTPUT);
  pinMode(rele_shower, OUTPUT);
  pinMode(btn1_led, OUTPUT);
  pinMode(btn2_led, OUTPUT);
  digitalWrite(rele_pump, HIGH);
  digitalWrite(rele_well, HIGH);
  digitalWrite(rele_shower, HIGH);
  digitalWrite(btn1_led, HIGH);
  digitalWrite(btn2_led, HIGH);

}

//************************************************************************************************
void loop() {

if (wellIsFull) {                                             // если колодец полон
//============================ btn1 (колодец) ===================================
if ((butt1.isClick())&(!wellIsOn)&(!shwIsOn)&(!btn1Led)) {    // цикл если нажали кнопку - налить нужное кол-во литров

  digitalWrite(rele_well, LOW);
  digitalWrite(rele_shower, HIGH);
  digitalWrite(rele_pump, LOW);
  digitalWrite(btn1_led, HIGH);
  wellIsOn=true;

} //===== конец if налить нужное кол-во литров


if ((butt1.isClick())&(wellIsOn)&(!shwIsOn)&(btn1Led)) {       // цикл если нажали кнопку - налить нужное кол-во литров

  digitalWrite(rele_well, HIGH);
  digitalWrite(rele_shower, HIGH);
  digitalWrite(rele_pump, HIGH);
  digitalWrite(btn1_led, LOW);
  wellIsOn=false;

} //===== конец if налить нужное кол-во литров

} //===== конец if wellIsFull колодец полон 

if (butt3.isPress()) {   // уровень в колодце
  wellIsFull=true;
//  Serial.println(" уровень полон "); 
}

if (butt3.isRelease()) {   // уровень в колодце
  wellIsFull=false;
 // Serial.println(" КОЛОДЕЦ ПУСТ!! "); 
 }
  

  
//============================ радио ===================================

radio_RX();
radio_TX();
   
} // конец цикла loop 

//************************************************************************************************
//============================ функция радио Прием ===================================

void radio_RX ()
{
while (( radio.available() )&& startListen) {
       
        radio.read(&gotByte,sizeof(gotByte));
    //    Serial.print("Recieved: "); Serial.println(gotByte);
        chck_sum();    
        if (millis() - prevnrfRmillis > 2000) {
        startListen=false;
        prevnrfRmillis=millis();
        }
}
}

//============================ функция радио Передача ===================================
void radio_TX ()
{
temp_set=T*1000000;   
sentByte=header+temp_set+set_heat+set_re_heat+reserv+footer;     
 
  if (millis() - prevnrfmillis > 1000) 
       {
        radio.openWritingPipe(pipes[1]);
        radio.openReadingPipe(0,pipes[0]);  
        radio.stopListening();
        radio.write(&sentByte,sizeof(sentByte));
    //    Serial.print("S:");  
   //     Serial.print(sentByte);          
   //     Serial.println();
        // restore TX & Rx addr for reading       
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1,pipes[1]); 
        radio.startListening();  
        prevnrfmillis=millis();
         startListen=true;
       }
}  
//============================ функция проверки посылки chck_sum===================================


void chck_sum ()
{
long h=gotByte/100000000;
int f=gotByte%100;
if (h==15&f==17)
{ chckSum = 1;}
else { chckSum = 0;}
//Serial.print(" chckSum: "); Serial.println(chckSum);
}












  

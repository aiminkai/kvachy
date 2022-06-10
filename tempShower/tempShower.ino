//nano
//распиновка
#define btn1_pin 0
#define btn1_led 1
#define btn2_pin 3
#define btn2_led 4

#define level_sens1 2  //уровень воды Край-полный бак (оранжевый пин 1 разъема 1 корпусного)

#define SW 5                 // кнопка энкодера 
#define CLK 6                // энкодер 
#define DT 7                 // энкодер 

#define level_sens2 8         // уровень воды 1/3 (бело-коричневый пин 4 разъема 1 корпусного)

////радиомодуль CE-9; CSN-10; SCK-13; MO-11; MI-12

#define shwIsEmpty_led 14     //светодиод пустого бака

#define rele_heat 15         // A1 реле насоса (оранжевый пин 2 разъема 2 корпусного)
#define rele_light 16        // A2  реле клапана для ведра (синий пин 3 разъема 2 корпусного)
//#define sens_temp 17   A3 определен ниже   // реле клапана для душа (бело-синий пин 4 разъема 2 корпусного)

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 17         // номер пина к которому подключен DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// 18 sda A4 для lcd
// 19 scl A5 для lcd

//lcd питание, GND к GND, дата идет через одноименные пины
#include "LCD_1602_RUS.h"
LCD_1602_RUS lcd(0x27, 16, 2); // в скобках дефолтные настройки для такого дисплея (16 символов, 2 строки)

#include <EEPROM.h>
#define INIT_ADDR 1023  // номер резервной ячейки для первой записи в eeprom 
#define INIT_KEY 50     // ключ первого запуска. 0-254, на выбор

int tempSet=30;                    // уставка преобразованная в пакет отправки 
int temp= 10;
float T;
long timeT=0;

boolean heatIsOn=false;           // уставка греть душ
boolean heatHyst=false;           // режим греть включен, реле выключено
boolean modeMenu=false;           // 0 работа системы, 1 настройка
boolean modeSave=false;           // флаг для сохранения
boolean modeSave_yes=true;        // флаг подтверждения сохранения
boolean shwIsFull=false;          // душ полон
boolean shwIsEmpty=false;         // душ 1/3
boolean btn1Led=false;            // подсветка кнопки 1 
boolean btn2Led=false;            // подсветка кнопки 2
boolean lightIsOn=false;          // свет 

boolean btn1_flag=false;
boolean btn1;
long btn1_Press_time=0;
long btn1_Release_time=0;

boolean btn2_flag=false;
boolean btn2;
long btn2_Press_time=0;
long btn2_Release_time=0;

boolean btn3_flag=false;
boolean btn3;
long btn3_Press_time=0;
long btn3_Release_time=0;
long shwIsFull_time=0;
boolean shwIsFullLightState=false;

boolean btn4_flag=false;
boolean btn4;
long btn4_Press_time=0;
long btn4_Release_time=0;

int encData_raw = 0;
int encData_true=0;
int encData_temp=0;
byte lastState = 0;
const int8_t increment[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
boolean btnEnc_flag=false;
boolean btnEnc;
long btnEnc_Press_time=0;
long btnEnc_Release_time=0;

#define  hold_time 500  // время удержания кнопки для второй функиции
#define  dbc 80         // время дребезга
#define  dbc2 800         // время дребезга2


//************************************************************************************************
//************************************************************************************************
void setup(){
  //Serial.begin(9600);
  
   if (EEPROM.read(INIT_ADDR) != INIT_KEY) { // первый запуск
       EEPROM.write(INIT_ADDR, INIT_KEY);    // записали ключ
       EEPROM.write(0, tempSet);}
    tempSet=EEPROM.read(0);
    lightIsOn=EEPROM.read(1);
   
//============= button && rele && level sensor ===============================================================================

pinMode(btn1_pin, INPUT_PULLUP);              // кнопка синяя 
pinMode(btn2_pin, INPUT_PULLUP);              // кнопка красная 
pinMode(level_sens1, INPUT_PULLUP);              // датчик уровня "полный" 
pinMode(level_sens2, INPUT_PULLUP);              // датчик уровня "пустой" 
pinMode(rele_heat, OUTPUT);
pinMode(rele_light, OUTPUT);
pinMode(btn1_led, OUTPUT);
pinMode(btn2_led, OUTPUT);
pinMode(shwIsEmpty_led, OUTPUT);
  digitalWrite(rele_heat, HIGH);
  digitalWrite(rele_light, lightIsOn);
  digitalWrite(btn1_led, !lightIsOn);
  digitalWrite(btn2_led, LOW);
  digitalWrite(shwIsEmpty_led, LOW);


//=============temp sens=========================================================================
  sensors.begin();

//=============LCD===============================================================================
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(L"hello");
  lcd.setCursor(12, 0);
  lcd.setCursor(0, 1);
  lcd.print(L"Бак: ");
  lcd.setCursor(5, 1);
  if ((shwIsFull)&&(!shwIsEmpty)){lcd.print(L"полный");}
  else  if ((!shwIsFull)&&(!shwIsEmpty)){lcd.print(L"больше 1/3");}
  else  if ((!shwIsFull)&&(shwIsEmpty)) {lcd.print(L"меньше 1/3");}

}

//************************************************************************************************
//************************************************************************************************
void loop() {

button1();

if (!modeMenu){

  if (!heatIsOn)  temperature(60000);
  else if (heatIsOn) temperature(3000);
button2();      
level_sensor1();
level_sensor2();
}

settings();

}
//************************************************************************************************
//************************************************************************************************

void menu_LCD(){
   lcd.clear();
     lcd.setCursor(5, 0); lcd.print(L"греть до:");
     lcd.setCursor(7, 1); lcd.print(tempSet);
     lcd.setCursor(9, 1);
     lcd.print((char)223);
     lcd.setCursor(10, 1);
     lcd.print(L"С");
}

void main_LCD(){
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(L"T = ");
  lcd.setCursor(6, 0);
  lcd.print(T);
  lcd.setCursor(11, 0);
  lcd.print((char)223);
  lcd.setCursor(12, 0);
  lcd.print(L"С");
  lcd.setCursor(0, 1);
  lcd.print(L"Бак: ");
  lcd.setCursor(5, 1);
  if ((shwIsFull)&&(!shwIsEmpty)){lcd.print(L"полный");digitalWrite(shwIsEmpty_led, LOW);}
  else  if ((!shwIsFull)&&(!shwIsEmpty)){lcd.print(L"больше 1/3");digitalWrite(shwIsEmpty_led, LOW);}
  else  if ((!shwIsFull)&&(shwIsEmpty)){lcd.print(L"меньше 1/3"); digitalWrite(shwIsEmpty_led, HIGH);}
  else  if ((shwIsFull)&&(shwIsEmpty)){lcd.print(L"ошибка"); digitalWrite(shwIsEmpty_led, HIGH);}

}

void heat_LCD(){
  
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(L"Нагрев до");
  lcd.setCursor(11, 0);
  lcd.print(tempSet);
  lcd.setCursor(13, 0);
  lcd.print((char)223);
  lcd.setCursor(14, 0);
  lcd.print(L"С");
  lcd.setCursor(1, 1);
  lcd.print(L"Уже");
  lcd.setCursor(5, 1);
  lcd.print(T);
  lcd.setCursor(10, 1);
  lcd.print((char)223);
  lcd.setCursor(11, 1);
  lcd.print(L"С");
  if (heatHyst){
  lcd.setCursor(13, 1); lcd.print(L"off");}
  else if (!heatHyst){
   lcd.setCursor(14, 1); lcd.print(L"on");}
  
}
//========================================== считывание и отображение температуры ==================
void temperature(int time_ask_temperature){
  if ((millis()-timeT)>time_ask_temperature){
    sensors.requestTemperatures();
    T=sensors.getTempCByIndex(0);
    temp= T;

    if (!heatIsOn&&!modeMenu){    // если нету режима нагрев
        main_LCD();
       }
    else if (heatIsOn && !modeMenu){
         heat_LCD();
        }

  timeT=millis();
  }
}

//============================ btn1 (свет) ===================================
void button1(){
btn1 = !digitalRead(btn1_pin); // считать текущее положение кнопки
  
  if (btn1 == 1 && btn1_flag == 0 && millis() - btn1_Press_time > dbc) // кнопка нажата, выставляю флаг
  {       
    btn1_flag=1;
    btn1_Press_time=millis();
  }

  if (btn1==0 && btn1_flag == 1)                      // кнопка отжата, начинаем обработку
  {
    btn1_flag=0;
    btn1_Release_time=millis();
                                           
         lightIsOn=!lightIsOn;
         digitalWrite(rele_light, lightIsOn);
         digitalWrite(btn1_led, !lightIsOn);      
         EEPROM.write(1, lightIsOn);  
         if (!lightIsOn)  shwIsFullLightState=false;
         
   }
}

//============================ btn2 (нагрев) ===================================
void button2(){
   btn2 = !digitalRead(btn2_pin); // считать текущее положение кнопки
  
  if (btn2 == 1 && btn2_flag == 0 && millis() - btn2_Press_time > dbc && !modeMenu) // кнопка нажата, выставляю флаг
  {       
    btn2_flag=1;
    btn2_Press_time=millis();
  }

  if (btn2==0 && btn2_flag == 1)                      // кнопка отжата, начинаем обработку
  {
    btn2_flag=0;
    btn2_Release_time=millis();
   
   
          heatIsOn=!heatIsOn;
          digitalWrite(btn2_led, heatIsOn);
          digitalWrite(rele_heat, !heatIsOn);
          heatHyst=false;
         if (heatIsOn){heat_LCD();}
          else  if (!heatIsOn){main_LCD();}        
         
   }

  if (heatIsOn && temp>=tempSet && !modeMenu&&!heatHyst){          // если флаг heatIsOn 1, текущая t больше или равна уставке - выключать нагрев и менять флаг гистерезиса на 1 (включить догрев)
  heatHyst=true;                                    //гистерезис догрева
  digitalWrite(rele_heat, HIGH);
  
   lightIsOn=true;                              //Включаем свет для инидикации, что вода нагрелась                        
   digitalWrite(rele_light, LOW);
   digitalWrite(btn1_led, HIGH); 
   
  heat_LCD();
   }
  
  if (heatIsOn&&(tempSet-temp>=1)&&!modeMenu&&heatHyst){
    heatHyst=false;                               //гистерезис догрева  
  digitalWrite(rele_heat, LOW);
  heat_LCD();
    }
}

//============================ уровень полного ===================================
void level_sensor1(){
  btn3 = !digitalRead(level_sens1);
if (btn3 && millis() - btn3_Press_time > dbc2 && !shwIsFull) 
    {shwIsFull=true;  
    btn3_Press_time= millis();
    
    if (!modeMenu && !heatIsOn){
      digitalWrite(rele_light, LOW);     //включаем индикацию налитого бака (свет) 
      lightIsOn=true;
      digitalWrite(btn1_led, HIGH); 
      shwIsFull_time=millis();
      shwIsFullLightState=true;
      main_LCD();   
      }
    }
 else if (!btn3 && millis() - btn3_Press_time > dbc2 && shwIsFull) 
    {shwIsFull=false;  
     btn3_Press_time= millis();
     if (!modeMenu && !heatIsOn){
     main_LCD();    
     }
    }

  if (millis()-shwIsFull_time>15000 && shwIsFullLightState){      //выключаем индикацию налитого бака (свет) через 5 секунд
    digitalWrite(rele_light, HIGH);
    digitalWrite(btn1_led, LOW);
    lightIsOn=false;
    shwIsFullLightState=false; 
  }
}

//============================ уровень пустого (1/3 чтобы успеть) ===================================
void level_sensor2(){
btn4 = !digitalRead(level_sens2);
if (btn4 && millis() - btn4_Press_time > dbc2 && shwIsEmpty) 
    {shwIsEmpty=false;  
    btn4_Press_time= millis();
    digitalWrite(shwIsEmpty_led, LOW);
    if (!modeMenu && !heatIsOn){
    main_LCD();
    }
    }
  else if (!btn4 && millis() - btn4_Press_time > dbc2 && !shwIsEmpty) 
    {shwIsEmpty=true;  
     btn4_Press_time= millis();
     digitalWrite(shwIsEmpty_led,HIGH );
     if (!modeMenu && !heatIsOn){
     main_LCD();
     }
     }
}  


//============================ настройка ===================================
void settings() {
  
btnEnc = !digitalRead(SW); // считать текущее положение кнопки
  
  if (btnEnc == 1 && btnEnc_flag == 0 && millis() - btnEnc_Press_time > dbc) // кнопка нажата, выставляю флаг
  {       
    btnEnc_flag=1;
    btnEnc_Press_time=millis();
   }

  if (btnEnc==0 && btnEnc_flag == 1)                      // кнопка отжата, начинаем обработку
  {
    btnEnc_flag=0;
    btnEnc_Release_time=millis();
        
    if (btnEnc_Release_time - btnEnc_Press_time > 1000)
        {                                                     // если долгое нажатие
          if (!modeMenu)
             {
             modeMenu=true;
             menu_LCD();
             }
          else if (modeMenu)
             {
             modeSave=1;
             modeSave_yes=1;
             lcd.clear();
             lcd.setCursor(3, 0); lcd.print(L"Сохраняем?");
             lcd.setCursor(3, 1); lcd.print(L"Да<   Нет");
             }
        }   
     else if (btnEnc_Release_time - btnEnc_Press_time < 1000)
     {
          if (modeMenu)
          {
            if (!modeSave)
             {
                lcd.setCursor(14, 1); lcd.print(L"OK");
                delay (1000);
                menu_LCD ();
              }
            else if (modeSave)
             {
                if (modeSave_yes)
                {
                 lcd.setCursor(14, 1); lcd.print(L"OK");
                 delay (1000);
                 modeSave=false;
                 modeMenu=false;
                 modeSave_yes=false;
                 EEPROM.write(0, tempSet);
                 if (heatIsOn){heat_LCD();}
                 else  if (!heatIsOn){main_LCD();}
                }
                 else if (!modeSave_yes)
                {
                 lcd.setCursor(14, 1); lcd.print(L"OK");
                 delay (1000); 
                 modeSave=false;
                 modeSave_yes=false;
                 menu_LCD ();
                }
          }
       } 
     }             
  }
//    =========== отрабатываем поворот (в цикле если мод меню) =============
 if (modeMenu)
 {
    byte state = digitalRead(DT) | (digitalRead(CLK) << 1);
    if (state != lastState) {
    encData_raw += increment[state | (lastState << 2)];
    lastState = state;
    Serial.println(encData_raw/4);
    encData_true=encData_raw/4;
    
    }
     if (encData_true>encData_temp) 
       { if (!modeSave)
            {tempSet=tempSet+1;
             if (tempSet>50) tempSet=50;
             menu_LCD ();
            }
        else  if (modeSave)
            { modeSave_yes=false;
              lcd.clear();
              lcd.setCursor(3, 0); lcd.print(L"Сохраняем?");
              lcd.setCursor(3, 1); lcd.print(L"Да   >Нет");}
         encData_temp=encData_true;  
       }
     if (encData_true<encData_temp) 
       { if (!modeSave)
            {tempSet=tempSet-1;
             if (tempSet<0) tempSet=0;
             menu_LCD ();
            }
        else  if (modeSave)
            { modeSave_yes=true;
              lcd.clear();
              lcd.setCursor(3, 0); lcd.print(L"Сохраняем?");
              lcd.setCursor(3, 1); lcd.print(L"Да<   Нет");}
         encData_temp=encData_true;
       }
 }
} 
 

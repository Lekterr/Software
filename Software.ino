#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DS3231.h>
#include <SD.h>
#include <SPI.h>

//die Pins den Knöpfen zuordnen 
#define ButtonOnePin 45
#define ButtonTwoPin 47
#define ButtonThreePin 49
#define ButtonFourPin 48

//die Pins den Relays der Geräte zuordnen 
#define FanPin 33
#define LampPin 31
#define PumpPin 30
#define HeaterPin 32

//die Pins den Geräten zuordenen
#define TransmittterPin 39
#define PhotoresistorPin A1
#define PieperPin A2
#define WatersensorPin A3
#define DHT22Pin 5
#define Poti A5

//RGB LED kofigurieren                                                                                
#define RPin 28
#define GPin 27
#define BPin 29

//Klassen----------------------------------------------------------------------------------------------
DHT dht(DHT22Pin,DHT22);
LiquidCrystal_I2C lcd(0x27,20,4);
DS3231 rtc(SDA, SCL);
File file;
Time t;

//Globale Variablen------------------------------------------------------------------------------------
const int lightStart = 3;
const int lightEnd = 21;
const int pumpTime = 18;
const int pumpFinetune = 60;  //nicht benutzt
const int aimTemp = 24;
const int humidityLimit = 70;
const int moistureLimit = 95;
const float osversion = 1.0;
const unsigned long divi = 1024;
const unsigned long multi = 100;
const String dataFile = "LOG.txt";

unsigned long potiValue;
int temp;
int moisture;
int humidity;
int light;
int pumpTimer; 
int button = 0;
int lightTimer;
float pumpDuration;
float potiConv;
float pumpsec;
bool pumpOn = false;
bool lightOn; 
bool dhtSensorWorking;
bool sdRep;

//Funktionen-------------------------------------------------------------------------------------------
//Rep-----------------
void repToSerial(){
  Serial.print("Temperatur: ");
  Serial.print(temp);
  Serial.println("C");
  Serial.print("Bodenfeuchtigkeit: ");
  Serial.print(moisture);
  Serial.println("%");
  Serial.print("Luftfeuchtigkeit: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("Licht: ");
  Serial.print(light);
  Serial.println("%");
  
  if(!digitalRead(HeaterPin)){Serial.println("Heizung ist an!");}
  else{Serial.println("Heizung ist aus!");}

  if(!digitalRead(FanPin)){Serial.println("Lüfter ist an!");}
  else{Serial.println("Lüfter ist aus!");}

  if(!digitalRead(PumpPin)){Serial.println("Pumpe ist an!");}
  else{Serial.println("Pumpe ist aus! 18 Uhr ON");}

  if(!digitalRead(LampPin)){Serial.println("Lampe ist an! 3-22 Uhr");}
  else{Serial.println("Lampe ist aus! 22-3 Uhr");}

  Serial.println("------------------------------");
}

void repToSD(){
  if(((t.min % 5) == 0) and sdRep){
    if(SD.exists(dataFile)){
      file = SD.open(dataFile, FILE_WRITE);
      file.print(String(rtc.getUnixTime(t)));
      file.print(";");
      file.print(t.date);
      file.print("/");
      file.print(t.mon);
      file.print("/");
      file.print(t.year);
      file.print(";");
      file.print(t.hour);
      file.print(":");
      file.print(t.min);
      file.print(";");
      file.print(String(pumpsec));
      file.print(",");
      file.print(String(temp));
      file.print(",");
      file.print(String(moisture));
      file.print(",");
      file.print(String(humidity));
      file.print(",");
      file.println(String(light));
      file.close();
      sdRep = false;
    }
    else{
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("dataFile existiert nicht!");
      for(int x=0; x<5; x++){
        digitalWrite(PieperPin, HIGH);
        delay(200);
        digitalWrite(PieperPin, LOW);
        delay(100);
      } 
      delay(10000);
      lcd.clear();
      lcd.noBacklight();
      sdRep = false;
    }  
  }
  else if((t.min % 5) != 0){
    sdRep = true;
  }
}

void repToLcdSensor(){

   lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Temperatur: ");
  lcd.print(temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Bodenfeuchte: ");
  lcd.print(moisture);
  lcd.print("%");
  lcd.setCursor(0, 2);
  lcd.print("Luftfeuchte: ");
  lcd.print(humidity);
  lcd.print("%");
  lcd.setCursor(0, 3);
  lcd.print("Licht: ");
  lcd.print(light);
  lcd.print("%");
}

void repToLcdStatus(){
  lcd.backlight();
  lcd.setCursor(0, 0);
  if(!digitalRead(HeaterPin)){lcd.print("Heizung ON!");}
  else{lcd.print("Heizung OFF!");}

  lcd.setCursor(0, 1);
  if(!digitalRead(FanPin)){lcd.print("Luefter ON!");}
  else{lcd.print("Luefter OFF!");}

  lcd.setCursor(0, 2);
  if(!digitalRead(PumpPin)){lcd.print("Pumpe ON!");}
  else{lcd.print("Pumpe OFF!->18:00 ON");}

  lcd.setCursor(0, 3);
  if(!digitalRead(LampPin)){lcd.print("Lampe ON! 3-22 Uhr");}
  else{lcd.print("Lampe OFF! 22-3 Uhr");}
}

void repToLcdInfo(){
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(t.date);
  lcd.print(".");
  lcd.print(t.mon);
  lcd.print(".");
  lcd.print(t.year);
  lcd.print("   ");
  lcd.print(t.hour);
  lcd.print(":");
  lcd.print(t.min);
  lcd.print(":");
  lcd.print(t.sec);

  lcd.setCursor(0, 1);
  lcd.print("Bewaesserung:");
  lcd.print(int(pumpsec));
  lcd.print(" sek");
  
  lcd.setCursor(0, 2);
  lcd.print("SD:5 min  OS:V");
  lcd.print(osversion);
  lcd.setCursor(0, 3);
  lcd.print("Temp:");
  lcd.print(aimTemp);
  lcd.print("C ");
  lcd.print("Hum:");
  lcd.print(humidityLimit);
  lcd.print("%"); 
}

void LcdOff(){
    lcd.noBacklight();
}

void SystCheck(){
  if(temp == 0 or humidity == 0 or moisture == 0 or temp >= 29 or temp <= 17){
    for(int x=0; x<5; x++){
        digitalWrite(PieperPin, HIGH);
        digitalWrite(RPin, HIGH);
        delay(50);
        digitalWrite(PieperPin, LOW);
        digitalWrite(RPin, LOW);
        delay(50);
    }
  }
  else{
    if(temp >= 27 or temp <= 20 or humidity >= 80 or humidity <= 35 or moisture <= 30){
    digitalWrite(GPin, HIGH);
    digitalWrite(RPin, HIGH);
    delay(50);
    digitalWrite(GPin, LOW);
    digitalWrite(RPin, LOW); 
    }
  }
}

void alarm(){
  if(dhtSensorWorking){
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("DHT Sensor funktioniert nicht!");
    for(int x=0; x<5; x++){
      digitalWrite(PieperPin, HIGH);
      delay(100);
      digitalWrite(PieperPin, LOW);
      delay(100);
    } 
    delay(10000);
    lcd.clear();
    lcd.noBacklight();
  }
}

void buttonFeedback(){
  lcd.clear();
  digitalWrite(GPin,HIGH);
  digitalWrite(PieperPin,HIGH);
  delay(50);
  digitalWrite(GPin,LOW);
  digitalWrite(PieperPin,LOW);
}

//Controll---------------
void controll(){ 
    if(temp < aimTemp){
      digitalWrite(HeaterPin, LOW);
      digitalWrite(FanPin, HIGH);
    }
    if(temp > aimTemp +1 ){
      digitalWrite(FanPin, LOW);
      digitalWrite(HeaterPin, HIGH);    
    }
    
    if (temp == aimTemp){
      digitalWrite(FanPin, HIGH);
      digitalWrite(HeaterPin,HIGH);
    }
 
if((t.hour >= lightStart) and ((t.hour <= lightEnd))){
  digitalWrite(LampPin, LOW);
}
else {digitalWrite(LampPin, HIGH);}

if((t.hour == pumpTime) and  (t.min == 0) and (t.sec == 1) and (moisture < moistureLimit)){
  digitalWrite(PumpPin, LOW);
  digitalWrite(BPin, HIGH);
  digitalWrite(PieperPin,HIGH);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Watering...");
  lcd.setCursor(0, 2);
  lcd.print("System paused. wait!");
  delay(100);
  digitalWrite(PieperPin, LOW);
  delay(pumpDuration);        
  lcd.clear();
  digitalWrite(PumpPin, LOW); 
    } 
   
else{digitalWrite(PumpPin, HIGH);
   digitalWrite(BPin, LOW);}

 if((humidity >= humidityLimit) and dhtSensorWorking){digitalWrite(FanPin, LOW);}
  
}

void poti(){
  potiConv = (analogRead(Poti))*100UL;
  pumpDuration = (potiConv/1024)*1000UL;
  pumpsec =(potiConv/1024);
}

void setup() {
 
  // put your setup code here, to run once:
  digitalWrite(RPin, HIGH);
  digitalWrite(PieperPin,HIGH);
  Serial.begin(9600);
  Wire.begin();
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    while (1);
  }
  lcd.init();
  dht.begin();
  rtc.begin();

  pinMode(ButtonOnePin, INPUT_PULLUP);
  pinMode(ButtonTwoPin, INPUT_PULLUP);
  pinMode(ButtonThreePin, INPUT_PULLUP);
  pinMode(ButtonFourPin, INPUT_PULLUP);

  pinMode(FanPin, OUTPUT);
  pinMode(LampPin, OUTPUT);
  pinMode(PumpPin, OUTPUT);
  pinMode(HeaterPin, OUTPUT);
  pinMode(PieperPin, OUTPUT);
  pinMode(RPin,OUTPUT);
  pinMode(GPin,OUTPUT);
  pinMode(BPin,OUTPUT);

  t = rtc.getTime();
  file = SD.open(dataFile, FILE_WRITE);
  file.close();
  Serial.print("LOG");
  delay(100);
  digitalWrite(PieperPin,LOW);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(">------Online------<");
  lcd.setCursor(0, 2);
  lcd.print("   Starting Code");
  lcd.setCursor(0, 3);
  for(int x=0; x<20; x++){
    lcd.print("=");
    delay(100);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("<<-----Online----->>");
  lcd.setCursor(0, 1);
  lcd.print("     >Running<");
  lcd.setCursor(0, 2);
  lcd.print(" B1   B2   B3   B4");
  lcd.setCursor(0, 3);
  lcd.print("Data|Syst|Info|OFF");
  digitalWrite(RPin, LOW);
}

void loop(){
 
  t = rtc.getTime();
  temp = float(dht.readTemperature());
  moisture = int((1 -(float(analogRead(WatersensorPin))) / 1024) * 100);
  humidity = float(dht.readHumidity());
  light = int((float(analogRead(PhotoresistorPin)) / 1024) * 100);

  if (isnan(dht.readTemperature()) || isnan(dht.readHumidity())){
    alarm();
    dhtSensorWorking = false;
    }
  else {dhtSensorWorking = true;} 

  controll();

  poti();
  
  repToSerial();
  
if(!(digitalRead(ButtonOnePin))){
  buttonFeedback();
  repToLcdSensor();
   }        
if(!(digitalRead(ButtonTwoPin))){
  buttonFeedback();
  repToLcdStatus();
   }
if(!(digitalRead(ButtonThreePin))){
  buttonFeedback();
  repToLcdInfo();
}
if(!(digitalRead(ButtonFourPin))){
  buttonFeedback();
  LcdOff();
}
 
  repToSD();
  
  SystCheck();

}

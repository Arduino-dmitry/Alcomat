
int impulseLitr = 10000; // Количество имульсов на 1 литр
int impulseUAH = 5; // Количество гривен на 1 имульс
int costLitr = 75; // Цена за 1 литр
int startMoney = 5; // От какой суммы разрешать наливать
int allDistance = 100; // Высота бочки в сантиметрах

String phoneSMS = "+380681234567"; // Номер телефона для отправки СМС

int ReleOn = 1; // Значение порта для включение реле
int ReleOff = 0; // Значение порта для выключения реле

int ReleNasosPin = 6; // Пин контроллера для реле контроллера
int ReleKlapanAlcoholPin = 7; // Пин контроллера для реле клапана алкоголя
int ReleKlapanAirPin = 8; // Пин контроллера для реле клапана воздуха
int ReleMoneyPin = 9; // Пин контроллера для реле блокировки купюроприемника
int ButtonStartPin = 10; // Пин контроллера для кнопки СТАРТ


int countAlcohol; // Cчетчик имульсов с датчика протока
int countMoney; // Счетчик имульсов с купюроприемника
int alcoholGram; // Значение грамм за сумму
int cuurentMoney; // Значение денег в UAH
int currentDistance; // Текущий остаток в бочке
char lcd_buffer[16]; // Буфер для дисплея
char sms_buffer[64]; // Текст смс для отправки

////////////////////////Ультразвуковой датчик////////////////////////////////////
#include <Ultrasonic.h>
Ultrasonic ultrasonic(4,5); // (Trig PIN,Echo PIN) Ультразвуковой дальномер HR-SC04
//currentDistance = ultrasonic.Ranging(CM); // Получаем дистанциию в сантиметрах
/////////////////////////////////////////////////////////////////////////////////

////////////////////////////LCD I2C Дисплей/////////////////////////////////////
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

////////////////////////////////////////////////////////////////////////////////


void setup()
{
  pinMode(ReleNasosPin, OUTPUT);
  pinMode(ReleKlapanAlcoholPin, OUTPUT);
  pinMode(ReleKlapanAirPin, OUTPUT);
  pinMode(ReleMoneyPin, OUTPUT);
  pinMode(ButtonStartPin, INPUT);
  
  digitalWrite(ReleNasosPin, ReleOff);
  digitalWrite(ReleKlapanAlcoholPin, ReleOff);
  digitalWrite(ReleKlapanAirPin, ReleOff);
  digitalWrite(ReleMoneyPin, ReleOff);

  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(ButtonStartPin, HIGH);
  
  attachInterrupt(0, interruptAlcohol, FALLING);
  attachInterrupt(1, interruptMoney, FALLING);
  
  Serial.begin(9600);
  
  initGSM();
  lcd.init();
  delay(500);
  lcd.backlight();

  countAlcohol = 0;
  countMoney = 0;
  alcoholGram = 0;
}

void loop()
{
  cuurentMoney = countMoney * impulseUAH;
  alcoholGram = getVolumeGram(cuurentMoney);

  lcd.clear();
  lcd.setCursor(0,0);
  sprintf(lcd_buffer, "Money: %d UAH",cuurentMoney);
  lcd.print(lcd_buffer); 
  lcd.setCursor(0,1);
  sprintf(lcd_buffer, "Alcohol: %d ml.",alcoholGram);
  lcd.print(lcd_buffer); 
  

  if(countMoney >= startMoney) {
    
      if(digitalRead(ButtonStartPin) == 0) {
          
          getAlcohol(getVolumeImpulse(cuurentMoney)); // Наливаем алкоголь
          
          currentDistance = allDistance - ultrasonic.Ranging(CM); // Получаем остаток в бочке
          sprintf(sms_buffer, "Summa: %d UAH, Ostatok: %d CM",cuurentMoney,currentDistance); // Формируем СМС
          
          sendSMS(sms_buffer, phoneSMS); // Отправляем СМС
          countMoney = 0; // Сбрасываем количество денег
      } 
  }
  
  delay(500);

}
 
void interruptAlcohol()
{
  static unsigned long millis_prev;
  if(millis()-100 > millis_prev) {
      countAlcohol++;
  }    
  millis_prev = millis();
}

void interruptMoney()
{
  static unsigned long millis_prev;
  if(millis()-100 > millis_prev) {
      countMoney++;
  }    
  millis_prev = millis();
}


////Функция получение грамм за сумму
int getVolumeGram(int Money)
{
  int impulseGram = impulseLitr / 1000;
  int impulseOneCost = impulseLitr / costLitr;
  int result = impulseOneCost * Money / impulseGram;

  return result;
}

////Функция получение количества импульсов за сумму
int getVolumeImpulse(int Money)
{
  int impulseOneCost = impulseLitr / costLitr;
  int result = impulseOneCost * Money;

  return result;
}

////Функция по наливанию алкоголя
void getAlcohol(int impulseAlcohol)
{
  countAlcohol = 0; // Сброс счетчика импульсов алкоголя
  
  digitalWrite(ReleMoneyPin, ReleOn); // Блокировка купюроприемника
  delay(500);
  digitalWrite(ReleKlapanAirPin, ReleOn); // Открываем клапан воздуха
  delay(100);
  digitalWrite(ReleNasosPin, ReleOn); // Включаем насос
  delay(200);
  digitalWrite(ReleKlapanAlcoholPin, ReleOn); // Открываем клапан алкоголя

  while(countAlcohol < impulseAlcohol)
  {
    //Ожидаем пока нальет
  }
  
  digitalWrite(ReleNasosPin, ReleOff);
  digitalWrite(ReleKlapanAlcoholPin, ReleOff);
  digitalWrite(ReleKlapanAirPin, ReleOff);
  digitalWrite(ReleMoneyPin, ReleOff);  
}


////Функция инициализации модема
void initGSM()
{
  Serial.println("AT+CLIP=1");  //включаем АОН
  delay(100);
  Serial.println("AT+CMGF=1");  //режим кодировки СМС - обычный (для англ.)
  delay(100);
  Serial.println("AT+CSCS=\"GSM\"");  //режим кодировки текста
  delay(100);  
}

////Функция отправки СМС
void sendSMS(String text, String phone)  
{
  Serial.println("AT+CMGS=\"" + phone + "\"");
  delay(500);
  Serial.print(text);
  delay(500);
  Serial.print((char)26);
  delay(1000);
}

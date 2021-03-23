#include <Arduino.h>

// Repository Name on GitHub: MotorDoor

// https://arduino-kit.ru/product/nodemcu-motor-shield-dlya-wi-fi-modulya-esp8266-esp-12e

//    GPIO
// D3 – 0 motor A direction (направление);
// D1 – 5 motor A speed (скорость PWM);
// D4 – 2 motor B direction (направление);
// D2 – 4 motor B speed (скорость PWM).

#define PIN_BUTTON 12				// кнопка подключена сюда (PIN --- КНОПКА --- GND)

// подключение библиотек
#include <ESP8266WiFi.h> 
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ds18b20.h>
#include <GyverButton.h>
#include <NTPClient.h>

#include <WiFiUdp.h>       // это для времени
#include <ArduinoOTA.h>    // это для времени

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
#define MAXSTR 10          // максимальное количество строк
int Index = 0;             // текущий номер строки
int IndexT = 0;            // временный номер строки
String sss[MAXSTR];        // массив строк
String sssT;               // временная строка

GButton butt1(PIN_BUTTON);
// GButton butt1(PIN_BUTTON, HIGH_PULL, NORM_OPEN); // можно инициализировать так
int value = 0;
bool openedA = false;     // начальное состояние двери A (false = закрыта)
bool openedB = false;     // дверь B
String mode = "TEM";      // режим по умолчанию ручной (TEM - по температуре, MAN - ручной)
bool manual = false;      // мануальный режим - 3-ой клик
bool WiFiOn = false;        // пока не подключились к WiFi = false


#ifndef STASSID
#define STASSID "GeKa"
#define STAPSK  "12344321"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// создание сервера 
ESP8266WebServer server(80);

// глобальные переменные для работы с ними в программе
int ds_int = 2;   // интервал замера ds18b20 в секундах 
float TemA = 26.0;     // температура ON/OFF двери A
float TemB = 25.0;     // температура ON/OFF двери B
float TemH = 1.0;      // гестерезис

String temp = "<meta charset='utf-8'><div style='width:100%; border:solid; font-size:200%;'><center><p>Температура: ";
// const char *form = " "; // так было в оригинале
const String form1 = "  <style> button { width:100;height:50 } input { width:110; font-size:90% }</style>"
   "<center><form action='/' <p>"
   "<button name='dir' type='submit' value='1'>Open A</button>&nbsp;"
   "<button name='dir' type='submit' value='2'>Open B</button><p>"
   "<button name='dir' type='submit' value='3'>Stop A</button>&nbsp;"
   "<button name='dir' type='submit' value='4'>Stop B</button><p>"
   "<button name='dir' type='submit' value='5'>Close A</button>&nbsp;"
   "<button name='dir' type='submit' value='6'>Close B</button><p>";
const String form1b = "&nbsp;<button name='dir' type='submit' value='7'>Изменить</button><p>"
   "Температурный порог<br>открывания/закрывания дверей:<p>"
   "Дверь A: <input type='text' name='TemA' value='";
const String form2 = "'>&nbsp;"
   "Дверь B: <input  type='text' name='TemB' value='";
const String form3 = "'><br>Гестерезис: +/- <input type='text' name='TemH' value='";
const String form4 = "'>°C";
const String form5 =  "</form></center></div>";  

//--------------------
bool activeA = true;    // дверь A
bool activeB = true;    // дверь B
bool trigger = true;    // если требуется сброс счётчиков (задержки останова моторов)

// Открыть дверь A
void openA(void) {
   //   analogWrite(D1, 1023); digitalWrite(D3, LOW);
     analogWrite(D1, 1023); analogWrite(D3, LOW);
     openedA = true;
     activeA = true;
}  
// Открыть дверь B
void openB(void) {
   //   analogWrite(D2, 1023); digitalWrite(D4, LOW); 
     analogWrite(D2, 1023); analogWrite(D4, LOW); 
     openedB = true;
     activeB = true;
}  
// Закрыть дверь A
void closeA(void) {
   //   analogWrite(D1, 1023); digitalWrite(D3, HIGH);
     analogWrite(D1, LOW); analogWrite(D3, 1023);
     openedA = false;
     activeA = true;
}  
// Закрыть дверь B
void closeB(void) {
   //   analogWrite(D2, 1023); digitalWrite(D4, HIGH); 
     analogWrite(D2, LOW); analogWrite(D4, 1023); 
     openedB = false;
     activeB = true;
}  
// стоп A
void stopA(void) {     
   analogWrite(D1, 0); analogWrite(D3, 0);
}  
// стоп B
void stopB(void) {     
   analogWrite(D2, 0); analogWrite(D4, 0);
}  
//-------------------------------------------------
// процедура подготовки одной строки (массива строк)
void timeMark(String ss) {
   sss[Index++] = "<p>" + timeClient.getFormattedTime() + " " + ss + " " + String(ds_tem, 1) + "°C" + "</p>";
   if (Index==MAXSTR) Index = 0;
}
// смена режима
void changeMode(void) {     
    String ss = "Holded ";
    ss = ss + (manual ? "-" : "+");
    Serial.println(ss);
    manual = !manual;
    if (manual)
     {
        mode = "MAN";
     } else
     {
        mode = "TEM";
     }
}  

void handle_form() {     
     // при переходе по форме    
     if (server.arg("dir"))  {
         // получить значение параметра формы "dir"
         int direction = server.arg("dir").toInt();
         // mode = "TEM";     // нажатие на любую кнопку WEB - переводит в АВТО-режим
         // выбор для кнопок         
         switch (direction)   {
             case 1:  openA(); timeMark("Открыта А "+mode);
                  break;             
             case 2:  openB(); timeMark("Открыта B "+mode);
                 break;
             case 3:  stopA();
                 break;
             case 4:  stopB();
                break;             
             case 5:  closeA(); timeMark("Закрыта А "+mode);
                break;         
             case 6:  closeB(); timeMark("Закрыта B "+mode);
                break;          
             case 7:  changeMode(); 
                break;         }           
         // пауза         
         delay(300);     
      }
      if (server.arg("TemA") != "0.0")  {
         // получить значение "TemA"
         TemA = server.arg("TemA").toFloat();
      }
      if (server.arg("TemB") != "0.0")  {
         // получить значение "TemB"
         TemB = server.arg("TemB").toFloat();
      }
      if (server.arg("TemH") != "0.0")  {
         // получить значение "TemH"
         TemH = server.arg("TemH").toFloat();
      }
      Serial.println(String(TemA) + " / " + String(TemB) + " +/-" + String(TemH) + " / " + mode);
      // подготовка строк для браузера
      IndexT = Index;      // берём текущее значение
      sssT = "";
      for (int i = 0; i < MAXSTR; i++)
      {
         (IndexT == 0) ? IndexT = MAXSTR-1 : --IndexT;
         sssT += sss[IndexT];
      }
      
  // отправка формы в браузер
  server.send(200, "text/html",
   temp + String(ds_tem, 1) + "°C / " + timeClient.getFormattedTime() + " V1.1<p>"
   + form1 + (openedA ? "открыта":"закрыта") + "&nbsp;&nbsp;" + (openedB ? "открыта":"закрыта")
   + "<p>Режим: " + ((mode == "MAN") ? "РУЧНОЙ" : "АВТОМАТ") + form1b
   + String(TemA, 1) + form2 + String(TemB, 1) + form3
   + String(TemH, 1) + form4 + sssT + form5
   ); 
}   
void setup() {
  Serial.begin(115200);
     // подсоединение к WiFi роутеру
     // задайте свои значения для essid и passphrase
// Port defaults to 8266
// ArduinoOTA.setPort(8266);

// Hostname defaults to esp8266-[ChipID]
// ArduinoOTA.setHostname("myesp8266");

// No authentication by default
// ArduinoOTA.setPassword("admin");

  Serial.write("Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
    // подсоединениет
    int i = 30;         // ждём не более 30 полусекунд (т.е. 15 секунд)
    while (WiFi.status() != WL_CONNECTED)  {
       if (i-- <= 0) break;
      Serial.print(".");
      delay(500);
    }
    if (i>0)
    {
     Serial.write("Connected!!!");
     Serial.println("");
     Serial.print("Connected to ");
     Serial.println(ssid);
     Serial.print("IP address: ");
     Serial.println(WiFi.localIP());

     if (MDNS.begin("esp8266")) {
       Serial.println("MDNS responder started");
     }

     // callback для http server
     server.on("/", handle_form);
     // запуск сервера
     server.begin();
     WiFiOn = true;
     timeClient.begin();
     timeClient.setTimeOffset(3*60*60);
    } else
    {
     Serial.write("No WiFi connected!!!");
    }

    pinMode(D1, OUTPUT); // motor A скорость
    pinMode(D2, OUTPUT); // motor B скорость
    pinMode(D3, OUTPUT); //  motor A направление
    pinMode(D4, OUTPUT); //  motor B направление
   //  pinMode(13, INPUT_PULLUP); // выключатель
   // часть setup(); -------------------------------------------------------- это для кнопки
//   butt1.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
//   butt1.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)
//   butt1.setClickTimeout(600);   // настройка таймаута между кликами (по умолчанию 300 мс)

  // HIGH_PULL - кнопка подключена к GND, пин подтянут к VCC (PIN --- КНОПКА --- GND)
  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
  butt1.setType(HIGH_PULL);

  // NORM_OPEN - нормально-разомкнутая кнопка
  // NORM_CLOSE - нормально-замкнутая кнопка
  butt1.setDirection(NORM_CLOSE);

  // OTA initializing
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  if (WiFiOn) {
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  for (int i = 0; i < MAXSTR; i++) { sss[i] = ""; }   // очищаем массив строк
} 
void handle_Door (int interval) {
    static unsigned long i;
    static unsigned int in;
    if (trigger) {i = millis(); in = interval; trigger = !trigger;}
    if(i + (in * 1000) > millis()) return; // если не пришло время - выходим из функции
    i = millis();
    in = interval;
    // всё, что ниже будет выполняться через интервалы
    if (activeA) { activeA = false; stopA();}
    if (activeB) { activeB = false; stopB();}
} 
void loop() {
  if (WiFiOn) {
   server.handleClient(); 
   ArduinoOTA.handle();
   timeClient.update();
   // Serial.println(timeClient.getFormattedTime());
  }
  handle_Door (20);  // цикл проверки состояни дверей
  ds_handle(ds_int); // цикл замера температуры ds18b20
  butt1.tick();  // обязательная функция отработки КНОПКИ. Должна постоянно опрашиваться
  if (butt1.isSingle()) {
    Serial.println("SINGLE");         // проверка на один клик
    Serial.print(mode +"->");
    if (!openedA) {
      openA();
      // mode = "MAN";     // 1-у нажатие на входную кнопку - переводит в РУЧНОЙ-режим
      Serial.println(mode);
    } else {
      closeA();
      // mode = "TEM";     // следующее нажатие на входную кнопку - переводит в АВТО-режим
      Serial.println(mode);
    }
    Serial.println(openedA ? "открыта":"закрыта");
  }
  if (butt1.isDouble()) {
    Serial.println("Double");
    if (!openedB) {
      openB();
    } else {
      closeB();
    }
    Serial.println(openedB ? "открыта":"закрыта");
   } 
  if (butt1.isHolded()) { // проверка на удержание +++
   changeMode();
  }                       // проверка на удержание ---
   // mode = "MAN";
   if (!manual) {
      if (ds_tem >= (TemA+TemH)) if (!openedA) {openA(); timeMark("Открыта А "+mode);}
      if (ds_tem <= (TemA-TemH)) if (openedA) {closeA(); timeMark("Закрыта A "+mode);}
      if (ds_tem >= (TemB+TemH)) if (!openedB) {openB(); timeMark("Открыта B "+mode);}
      if (ds_tem <= (TemB-TemH)) if (openedB) {closeB(); timeMark("Закрыта B "+mode);}
   }
}
/*//--------------------
// останов
void stop(void) {     
   analogWrite(D1, 0);     
   analogWrite(D2, 0); 
}  
// вперед 
void forward(void) {
     analogWrite(D1, 1023); analogWrite(D2, 1023);
     digitalWrite(D3, HIGH); digitalWrite(D4, HIGH); 
}  
// назад 
void backward(void) {
     analogWrite(D1, 1023); analogWrite(D2, 1023);
     digitalWrite(D3, LOW); digitalWrite(D4, LOW); 
}   
// влево
void left(void) {
     analogWrite(D1, 1023);analogWrite(D2, 1023);
     digitalWrite(D3, LOW);digitalWrite(D4, HIGH);
}   
// вправо
void right(void) {
     analogWrite(D1, 1023);analogWrite(D2, 1023);
     digitalWrite(D3, HIGH); digitalWrite(D4, LOW); 
}   
*/
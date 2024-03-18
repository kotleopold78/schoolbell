#include <WiFi.h>
#include <WiFiClient.h>
#include "RTClib.h"
#include <EEPROM.h>

const char* ssid     = "ESPD";
const char* password = "1";

const char* ssidap     = "Bell";
const char* passwordap = "12345678";

unsigned int ring[300];
unsigned int clocks,minuts;
unsigned long timer,timeer;
byte zvonok,nowifi;
byte regim=9;
unsigned int uu;
byte schedule;
byte nomschedule=1;
boolean onoff=1;
//byte bellsched=0;

RTC_DS1307 rtc;

WiFiServer server(80);
WiFiClient client;
IPAddress local_IP(10, 76, 78, 239);
IPAddress gateway(10, 76, 78, 1);
IPAddress subnet(255, 255,254, 0);
IPAddress myIP;



void setup() {
  Serial.begin(115200);
   if (! rtc.begin()) {
    Serial.println("нет модуля часов, работа невозможна");
    Serial.flush();
    while (1) delay(10);
  }
   if (! rtc.isrunning()) {
    delay(10000);
    Serial.println("Нет данных времени, осуществляем запись");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Время пишется на этапе компиляции, будет отставать
    // Можно внести вручную 2023 год, январь,21 число, три часа ровно
    // rtc.adjust(DateTime(2023, 1, 21, 3, 0, 0));
  }
int sch,cou,tem,tem1,tem2,coutemp,ccount;

 EEPROM.begin(600);
 //for (int rr=0;rr<600;rr++){
  //EEPROM.write(rr,255);
 //}
 //EEPROM.commit();
 // двойной цикл перебора для выгрузки значений из памяти в массив ( если значение больше 1500, значит расписание недоступно)
 for (sch=0;sch<10;sch++){
for (cou=1; cou<=30;cou++){
  coutemp=(60*sch+cou*2)-1;
 tem1= EEPROM.read(coutemp-1);
 tem2=EEPROM.read(coutemp);

 tem=(tem2<<8)+tem1;
 
 Serial.print(tem);
  Serial.print(" ");
   Serial.println(timedec(tem));
 if (tem>1500){
  ccount=cou;
  break;
 }else{
  ring[(30*sch+cou-1)]=tem;
 
      }
    }

if (ccount<2){
  if (sch==0){
    schedule=0;
  }else{
    schedule=sch;
  }
  break;
}
 }
 Serial.print ("всего расписаний:");
 Serial.println(schedule);
 EEPROM.end();
 
//пин реле, реле включено при низком уровне
pinMode(27,OUTPUT);
digitalWrite(27,HIGH);

//пытаемся подключить wi-fi

 /* if (!WiFi.config(local_IP, gateway, subnet)) { // разблокируем, если нужно жестко задать адрес в сети
    Serial.println("Не могу установить настройки сети");
  } */
  WiFi.begin(ssid, password);
   int i;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(WiFi.status());
        delay(500);
        i++;
        if (i>60){ 
          nowifi=1;
          break;
        }

}
//если не удается присоединится к сети, поднимаем свою точку доступа и дальше работаем с ней 
if (nowifi>0){
	WiFi.softAP(ssidap, passwordap);
  myIP = WiFi.softAPIP();
  Serial.println();
Serial.print("Внимание, поднимаю собственную точку доступа, подключитесь к сети ");
Serial.print(ssidap);
Serial.print(" с паролем ");
Serial.print(passwordap);
Serial.print(" и подключитесь по указанному адресу: ");

}else{
 myIP = WiFi.localIP();
  Serial.println();
Serial.print("Внимание подключаюсь к сети, подключитесь по указанному адресу: ");
}
Serial.print("IP адрес: ");
  Serial.println(myIP);
server.begin();

}

void loop() {
  client = server.available(); // обработка веба
 if (client) {
    webs();
  }
  if (millis()-timer>59999){ //отсчет минут
rings();
      timer=millis();
       if (nowifi==0){
        if (WiFi.status() != WL_CONNECTED)  {
       
      Serial.println("reconnect:");
    WiFi.disconnect();
    WiFi.reconnect();
        }
  }
  }
  
  if ((zvonok==10)&&(millis()-timeer>5000)){ //отключение звонка
    digitalWrite(27,HIGH);
    zvonok=0;
    Serial.println("call stop");
  }

}
void webs(){
  String web;
  boolean sec=1; //пароль отключен, установите в 0, если нужен запрос пароля
  byte getw=0;
  int i;
  DateTime now = rtc.now();
Serial.println("новое соединение");
    
    while (client.connected()) {
      uu=client.available();
      if (uu) {
        char c = client.read();
        Serial.write(c);
        web+=c;
       
        if (c == '\n') {
          //обработка строк ответа
          if (web.lastIndexOf(F("Authorization: Basic YWRtaW46YWRtaW4="))>-1) sec=1; //admin admin
          if (web.lastIndexOf(" /reg1.html")>-1) getw=1;
         if (web.lastIndexOf(" /reg2.html")>-1) getw=2;
         if (web.lastIndexOf(" /reg3.html")>-1) {
          //удаление последнего расписания
          schedule--;
          if (schedule<0) schedule=0;
           EEPROM.begin(600);
           Serial.print ("в ячейке");
           Serial.println(EEPROM.read(60*schedule));
           Serial.println(EEPROM.read(60*schedule+1));
           Serial.println("");
           EEPROM.write (60*schedule+1,255);
           Serial.print ("стираем ячейку");
           Serial.println(60*schedule);
           EEPROM.commit();
           getw=3;
         }
          web="";
        }
          if (uu<=1){
 if (sec==0){
 client.println(F("HTTP/1.0 401 Unauthorized"));
            client.println(F("WWW-Authenticate: Basic realm=\"schoolbell\""));
            
             break;
            }else{
              
            //Перед выдачей данных происходит обработка post запроса
               i=web.lastIndexOf("editg="); //внесение времени в часы
          if (i>-1) {
            Serial.println();
                      
            String strr=web.substring(i+6,i+10);
            
            int god=strr.toInt();
           strr=web.substring(i+11,i+13);
            int mes=strr.toInt();
             strr=web.substring(i+14,i+16);
            int dayy=strr.toInt();
            
            strr=web.substring(i+23,i+25);
            int chas=strr.toInt();
            strr=web.substring(i+28,i+30);
            int minu=strr.toInt();
            rtc.adjust(DateTime(god, mes, dayy, chas, minu, 0));
          }
           i=web.lastIndexOf("cron0=");//если есть новое расписание
           
          if (i>-1) {
            EEPROM.begin(600);
             
             Serial.println("");
            int itog,dobav,schedin;
            schedin=schedule;
            if (schedin>9) schedin=9;
          for (int schd=0;schd<30; schd++){
            i=web.lastIndexOf("cron"+String(schd)+"=");
            if (schd>9){
               dobav=1;
            }else{
              dobav=0;
            }
           String strr=web.substring(i+6+dobav,i+8+dobav);
          
            int chas=strr.toInt();
            strr=web.substring(i+11+dobav,i+13+dobav);
             int minu=strr.toInt(); 
             itog=chas*60+minu;
            Serial.print("расписание ");
            Serial.print(schd);
            Serial.print(" Время");
             Serial.println(itog);
             if (itog==0) itog=1500;
             //Serial.println(itog);
             ring[30*schedule+schd]=itog;
             EEPROM.write(schd*2+60*schedin,lowByte(itog));
             EEPROM.write(schd*2+60*schedin+1,highByte(itog));
          }
          EEPROM.commit();
          schedule++;
          }
 if (web.lastIndexOf("callon=9")>-1){//режим звонка
            regim=9;
          }
          
          if (web.lastIndexOf("callon=0")>-1){
            regim=0;
          }
          i=web.lastIndexOf("raspis=");//выбор расписания
           
          if (i>-1) {
            String strr=web.substring(i+7,i+8);
             nomschedule=strr.toInt();
             Serial.print ("new scdsed");
             Serial.println(nomschedule);
           
          }
          
          // отправка заголовка странички, после заголовка обязательно пустая строка
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  
          client.println(); //Этот блок лучше не трогать, браузер скажет большое спасибо
          
          client.println("<!DOCTYPE HTML>"); // начало веб выдачи
          client.println("<html><meta charset='utf-8'>");
		  if (getw==0){
      client.print("Звонок автоматический к вашим услугам. Сейчас ");
      client.print (now.hour());
      client.print(":");                                                 
     client.print (now.minute());
          client.print("<form name='s' action method='post'>");
          client.println("Всего расписаний: "); 
          client.println(schedule); 
          for (int nom=0;nom<schedule;nom++){
           client.println("<table border=1><tr>");
          
          for (int y=0;y<30;y=y+2){
          client.println("<td>");
          client.println(timedec(ring[30*nom+y]));
          client.println("</td>");   
          }
          client.println("</tr><tr>");
            for (int y=1;y<30;y=y+2){
            client.println("<td>");
          client.println(timedec(ring[30*nom+y]));
          client.println("</td>");    
          }
client.println("</tr></table>"); 
          client.println("<input name=raspis type=radio value=");
          client.println(nom+1);
          if (nom+1==nomschedule) client.println("checked=checked");
          client.println(">Расписание №");
          client.println(nom+1);
            
          }
            client.println("<br><br><fieldset><legend>Режим звонка</legend>");
            //client.println("<input name="callon" type="radio" value="9"> Звонок включен<br><input name=callon" type="radio" value=0 > Звонок выключен<br></fieldset>");
          if (regim==0) {
            client.println("<input name=callon type=radio value=9> Звонок включен<br><input name=callon type=radio value=0 checked=checked> Звонок выключен<br></fieldset>");
          }
          if (regim==9) {
            client.println("<input name=callon type=radio value=9 checked=checked> Звонок включен<br><input name=callon type=radio value=0 > Звонок выключен<br></fieldset>");
          }
          client.println("<input type='submit' value='Внести изменения'></form>");
          client.println("<br><br><fieldset><legend>Настройка звонкового автомата</legend>");
           
            client.print("<a href=/reg1.html>Добавить расписание.</a> <br>");
            client.print("<a href=/reg2.html>Править внутреннее время.</a> <br>");
            client.print("</fieldset>");
            client.println("<br><br><fieldset><legend>Опасный режим - удаление расписания</legend>");
            client.print("<a href=/reg3.html>Удалить последнее расписания.</a> <br>");
            client.print("</fieldset>");
          client.println("</html>"); //окончание
          break;
        }
		if (getw==1){
			client.print("Ввод нового расписания № ");
			client.print(schedule+1);
		client.print("<br>Если вам не нужны звонки установите время в 00:00<form name='times' action='/' method='post'><table border=1><tr>"); 
for (int i=0;i<15;i++){
client.print("<td>Урок ");
client.print(i+1);
client.print("</td>");
}	
client.print("</tr><tr>");
int ee=0;
for (int i=0;i<30;i=i+2){
client.print("<td><input type=time name=cron");
client.print(i);
client.print(" value=");
client.print(timedec(510+55*ee));
ee++;
client.print("></td>");
}

client.print("</tr><tr>");
for (int i=0;i<15;i++){
client.print("<td>Перемена");
client.print(i+1);
client.print("</td>");
}

client.print("</tr><tr>");
ee=0;
for (int i=1;i<30;i=i+2){
client.print("<td><input type=time name=cron");
client.print(i);
client.print(" value=");
client.print(timedec(555+55*ee));
ee++;
client.print("></td>");
}

client.println("</tr></table><br><input type='submit' value='Внести'></form></html>");
break;
		}
   if (getw==2){
     client.print("Ввод времени ");
     
    client.print("<br><form name='times' action='/' method='post'>"); 

client.print("Дата<input type=date name=editg>");

client.print("<br>Время<input type=time name=editt>");




client.println("<br><input type='submit' value='Внести'></form></html>");
break;
    }
      if (getw==3){
     client.print("Расписание удалено  ");
     
   client.print("<br><form name='del' action='/' method='post'>"); 

client.print("Если вы сделали это случайно, мне вас очень жаль!");
client.println("<br><input type='submit' value='Жми, все уже сделано'></form></html>");
break;
    }
			}
      }
    
      }
    }
   web="";
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("отключение");
  
}

void rings(){
    int timen;
//watch.gettime();    
DateTime now = rtc.now();                                             
   int  W = now.dayOfTheWeek();
    int d= now.day();
     int M= now.month();
      int ye= now.year();                                            
     int h = now.hour();                                                 
     int m = now.minute();
      timen=(h*60)+m;
      //Serial.println(timen);
      Serial.println(timedec(timen));
      if (onoff){
      if (W>0&&W<6){
      for (int i=1; i<=30; i++){
        if (timen==ring[(30*(nomschedule-1)+i)]){
          
            if (regim==9){
            digitalWrite(27,LOW);
            timeer=millis();
            zvonok=10;
            Serial.println("call start");
            }
          }
          
        }
      }
  }
}

String timedec(int enc){
  String ti;
  char buff[3];
clocks=enc/60;
minuts=enc%60;
itoa(clocks,buff,10);

ti=String(buff);
if (clocks<10) ti="0"+ti;
itoa(minuts,buff,10);
if (minuts<10) {
  ti=ti+":0"+String(buff);
}else{
ti=ti+":"+String(buff);
}
return ti;
}

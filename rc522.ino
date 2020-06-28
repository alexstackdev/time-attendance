#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"
#include <time.h>
#include <WebSocketsServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DS1302.h>
#include "TM1637.h"
#include <Ticker.h>
#include <DNSServer.h>

const int CLK = 10;
const int DIO = 16;
const byte DNS_PORT = 53;
TM1637 display(CLK, DIO);
MFRC522 rfid(15, 5);
DS1302 rtc(4, 5, 2);
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer server (80);
DNSServer dnsServer;
IPAddress apIP(192, 168, 1, 1);
WebSocketsServer webSocket = WebSocketsServer(81);
Ticker ledTime, wifiAP, showPass;
uint8_t numTemp;
unsigned long idCardRecoTemp;
int checkDot = 0, inGetIDcard = 0, timeCountDown = 0;
String inputString = "";
String verESP = "", creESP = "", timeESP = "", nameWifiESP = "", passWifiESP = "", passWebESP = "";
int cardCount = 0, showPassWifi = 0;
boolean stringComplete = false;




/*
   Num of type:
   1 -> date by Epoch
   2 -> card id by unsigned long
   3 -> new id employess and card id for employess.   ex: #11034586783457  #1 for set id and id card------103 for id to set----------4586783457 for id card of employess
   3.1 -> id or card id already have in database.   ex: !3.1
   3.2 -> already set name for id.   ex: !3.1101name_of_id
   4 -> all list id, name, card list of id.    ex: ?4
   5 -> delete cardID from all database. ex:  #5234623456
   6 -> get info for Home tab.   ex: ?6
   7 -> change name of id.    ex: #7111ThisIsName
   8 -> get salary log by month/year of id.    ex: ?8111012019     ?8 is querry, 111 is id of employess, 012019 is month and year. month 01 year 2019
   9 -> set wifi config.  ex: #9thisiswifiname!thisispass!thisistimeout
   . -> password for web control.    ex: ?.


   Syntax:
   # for set data
   ? for get data
   ! for responese data


   Num of setData to Seerver:
    1 -> set time to RTC in server by Epoch time. ex: #1#1554303811





   ------------------
   Num of get Data from Server to Client:
    1 -> gett time from RTC in server by Epoch time to Client. ex: ?1?


*/


time_t getEpoch(int gio, int phut, int giay, int ngay, int thang, int nam) {
  struct tm  ptm;
  time_t t;
  memset(&ptm, 0, sizeof(tm));
  ptm.tm_year = nam - 1900;
  ptm.tm_mon = thang - 1;
  ptm.tm_mday = ngay;
  ptm.tm_hour = gio;
  ptm.tm_min = phut;
  ptm.tm_sec = giay;
  t = mktime(&ptm);
  return t;
}
int getNgay(long epoch) {
  time_t t = epoch;
  struct tm * ptm;
  ptm = gmtime(&t);
  return ptm->tm_mday;

}
int getThang(long epoch) {
  time_t t = epoch;
  struct tm * ptm;
  ptm = gmtime(&t);
  return ptm->tm_mon + 1;
}
int getNam(long epoch) {
  time_t t = epoch;
  struct tm * ptm;
  ptm = gmtime(&t);
  return ptm->tm_year + 1900;
}
int getGio(long epoch) {
  time_t t = epoch;
  struct tm * ptm;
  ptm = gmtime(&t);
  return ptm->tm_hour;
}
int getPhut(long epoch) {
  time_t t = epoch;
  struct tm * ptm;
  ptm = gmtime(&t);
  return ptm->tm_min;
}
int getGiay(long epoch) {
  time_t t = epoch;
  struct tm * ptm;
  ptm = gmtime(&t);
  return ptm->tm_sec;
}
void showTime(unsigned long t) {
  time_t tt = t;
  Serial.println(ctime(&tt));
}
int getDayInMonth(int y, int yer) {
  if (y == 1 || y == 3 || y == 5 || y == 7 || y == 8 || y == 10 || y == 12) return 31;
  else if (y == 4 || y == 6 || y == 9 || y == 11) return 30;
  else if (y == 2) {
    if (yer % 4 == 0) return 29;
    else return 28;
  } else return 0;

}
void setTimeRTC(int a, int b, int c, int d, int e, int f, unsigned long unix = 0) {
  String text = "";
  if (unix == 0) {
    rtc.halt(false);
    rtc.writeProtect(false);
    //rtc.setDOW(FRIDAY);
    rtc.setTime(a, b, c);
    rtc.setDate(d, e, f);
    text = "Already set rtc time to" + String(a) + ":" + String(b) + ":" + String(c) + "   " + String(d) + "/" + String(e) + "/" + String(f) + "\n";
    Serial.println(text);
  } else {
    rtc.halt(false);
    rtc.writeProtect(false);
    //rtc.setDOW(FRIDAY);
    rtc.setTime(getGio(unix), getPhut(unix), getGiay(unix));
    rtc.setDate(getNgay(unix), getThang(unix), getNam(unix));
    text = "Already set rtc time to" + String(getGio(unix)) + ":" + String(getPhut(unix)) + ":" + String(getGiay(unix)) + "   " + String(getNgay(unix)) + "/" + String(getThang(unix)) + "/" + String(getNam(unix)) + "\n";
  }
  Serial.println(text);
}
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
unsigned long printDec(byte *buffer, byte bufferSize) {
  unsigned long uidDec, uidDecTemp;
  uidDec = uidDecTemp = 0;
  for (byte i = 0; i < bufferSize; i++) {
    uidDecTemp = buffer[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }
  Serial.println(uidDec);
  return uidDec;
}
void showCardInfo() {
  //Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  //Serial.println(rfid.PICC_GetTypeName(piccType));
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  Serial.print(F("The NUID tag is:   "));
  idCardRecoTemp = printDec(rfid.uid.uidByte, rfid.uid.size);
  String idTemp = String(idCardRecoTemp);
  for (int i = 1; i <= 4; i++)  display.display(i - 1, String(idTemp[idTemp.length() - 5 + i]).toInt());
  display.point(POINT_OFF);

}
void showTimeNow(String firstString, unsigned long epochs, int n = 1) {
  Serial.print(firstString);
  Serial.print((epochs  % 86400L) / 3600);
  Serial.print(':');
  if (((epochs % 3600) / 60) < 10) {
    Serial.print('0');
  }
  Serial.print((epochs  % 3600) / 60);
  Serial.print(':');
  if ((epochs % 60) < 10) {
    Serial.print('0');
  }
  if (n == 1) Serial.println(epochs % 60);
  else Serial.print(epochs % 60);
}
unsigned long timeUnixNow() {
  Time tTemp = rtc.getTime();
  return getEpoch(tTemp.hour, tTemp.min, tTemp.sec, tTemp.date, tTemp.mon, tTemp.year);

}
void printFile(String filename) {
  String name = filename;
  name.remove(name.length() - 1, 1);

  File file = SPIFFS.open(name.c_str(), "r");
  if (!file) {
    Serial.println(F("Failed to read file"));
    file.close();
  } else {
    Serial.println("-------------------------START FILE---------------------------");
    while (file.available()) {
      Serial.println(file.readStringUntil('\n'));
    }
    Serial.println("-------------------------END FILE-------------------------");
  }

  file.close();

  Serial.println();

}
void printListFile() {
  String str = "";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    str += dir.fileName();
    str += " / ";
    str += dir.fileSize();
    str += "\r\n";
  }
  Serial.print(str);
}
void deleteAllTrackingOfId(String id){
  String str = "";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    str = dir.fileName();
    String temp="";
    temp += String(str[str.length()-7]);
    temp += String(str[str.length()-6]);
    temp += String(str[str.length()-5]);
    if(temp.toInt()>99&&temp.toInt()<1000&&temp==id){
      Serial.print("found id "); Serial.println(temp);
      if(SPIFFS.remove(str.c_str()))  Serial.println("Deleted salary tracking");
    }
  }
}
bool Checkrc522() {
  int boo = 0;
  Serial.println(F("*****************************"));
  Serial.println(F("MFRC522 Digital self test"));
  Serial.println(F("*****************************"));
  rfid.PCD_DumpVersionToSerial();  // Show version of PCD - MFRC522 Card Reader
  Serial.println(F("-----------------------------"));
  Serial.println(F("Only known versions supported"));
  Serial.println(F("-----------------------------"));
  Serial.println(F("Performing test..."));
  bool result = rfid.PCD_PerformSelfTest(); // perform the test
  String resp = "";
  Serial.println(F("-----------------------------"));
  Serial.print(F("Result: "));
  if (result) {
    Serial.println(F("OK"));
    resp = "OK";
    boo = 1;
  } else {
    Serial.println(F("DEFECT or UNKNOWN"));
    resp = "DEFECT or UNKNOWN";
    boo = 0;
  }
  Serial.println();
  resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
  server.send ( 200, "text/html", resp.c_str());
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  return boo;

}
void ResetESP() {
  delay(500);

  ESP.restart();
}
void GetSalaryList() {
  Serial.println(ESP.getFreeHeap());
  if (server.arg("idmem") == "") {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  String qr = "";
  unsigned long t = timeUnixNow();
  if (server.arg("thang") == "" || server.arg("nam") == "")  qr = "/" + String(getNam(t)) + "/" + String(getThang(t)) + "/" + String(server.arg("idmem")) + ".txt" ;
  else qr = "/" + String(server.arg("nam")) + "/" + String(server.arg("thang")) + "/" + String(server.arg("idmem")) + ".txt" ;
  Serial.println(qr);

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  String resp = "<h1 style='text - align: center; '>List Salary Tracking of ID Mem";

  resp += String(server.arg("idmem"));
  resp += "</h1>\
                <h3>&nbsp;</h3>\
                <table style='width: 245px; ' border='1' cellspacing='1' cellpadding='1'>\
                <tbody>\
                <tr>\
                <td style='width: 121px; '>Time in Sec</td>\
                <td style='width: 115px; '>Time in Normal</td>\
                </tr>";
  server.send ( 200, "text/html", resp.c_str());
  int rec_c = 0;
  String c = "";
  File f = SPIFFS.open(qr.c_str(), "r+");
  if (!f) {
    Serial.println("Cant open salary tracking to read");
    return;
  }

  while (f.available()) {
    t = strtoul(f.readStringUntil('\n').c_str(), NULL, 0);
    resp = "<tr><td>";
    resp += String(t);
    resp += "</td><td>";
    resp += String("Day " + String(getNgay(t)) + "  " + String(getGio(t)) + ":" + String(getPhut(t)) + ":" + String(getGiay(t)));
    resp += "</td></tr>";
    server.sendContent(resp);
    webSocket.broadcastTXT(resp);
    rec_c++;
  }
  f.close();

  resp = "</tbody></table><br>Number of records: ";
  resp += String(rec_c) + "<br>";
  resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
  server.sendContent(resp);

}
void addCard(String idIn, String cardIn, String name) {
  File f = SPIFFS.open("/cardList.txt", "a+");
  if (!f) {
    Serial.println("file open failed");
    return;
  }

  if ( f.size() == 0) {
    if (idIn == "000") idIn = "100";
    Serial.println(F("No card stored in chip"));
    f.print(idIn); f.print("-"); f.println(cardIn);
    f.close();
    Serial.println(String("Added " + idIn + " with " + cardIn));
    cardCount = 1;
    if (name != "") {
      if (SPIFFS.exists(String("/idMem/" + idIn + "/info.txt").c_str())) {
        SPIFFS.remove(String("/idMem/" + idIn + "/info.txt").c_str());
      }
      File f1 = SPIFFS.open(String("/idMem/" + idIn + "/info.txt").c_str(), "a+");
      if (!f1) {
        Serial.println("open file for create/edit name failed");
        return;
      }
      Serial.println(String("Set employess with name is " + name));
      webSocket.broadcastTXT(String("!3.2" + idIn + name).c_str());
      f1.println(name);
      f1.close();
    }
  } else {
    int randomID = 000, ranCheck = 0, row = 0, i = 0;
    while (f.available()) {
      row++;
      String t1 = f.readStringUntil('\n');
    }
    f.seek(0, SeekSet);
    Serial.println(row);
    String idList[row]; String cardList[row];
    i = 0;
    while (f.available()) {
      idList[i] = f.readStringUntil('-');
      cardList[i] = f.readStringUntil('\n');
      i++;
    }
    if (idIn == "000") {
      while (ranCheck == 0) {
        randomID = random(120, 999);
        for (int a = 0; a < row; a++) {
          if (randomID == String(idList[a]).toInt()) {
            ranCheck = 0;
            continue;
          }
        }
        ranCheck = 1; break;
      }
      idIn = String(randomID);
    }
    if (name != "") {
      if (SPIFFS.exists(String("/idMem/" + idIn + "/info.txt").c_str())) {
        SPIFFS.remove(String("/idMem/" + idIn + "/info.txt").c_str());
      }
      File f1 = SPIFFS.open(String("/idMem/" + idIn + "/info.txt").c_str(), "a+");
      if (!f1) {
        Serial.println("open file for create/edit name failed");
        return;
      }
      Serial.println(String("Set employess with name is " + name));
      webSocket.broadcastTXT(String("!3.2" + idIn + name).c_str());
      f1.println(name);
      f1.close();
    }
    for (int c = 0; c < row; c++) {
      Serial.println(String(idList[c]));
      if (String(idList[c]) == idIn) {
        Serial.print("Found id employees ");
        Serial.println(String(String(idList[c]) + " with card id " + String(cardList[c])));
        if (String(cardList[c]).toInt() == cardIn.toInt()) {
          Serial.println("Found card id employees");
          Serial.println(String("Employess " + idIn + " with " + cardIn + " already have in database"));
          webSocket.broadcastTXT(String("!3.1" + idIn + " with " + cardIn + " already have in database").c_str());
          Serial.println(F("-------in file cardList------"));
          f.close();
          printFile("/cardList.txt");
          return;
        }
      }

      if (String(cardList[c]).toInt() == cardIn.toInt()) {
        Serial.println(String("Card id  " + cardIn + " already have in database with employess " + String(idList[c])));
        webSocket.broadcastTXT(String("!3.1" + idIn + " with " + cardIn + "already have in database").c_str());
        Serial.println(F("-------in file cardList------"));
        f.close();
        printFile("/cardList.txt");
        return;
      }
    }
    f.print(idIn);
    f.print("-");
    f.println(cardIn);
    f.close();
    Serial.println(String("Added " + idIn + " with " + cardIn));
    cardCount++;
  }
  Serial.println(F("-------in file cardList------"));
  printFile("/cardList.txt");
}
void getCardList() {
  File f = SPIFFS.open("/cardList.txt", "r");
  if (!f) {
    Serial.println("Cant open cardList.txt file!!");
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  int rec_c = 0;
  String c = "", d = "";
  while (f.available()) {
    c = f.readStringUntil('-');
    d = f.readStringUntil('\n');
    d = d.substring(0, d.length() - 1);
    rec_c++;
  }
  f.close();

}
void writeToTopFile(String fileName, String text, String newTracking) {
  int countline = 0;
  File f = SPIFFS.open(fileName.c_str(), "r");
  if (!f) {
    Serial.println("cant open file to change value in top file!!");
    return;
  }
  Serial.print("Top file before change is: ");
  Serial.println(f.readStringUntil('\n'));
  while (f.available()) {
    Serial.println(f.readStringUntil('\n'));
    countline++;
  }
  f.seek(0, SeekSet);
  Serial.println(f.readStringUntil('\n'));
  String arr[countline];
  for (int i = 0; i < countline; i++) {
    String temp = f.readStringUntil('\n');
    temp = temp.substring(0, temp.length() - 1);
    arr[i] = temp;
  }
  f.close();
  Serial.println();
  Serial.println(countline);
  Serial.println();
  if (SPIFFS.remove(fileName.c_str())) Serial.println("remove file for change value done");
  f = SPIFFS.open(fileName.c_str(), "w+");
  Serial.println("Start write file again");
  f.println(text);
  Serial.println(text);
  for (int i = 0; i < countline; i++) {
    f.println(arr[i]);
  }
  f.println(newTracking);
  f.close();
}
void showFullLed(int z = 0) {
  ledTime.detach();
  for (int i = 1; i <= 4; i++)  display.display(i - 1, z); delay(550); display.clearDisplay(); delay(50);
  for (int i = 1; i <= 4; i++)  display.display(i - 1, z); delay(550);
  ledTime.attach(1, ledTimeShow);
}
void addSalaryTracking() {

  String IDMemTemp = "", c = "", d = "", Card = "";
  File f = SPIFFS.open("/cardList.txt", "r+");
  if (!f) {
    Serial.println("file open failed, cant add salary tracking!!");
    return;
  }
  if (f.size() == 0) {
    Serial.println("No data in cardList.txt, cant add salary tracking too!!");
    f.close();
    return;
  }
  Card =  String(idCardRecoTemp);
  while (f.available()) {

    c = f.readStringUntil('-');
    d = f.readStringUntil('\n');
    d = d.substring(0, d.length() - 1);
    if (d == Card) {
      IDMemTemp = c;
      break;
    }
  }
  f.close();
  if (IDMemTemp == "") {
    Serial.println(F("Cant find your card in DB so cant add salary tracking too"));
    return;
  }
  time_t t = timeUnixNow();
  String q = "/" + String(getNam(t)) + "/" + String(getThang(t)) + "/" + IDMemTemp + ".txt";
  Serial.println(q);
  if (!SPIFFS.exists(q.c_str())) {
    File z = SPIFFS.open(q.c_str(), "a");
    z.close();
  }
  File file = SPIFFS.open(q.c_str(), "r+");
  if (!file) {
    Serial.println("Cant open data tracking for this user!!");
    return;
  }

  if (file.size() == 0) {
    Serial.println("Init salary file");
    file.println(String("1" + String(getNgay(timeUnixNow()))));
    Serial.println(String("1" + String(getNgay(timeUnixNow()))));
    for (int i = 1; i <= 4; i++)  display.display(i - 1, 0);
    file.println(timeUnixNow());
    Serial.println(F("Add salary tracking done for this user!!"));
    file.close();
    showFullLed(0);
    return;
  } else {
    Serial.println("reading state of salary tracking"); \
    String state = file.readStringUntil('\n');
    state = state.substring(0, state.length() - 1);
    int check = state.substring(0, 1).toInt();
    int dayWorking = state.substring(1, state.length()).toInt();
    if (String(dayWorking) != String(getNgay(timeUnixNow()))) {
      Serial.println("Start new Day working");
      file.close();
      writeToTopFile(q, String("1" + String(getNgay(timeUnixNow()))), String(timeUnixNow()));
      showFullLed(0);
    } else {
      if (check == 1) {

        file.close();
        writeToTopFile(q, String("0" + String(getNgay(timeUnixNow()))), String(timeUnixNow()));
        showFullLed(9);
      } else {
        file.close();
        writeToTopFile(q, String("1" + String(getNgay(timeUnixNow()))), String(timeUnixNow()));
        showFullLed(0);
      }

    }
  }

  Serial.println(F("Add salary tracking done for this user!!"));
  return;


}
void sendRTCTimeToClient(uint8_t num) {
  Time tTemp = rtc.getTime();
  unsigned long timeTimpp = getEpoch(tTemp.hour, tTemp.min, tTemp.sec, tTemp.date, tTemp.mon, tTemp.year);
  webSocket.sendTXT(num, String("!1" + String(timeTimpp)).c_str());
  Serial.println("Da gui thoi gian toi web browser");
}
void sendIdCardToClient(uint8_t num) {
  String id = String(idCardRecoTemp);
  webSocket.sendTXT(num, String("!2" + id).c_str());
}
void sendAllEmployess(uint8_t num = 0) {
  String dataSend = "!4";
  File f = SPIFFS.open("/cardList.txt", "a+");
  if (!f) {
    Serial.println("file open failed");
    return;
  }
  if (f.size() == 0) {
    dataSend += "0#";
    webSocket.sendTXT(num, dataSend.c_str());
    f.close();
    return;
  }
  int  count = 0, i = 0, idCount = 0;
  while (f.available()) {
    Serial.println(f.readStringUntil('\n'));
    count++;
  }
  String idList[count], cardList[count];
  for (i = 0; i < count ; i++) {
    idList[i] = "";
    cardList[i] = "";
  }
  f.seek(0, SeekSet); i = 0;
  while (f.available()) {
    idList[i] = f.readStringUntil('-');
    cardList[i] =  f.readStringUntil('\n');
    i++;
  }
  f.close();
  String idListDiff[count];
  Serial.println("Delete all array idListDiff[]");
  for (int c = 0; c < count; c++)idListDiff[c] = "";
  for (int a = 0; a < count; a++) {
    if (a == 0) {
      idListDiff[a] = String(idList[a]);
      idCount = 1;
    }
    int check = 0;
    for (int b = 0; b < idCount; b++) {
      if (String(idList[a]) == String(idListDiff[b])) {
        check = 1;
      }
    }
    if (!check) {
      idListDiff[idCount] = String(idList[a]);
      idCount ++;
    }
  }

  Serial.println();
  Serial.println("List id Diff");
  for (int x = 0; x < idCount; x++) {
    Serial.println(String(idListDiff[x]));
  }
  Serial.println();
  dataSend = dataSend + String(idCount) + "#";
  /*  type normal
    for(int i =0; i<idCount; i++){
    dataSend += String(idListDiff[i]) + ",";
    f = SPIFFS.open(String("/idMem/"+String(idListDiff[i])+"/info.txt").c_str(), "a+");
    if(f.size()==0){
      dataSend += "Nhân viên No Name,";
      f.close();
    } else{
      dataSend += String(f.readStringUntil('\n')) + ",";
      f.close();
    }
    for(int v=0; v<count; v++){
      if(String(idList[v])==String(idListDiff[i])) {
        dataSend += String(cardList[v]);
        dataSend += ",";
      }
    }
    if(String(dataSend[dataSend.length()-1])==",") dataSend.remove(dataSend.length()-1,1);
    dataSend += "#";
    }
  */

  // type json obj string =)))
  dataSend += "{";
  for (int i = 0; i < idCount; i++) {

    dataSend += String((char)34) + String(idListDiff[i]) + String((char)34) + ":{" + String((char)34) + "name" + String((char)34) + ":" + String((char)34);
    f = SPIFFS.open(String("/idMem/" + String(idListDiff[i]) + "/info.txt").c_str(), "a+");
    if (f.size() == 0) {
      dataSend += "Nhân viên No Name" + String((char)34) + ",";
      f.close();
    } else {
      String t = String(f.readStringUntil('\n'));
      t.remove(t.length() - 1, 1);

      dataSend += t + String((char)34) + ",";
      f.close();
    }
    int z = 0;
    for (int v = 0; v < count; v++) {
      if (String(idList[v]) == String(idListDiff[i])) {
        dataSend += String((char)34) + String(z) + String((char)34) + ":" + String((char)34) + String(String(cardList[v]).toInt()) + String((char)34) + ",";
        z++;
      }
    }

    if (String(dataSend[dataSend.length() - 1]) == ",") dataSend.remove(dataSend.length() - 1, 1);
    dataSend += "}";
    if (i != idCount - 1) {
      dataSend += ",";
    }

  }
  dataSend += "}";



  Serial.println(dataSend);
  webSocket.sendTXT(num, dataSend.c_str());
}
void deleteCardId(uint8_t num = 0, String inCard = "") {
  Serial.print("Deleting Card ID: "); Serial.print(inCard);  Serial.println(" from all database");
  File f = SPIFFS.open("/cardList.txt", "r+");
  if (!f) {
    Serial.println("file open failed");
    return;
  }

  if (f.size() == 0) {
    Serial.println("/cardList.txt empty, nothing to delete!!!");
    webSocket.sendTXT(num, String("!50").c_str());
    f.close();
    return;
  }
  if (inCard == "") {
    Serial.println("Card ID is empty, not true!!");
    return;
  }
  int count = 0, i = 0;
  String y = "";
  while (f.available()) {
    y =  f.readStringUntil('\n');
    count++;
  }
  f.seek(0, SeekSet);
  String id[count], card[count];
  for (int c = 0; c < count; c++) {
    id[c] = "";
    card[c] = "";
    Serial.println("clear array");
  }
  i = 0;
  while (f.available()) {
    id[i] = f.readStringUntil('-');
    Serial.print(String(id[i])); Serial.print("-");
    card[i] =  f.readStringUntil('\n');
    Serial.println(String(card[i]));
    if (String(card[i]).toInt() == inCard.toInt()) y = String(id[i]);
    i++;

  }
  f.close();
  f = SPIFFS.open("/cardList.txt", "w");
  if (!f) {
    Serial.println("file open failed");
    return;
  }
  int isOneIdLeft = 0;
  for (int q = 0; q < count; q++) {
    if (String(card[q]).toInt() != inCard.toInt()) {
      if (String(id[q]).toInt() == y.toInt()) {
        isOneIdLeft++;
      }
      f.print(String(id[q]));
      Serial.print(String(id[q])); Serial.print("-");
      f.print("-");
      f.println(String(card[q]));
      Serial.println(String(card[q]));
    }
    else {
      Serial.println(String("Deleted row" + String(id[q]) + "-" + String(card[q])));
      isOneIdLeft++;
    }
  }

  f.close();
  if (isOneIdLeft == 1) {

    deleteAllTrackingOfId(y);
    if (SPIFFS.remove(String("/idMem/" + y + "/info.txt").c_str())) {
      Serial.println("remove both info done!");
    } else Serial.println("Cant remove info!!!");
  }


  Serial.println("Deleted Card ID from all database done");
  webSocket.sendTXT(num, String("!51").c_str());
}
void sendInfoHomeTab(uint8_t num = 0) {
  String dataSend = "!6";
  dataSend += "#" + String(ESP.getChipId()) + "#" + String(ESP.getSketchSize()) + "#" + String(ESP.getFreeHeap()) + "#";
  String s = "", z = "";
  IPAddress ipAP = WiFi.softAPIP();
  for (int i = 0; i < 4; i++)
    s += i  ? "." + String(ipAP[i]) : String(ipAP[i]);
  IPAddress ipLocal = WiFi.localIP();
  for (int i = 0; i < 4; i++)
    z += i  ? "." + String(ipLocal[i]) : String(ipLocal[i]);

  dataSend += String(ESP.getFreeSketchSpace()) + "#" + ESP.getCoreVersion() + "#" + String(WiFi.macAddress()) + "#" + s + "#" + z;
  webSocket.sendTXT(num,  dataSend.c_str());
}
void changeName(String id, String name, uint8_t num) {
  File f = SPIFFS.open(String("/idMem/" + id + "/info.txt").c_str(), "w");
  if (!f) {
    Serial.println("Open file for change name failed!!!");
    f.close();
    return;
  } else {
    f.println(name);
    Serial.print("Already write name to id: "); Serial.print(id); Serial.print(" with name: "); Serial.println(name);
    webSocket.sendTXT(num, "!71");
    return;
  }
}
void getSalaryByMonth(String id, String monthIn, String yearIn, uint8_t num) {
  String qr = "/" + yearIn + "/" + String(monthIn.toInt()) + "/" + id + ".txt";
  printFile(qr);
  Serial.println(qr.length());
  File f = SPIFFS.open(qr.c_str(), "r");
  if (!f) {
    Serial.println("Open file failed!!, cant read salary by month");
    webSocket.sendTXT(num, "Open file failed!!, cant read salary by month");
    webSocket.sendTXT(num, "!80");
    f.close();
    return;
  } else {
    int firstLine = 1;
    String dataSend = "!8" + id + monthIn + yearIn + "\n";
    while (f.available()) {
      if (firstLine) {
        Serial.print("First line is: ");
        Serial.println(f.readStringUntil('\n'));
        firstLine = 0;
      }
      String temp = f.readStringUntil('\n');
      temp.remove(temp.length() - 1, 1);
      Serial.println(temp.length());
      dataSend += temp;
      dataSend += "!";
    }
    Serial.println(dataSend);
    webSocket.sendTXT(num, dataSend);
  }
}
void sendPassWeb(uint8_t num) {

  String dataSend = "!.";
  File f = SPIFFS.open("/configESP.txt", "r");
  if (!f) {
    Serial.println("cant open configESP.txt file to send pass web");
    f.close();
    return;
  }
  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  String temp = "";
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  dataSend += temp;
  Serial.println(dataSend);
  webSocket.sendTXT(num, dataSend.c_str());
}
void changePassWeb(String pass) {
  File f = SPIFFS.open("/configESP.txt", "r");
  String arr[6];
  if (!f) {
    Serial.println("Cant open /configESP.txt to change pass web!!!");
    f.close();
    return;
  }
  if (pass.toInt() > 996992 || pass.toInt() < 100000) {
    Serial.println("Pass web khong hop le, su dung pass mac dinh la 123456");
    pass = "123456";
  }
  String temp = "";
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[0] = temp;
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[1] = temp;
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[2] = temp;
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[3] = temp;
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[4] = temp;
  f.close();
  if (SPIFFS.remove("/configESP.txt")) Serial.println("Delete file config to change value ok!");
  f = SPIFFS.open("/configESP.txt", "w+");
  f.println(String(arr[0]));
  f.println(String(arr[1]));
  f.println(String(arr[2]));
  f.println(String(arr[3]));
  f.println(String(arr[4]));
  f.println(pass);
  Serial.println(pass);
  Serial.println("Change value config for pass web ok!");

}
void changeWifiESP(String dataIn) {
  String nameWifi = "", passWifi = "", timeOut = "";
  nameWifi = dataIn.substring(0, dataIn.indexOf("!"));
  dataIn = dataIn.substring(dataIn.indexOf('!') + 1, dataIn.length());
  Serial.println(nameWifi);
  passWifi = dataIn.substring(0, dataIn.indexOf('!'));
  dataIn = dataIn.substring(dataIn.indexOf('!') + 1, dataIn.length());
  Serial.println(passWifi);
  timeOut = dataIn;
  Serial.println(timeOut);
  File f = SPIFFS.open("/configESP.txt", "r");
  String arr[6];
  if (!f) {
    Serial.println("Cant open /configESP.txt to change wifi config!!!");
    f.close();
    return;
  }
  if (nameWifi == "" || passWifi == "" || timeOut.toInt() > 3600 || timeOut.toInt() < 300 || passWifi.toInt() < 10000000) {
    Serial.println("Some value not right, use default value, ChamCongVietHan, pass: 12345678, timeout is 600");
    nameWifi = "ChamCongVietHan";
    timeOut = "900";
    passWifi = "12345678";
  }
  String temp = "";
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[0] = temp;
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[1] = temp;
  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  arr[5] = temp;
  f.close();
  if (SPIFFS.remove("/configESP.txt")) Serial.println("Delete file config to change value ok!");
  f = SPIFFS.open("/configESP.txt", "w+");
  f.println(String(arr[0]));
  f.println(String(arr[1]));
  f.println(nameWifi);
  f.println(passWifi);
  f.println(timeOut);
  f.println(String(arr[5]));
  f.close();
  Serial.println("Change value config for wifi ok!");

}
void sendWifiESP(uint8_t num) {
  File f = SPIFFS.open("/configESP.txt", "r");
  if (!f) {
    Serial.println("Cant open /configESP.txt to send pass web control!!!");
    f.close();
    return;
  }
  String nameWifi = "", passWifi = "", timeOut = "", dataSend  = "!9";

  Serial.println(f.readStringUntil('\n'));
  Serial.println(f.readStringUntil('\n'));
  nameWifi = f.readStringUntil('\n');
  nameWifi = nameWifi.substring(0, nameWifi.length() - 1);
  Serial.println(nameWifi);
  passWifi = f.readStringUntil('\n');
  passWifi = passWifi.substring(0, passWifi.length() - 1);
  Serial.println(passWifi);
  Serial.println(passWifi.length());
  timeOut = f.readStringUntil('\n');
  timeOut = timeOut.substring(0, timeOut.length() - 1);
  Serial.println(timeOut);
  dataSend += nameWifi + "!" + passWifi + "!" + timeOut;
  Serial.println(dataSend);
  f.close();
  webSocket.sendTXT(num, dataSend.c_str());


}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %s url: %s\n", num, ip.toString().c_str(), payload);

        // send message to client
        webSocket.sendTXT(num, "Connected to Serial on " + WiFi.localIP().toString() + "\n");
      }
      break;
    case WStype_TEXT: {
        String textt = "";
        for (int i = 0; i < lenght ; i++) textt += String((char)payload[i]);
        Serial.println(textt);
        Serial.printf("[%u] Received text: %s\n", num, payload);
        if (textt == "reset") ESP.reset();
        if (textt == "restart") ESP.restart();

        if (String(textt[0]) == "?") { //for get data
          if (String(textt[1]) == "1") { //get RTC time in MCU
            sendRTCTimeToClient(num);

            break;
          }
          if (String(textt[1]) == "2") { //get card ID in MCU
            inGetIDcard = 1;
            numTemp = num;
            timeCountDown = millis();
            break;
          }
          if (String(textt[1]) == "4") {
            sendAllEmployess(num);
            break;
          }
          if (String(textt[1]) == "6") {
            sendInfoHomeTab(num);
            break;
          }
          if (String(textt[1]) == "8") {
            String id = ""; id += String(textt[2]) + String(textt[3]) + String(textt[4]);
            String monthIn = ""; monthIn += String(textt[5]) + String(textt[6]);
            String yearIn = ""; yearIn += String(textt[7]) + String(textt[8]) + String(textt[9]) + String(textt[10]);
            getSalaryByMonth(id, monthIn, yearIn, num);
            break;
          }
          if (String(textt[1]) == ".") {
            sendPassWeb(num);
            break;
          }
          if (String(textt[1]) == "9") {
            sendWifiESP(num);
            break;
          }
        }
        if (String(textt[0]) == "#") { //for set data

          if (String(textt[1]) == "1") { //set time to RTC clock
            textt.remove(0, 2);
            if (textt.toInt() > 946684801) setTimeRTC(0, 0, 0, 0, 0, 0, textt.toInt());
            else Serial.println("Cant set time before 2000/1/1 0:0:1");
            break;
          }
          if (String(textt[1]) == "3") { //add new id and cardId and/or name for this id
            String idIn = "", cardIn = "", nameIn = "";
            int offset = 0;
            idIn += String(textt[2]) + String(textt[3]) + String(textt[4]);
            for (int i = 5; i < textt.length(); i++) {
              if (String(textt[i]) != "!")cardIn += String(textt[i]);

              if (String(textt[i]) == "!") {
                offset = i + 1;
                i = textt.length();
              }
            }
            if (!offset) {
              addCard(idIn, cardIn, "");
              break;
            } else {
              Serial.println(String(idIn + cardIn + nameIn));
              for (offset; offset < textt.length(); offset++) {
                nameIn += String(textt[offset]);
              }
              addCard(idIn, cardIn, nameIn);
              Serial.println(String(idIn + cardIn + nameIn));
              break;
            }
            break;
          }
          if (String(textt[1]) == "5") { // delete cardID from all database  ex: #5366672345     366672345 is card ID
            textt.remove(0, 2);
            deleteCardId(num, textt);
            break;
          }
          if (String(textt[1]) == "7") {
            String id = ""; id += String(textt[2]); id += String(textt[3]); id += String(textt[4]);
            String name = "";
            for (int c = 5; c < textt.length(); c++) {
              name += String(textt[c]);
            }
            changeName(id, name, num);

          }
          if (String(textt[1]) == ".") {
            textt.remove(0, 2);
            changePassWeb(textt);
            break;
          }
          if (String(textt[1]) == "9") {
            textt.remove(0, 2);
            changeWifiESP(textt);
            break;
          }

        }


        break;
      }

    case WStype_BIN:
      Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
      hexdump(payload, lenght);

      // send message to client
      // webSocket.sendBIN(num, payload, lenght);
      break;
  }
}
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
      return;
    } else {
      inputString += inChar;
    }
  }
}
void ledTimeShow() {
  Time t;
  t = rtc.getTime();
  if (!inGetIDcard) {
    if (!checkDot) {
      display.point(POINT_ON);
      display.display(0, t.hour / 10);
      display.display(1, t.hour % 10);
      display.display(2, t.min / 10);
      display.display(3, t.min % 10);
      checkDot = 1;
    } else {
      display.point(POINT_OFF);
      display.display(0, t.hour / 10);
      display.display(1, t.hour % 10);
      display.display(2, t.min / 10);
      display.display(3, t.min % 10);
      checkDot = 0;

    }

  }
}
void ledStarting() {
  display.display(0, 0);
  delay(60);
  display.display(1, 0);
  delay(60);
  display.point(POINT_ON);
  delay(60);
  display.display(2, 0);
  delay(60);
  display.display(3, 0);
  display.clearDisplay();
  display.display(3, 8);
  delay(60);
  display.display(2, 8);
  delay(60);
  display.point(POINT_ON);
  delay(60);
  display.display(1, 8);
  delay(60);
  display.display(0, 8);
  delay(60);
  display.clearDisplay();
  display.display(0, 0);
  delay(60);
  display.display(1, 0);
  delay(60);
  display.point(POINT_ON);
  delay(60);
  display.display(2, 0);
  delay(60);
  display.display(3, 0);
  delay(60);
  display.clearDisplay();
  delay(60);
  display.clearDisplay();
  display.display(3, 0);
  delay(60);
  display.display(2, 0);
  delay(60);
  display.point(POINT_ON);
  delay(60);
  display.display(1, 0);
  delay(60);
  display.display(0, 0);
  delay(60);
  display.clearDisplay();
}
void loadConfig() {
  File f = SPIFFS.open("/configESP.txt", "a+");
  if (!f) {
    Serial.println("Open file failed!!, cant set config for ESP");
    f.close();
    return;
  }
  if (f.size() == 0) {
    Serial.println("Initing config default....");
    f.println("0.1");
    f.println("ThanhDatNguyen");
    f.println("ChamCongVietHan");
    f.println("12345678");
    f.println(15 * 60);
    f.println("123456");
    Serial.println("Init config default done");
  } 
  f.seek(0, SeekSet);
  String temp = "";
  Serial.println("Used config in storage");
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  verESP = temp;
  Serial.println(verESP);
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  creESP = temp;
  Serial.println(creESP);
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  nameWifiESP = temp;
  Serial.println(nameWifiESP);
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  passWifiESP = temp;
  Serial.println(passWifiESP);
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  timeESP = temp;
  Serial.println(timeESP);
  temp = f.readStringUntil('\n');
  temp = temp.substring(0, temp.length() - 1);
  passWebESP = temp;
  Serial.println(passWebESP);
  f.close();

}
void wifiAPFunc() {
  if (millis() < 20000) return;
  WiFi.mode(WIFI_OFF);
  Serial.println("Timeout for use wifi, disabled wifi now");
  wifiAP.detach();
}
void showPassFunc() {
  if (millis() < 4000) return;
  showPassWifi = 1;
  showPass.detach();
}
void showPassToLed() {
  ledTime.detach();
  display.clearDisplay();
  display.point(POINT_OFF);
  int z = 0;
  Serial.println(passWifiESP);
  for (int i = 0; i < passWifiESP.length(); i++) {
    if (i == 4 || i == 8 || i == 12 || i == 16 || i == 20) {
      z = 0; display.clearDisplay();
    }
    Serial.println(String(passWifiESP[i]).toInt());
    display.display(z, String(passWifiESP[i]).toInt());
    delay(1000);
    z++;
  }
  showPassWifi = 0;
  ledTime.attach(1, ledTimeShow);
}
void timeCountDownGetCard() {
  ledTime.detach();
  int ti = 0;
  ti = millis();
  if ((ti - timeCountDown) > 10000) {
    Serial.println("Het thoi gian cho card");
    inGetIDcard = 0;
    return;
  }
  if ((ti - timeCountDown) > 0 && (ti - timeCountDown) < 1000) {
    display.clearDisplay(); display.display(2, 1); display.display(3, 0);
  } else if ((ti - timeCountDown) > 1001 && (ti - timeCountDown) < 2000) {
    display.clearDisplay(); display.display(3, 9);
  } else if ((ti - timeCountDown) > 2001 && (ti - timeCountDown) < 3000) {
    display.clearDisplay(); display.display(3, 8);
  } else if ((ti - timeCountDown) > 3001 && (ti - timeCountDown) < 4000) {
    display.clearDisplay(); display.display(3, 7);
  } else if ((ti - timeCountDown) > 4001 && (ti - timeCountDown) < 5000) {
    display.clearDisplay(); display.display(3, 6);
  } else if ((ti - timeCountDown) > 5001 && (ti - timeCountDown) < 6000) {
    display.clearDisplay(); display.display(3, 5);
  } else if ((ti - timeCountDown) > 6001 && (ti - timeCountDown) < 7000) {
    display.clearDisplay(); display.display(3, 4);
  } else if ((ti - timeCountDown) > 7001 && (ti - timeCountDown) < 8000) {
    display.clearDisplay(); display.display(3, 3);
  } else if ((ti - timeCountDown) > 8001 && (ti - timeCountDown) < 9000) {
    display.clearDisplay(); display.display(3, 2);
  } else if ((ti - timeCountDown) > 9001 && (ti - timeCountDown) < 9000) {
    display.clearDisplay(); display.display(3, 1);
  }
  ledTime.attach(1, ledTimeShow);

}


void setup() {
  display.init();
  display.set(7);
  ledStarting();
  ledTime.attach(1, ledTimeShow);
  Serial.begin(500000);
  SPI.begin();
  SPIFFS.begin();
  loadConfig();
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(nameWifiESP.c_str(), passWifiESP.c_str());
  dnsServer.start(DNS_PORT, "*", apIP);
  server.begin();
  webSocket.begin();
  MFRC522::MIFARE_Key key; 
  wifiAP.attach(timeESP.toInt(), wifiAPFunc);
  showPass.attach(15, showPassFunc);
  Serial.println("all ticker start");
  rfid.PCD_Init();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  httpUpdater.setup(&server);
  for (byte i = 0; i < 6; i++)    key.keyByte[i] = 0xFF;
  Serial.println ( F("HTTP server started" ));
  server.serveStatic("/", SPIFFS, "/web/index.html"); server.serveStatic("/index.html", SPIFFS, "/web/index.html");
  server.serveStatic("/css/b.css", SPIFFS, "/web/css/b.css");
  server.serveStatic("/css/bt.css", SPIFFS, "/web/css/bt.css");
  server.serveStatic("/js/bootstrap.min.js", SPIFFS, "/web/js/bootstrap.min.js");
  server.serveStatic("/js/jquery-3.3.1.min.js", SPIFFS, "/web/js/jquery-3.3.1.min.js");
  server.serveStatic("/js/monitor.js", SPIFFS, "/web/js/monitor.js");
  printListFile();
  webSocket.onEvent(webSocketEvent);
  Serial.println("START MIRRORING SERIAL");
  inputString.reserve(256);
  rtc.halt(false);
  Serial.print("Time used for starting system: ");
  Serial.print(millis());
  Serial.println(" ms");
}
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  webSocket.loop();
  serialEvent();
  if (showPassWifi) {
    showPassToLed();
  }
  if (stringComplete) {

    String line = inputString;
    String temp = "";
    for (int i = 0; i < 7; i++) temp += String(line[i]);
    if (temp == "setTime") {

      temp = "";
      for (int i = 7; i < line.length(); i++) {
        temp += String(line[i]);
      }
      long timee = temp.toInt();
      if (timee > 946684801) {
        setTimeRTC(0, 0, 0, 0, 0, 0, timee);
      }
    } else if ( temp == "restart") ESP.restart();
    if (String(line[0]) == "p") {
      temp = "";
      for (int i = 2; i < line.length(); i++) {
        temp += String(line[i]);
      }
      Serial.println(temp);
      printFile(temp);
    }
    if (String(line[0]) == "r") {
      temp = "";
      for (int i = 2; i < line.length() - 1; i++) {
        temp += String(line[i]);
      }
      Serial.println(temp);
      if (SPIFFS.remove(temp.c_str())) Serial.println("Delete file ok!");
      else Serial.println("Delete file failed!!!");
    }
    inputString = "";
    stringComplete = false;
    webSocket.broadcastTXT(line);
    Serial.println(line);
  }
  if (inGetIDcard == 1) {
    timeCountDownGetCard();
  }
  if ( rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    showCardInfo();
    showTimeNow("Card Recogize at  ", timeUnixNow());
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    if (!inGetIDcard)addSalaryTracking();
    if (inGetIDcard == 1) {
      inGetIDcard = 0;
      sendIdCardToClient(numTemp);
      Serial.println("Da gui id card to web browser");
    }
  }



}

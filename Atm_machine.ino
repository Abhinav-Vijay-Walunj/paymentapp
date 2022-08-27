#include<HTTPClient.h>
#include <WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include<EEPROM.h>
//for not esp32
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   
#include <ArduinoJson.h>

const char* MySSID = "Abhi";
const char* myPassword = "Abhi@2001";
const char* myServer ="https://api.thingspeak.com/update?";

#define myBotTokedID "2113648614:AAHC-s5YehUuKvulbsR5DDnNzphuuaKP1HE"  
#define myChatId "895842449"
String myChannelApiWriteKey = "YYRSOWSFAQS9MONE";


WiFiClientSecure client;
UniversalTelegramBot bot(myBotTokedID, client);

// check arrival request in every 1 sec. 
int produceDelayRequest = 1000;
unsigned long prevRunTime;
int notes500=0,notes1000=0,notes2000=0;
int accBalance=0;
bool isVerified=false;
char readPin() {
 int pin0;
  int pin1,pin2,pin3,pin4,pin5,pin6,pin7,pin8;
  while(1)
  {
  pin0=touchRead(2);
  pin1=touchRead(4);
  pin2=touchRead(15);
  pin3=touchRead(13);
  pin4=touchRead(12);
  pin5=touchRead(14);
  pin6=touchRead(27);
  pin7=touchRead(32);
  pin8=touchRead(33);
  int sum=(pin1<50)+(pin2<50)+(pin3<50)+(pin4<50)+(pin5<50)+(pin6<50)+(pin7<50)+(pin8<50);
  if(sum!=0) break;
  delay(1000); 
  }
//  if(pin0>50) return '0';
  
  if(pin0<50) return '0';
  if(pin1<50) return '1';
  if(pin2<50) return '2';
  if(pin3<50) return '3';
  if(pin4<50) return '4';
  if(pin5<50) return '5';
  if(pin6<50) return '6';
  if(pin7<50) return '7';
  if(pin8<50) return '8';
  return '\0';
}

String readFromPins() {
  bot.sendMessage(myChatId,"Enter no of digits","");
  String input;
  char count=readPin();
  Serial.println("count is "+count);
  count-=48;
  bot.sendMessage(myChatId,"Enter digits","");
  for(int i=0;i<count;i++){ 
    char ch=readPin();
    Serial.println(ch);
    input+=ch;
    }
  return input;
}

void readDataFromEEPROM() {
  int low = EEPROM.read(0);
   int high=EEPROM.read(1);
   accBalance=low+high*(1<<8);
   if(accBalance%100) {
    low=168;
    high=97;
    EEPROM.write(0,low);
    EEPROM.commit();
    EEPROM.write(1,high);
    EEPROM.commit();
    EEPROM.write(2,10);
    EEPROM.commit();
    EEPROM.write(3,10);
    EEPROM.commit();
    EEPROM.write(4,5);
    EEPROM.commit();
    accBalance=25000;
    notes500=10;
    notes1000=10;
    notes2000=5;
   }
   else {
     notes500=EEPROM.read(2);
     notes1000=EEPROM.read(3);
     notes2000=EEPROM.read(4);
   }
}
void writeDataIntoEEPROM() {
  int low=accBalance&255;
  int high=accBalance>>8;
  EEPROM.write(0,low);
  EEPROM.commit();
  EEPROM.write(1,high);
  EEPROM.commit();
  EEPROM.write(2,notes500);
  EEPROM.commit();
  EEPROM.write(3,notes1000);
  EEPROM.commit();
  EEPROM.write(4,notes2000);
  EEPROM.commit();
}

void availableCommands() {
  if(isVerified) {
    String arrivalMessage;
        arrivalMessage+="Following commands you can use\n";
        arrivalMessage+="/balance to check your balance\n";
        arrivalMessage+="/withdraw to withdraw money\n";
        arrivalMessage+="/deposit to deposit money\n";
        arrivalMessage+="/log_out";
        bot.sendMessage(myChatId,arrivalMessage,"");
  }
  else {
    bot.sendMessage(myChatId,"/login to access your account","");
  }
}

int withdrawAmt(int withdraw) {
  int tnotes2000=notes2000,tnotes1000=notes1000,tnotes500=notes500;
  while(withdraw>=2000&&tnotes2000) {
    tnotes2000--;
    withdraw-=2000;
  }
  while(withdraw>=1000&&tnotes1000) {
    tnotes1000--;
    withdraw-=1000;
  }
  while(withdraw>=500&&tnotes500) {
    tnotes500--;
    withdraw-=500;
  }
  if(withdraw==0) {
    notes2000=tnotes2000;
    notes1000=tnotes1000;
    notes500=tnotes500;
  }
  return withdraw;
}

void addIntoBalance(int depMoney) {
  bot.sendMessage(myChatId,"Enter no of notes of 500","");
  while(Serial.available()==0) {}
  String notesOf500=Serial.readString();
  bot.sendMessage(myChatId,"Enter no of notes of 1000","");
  while(Serial.available()==0) {}
  String notesOf1000=Serial.readString();
  bot.sendMessage(myChatId,"Enter no of notes of 2000","");
  while(Serial.available()==0) {}
  String notesOf2000=Serial.readString();
  int t500=notesOf500.toInt(),t1000=notesOf1000.toInt(),t2000=notesOf2000.toInt();
  if(t500*500+1000*t1000+2000*t2000==depMoney) {
    notes500+=t500;
    notes1000+=t1000;
    notes2000+=t2000;
    accBalance+=depMoney;
    char temp[25];
    itoa(accBalance,temp,10);
    String depp(temp);
    bot.sendMessage(myChatId,"Money has been successfully deposited and your net balance is "+depp,"");
  }
  else bot.sendMessage(myChatId,"Incorrect data of notes entered\n","");
  
}

void sendBalToThinkspeak() {
 HTTPClient httpClient;
  httpClient.begin(myServer);
  String dataForm="api_key="+myChannelApiWriteKey+"&field1="+String(accBalance);
  int sentInfo=httpClient.POST(dataForm);
    Serial.print("sentInfo:");
  Serial.println(sentInfo);
  httpClient.end();
}


void respondArrivedRequest(int arrivedMessage) {
  Serial.println("respondArrivedRequest");
  Serial.println(String(arrivedMessage));

  for (int i=0; i<arrivedMessage; i++) {
    // Chat id of the requester
    String myChat_id = String(bot.messages[i].chat_id);
    if (myChat_id != myChatId){
      bot.sendMessage(myChatId, "You don't have write to send request", "");
      continue;
    }
    
    // Print the received message
    String request = bot.messages[i].text;
    Serial.println(request);

    String Username = bot.messages[i].from_name;
    readDataFromEEPROM();
    if (request == "/start") {
      String arrivalMessage = "Hii, " + Username + ".\n";
      arrivalMessage += "arrivalMessage to your ATM.\n\n";
      arrivalMessage += "press /login to access your account\n";
      bot.sendMessage(myChatId, arrivalMessage, "");
    }
    
    if (request == "/login") {
//      srand(time(0));
      int generaterandomOtp=rand()%100;
      char str[25];
      itoa(generaterandomOtp,str,10);
      String randomotp(str);
      bot.sendMessage(myChatId,"Your required OTP is "+randomotp,"");
      Serial.println("Your OTP is "+randomotp);
      while (Serial.available() == 0)   
      { }
      String  otpInput = Serial.readString();
//        bot.sendMessage(myChatId,"Enter no of digits","");
//        String otpInput=readFromPins();
      Serial.println("otpInput is "+otpInput);
      if(randomotp==otpInput) { 
        bot.sendMessage(myChatId,"Successfully isVerified!","");
        isVerified=true;
        availableCommands();
      }
      else bot.sendMessage(myChatId, "Entered Incorrect OTP \n press /login to login again", "");
    }

    if(request=="/deposit") {
      if(!isVerified) {
        bot.sendMessage(myChatId,"Please /login first to deposit the money","");
        continue;
      }
      bot.sendMessage(myChatId,"Enter Amount","");
      while(Serial.available()==0) {}
      String depositAmt=Serial.readString();
      int depMoney=depositAmt.toInt();
      if(depMoney%100) {
        bot.sendMessage(myChatId,"Enter amount in the multiple of 500","");
        continue;
      }
      addIntoBalance(depMoney);
      availableCommands();
    }
    

    if(request=="/balance") {
      char temp[25];
      itoa(accBalance,temp,10);
      String bal(temp);
      if(isVerified) {
      bot.sendMessage(myChatId, "Your current balance is "+bal,"");
      Serial.println("Your current balance is "+bal);
      availableCommands();
      }
      else {
        bot.sendMessage(myChatId,"Please /login to check your balance","");
      }
    }

    if(request=="/withdraw") {
      if(!isVerified) {
        bot.sendMessage(myChatId,"Please /login first to withdraw your money","");
        continue;
      }
      bot.sendMessage(myChatId, "Enter Amount ", "");
      while (Serial.available() == 0)   
      { }
      String  inputWithdraw = Serial.readString();
//      Serial.println("Entered Withdraw amount is "+inputWithdraw);
      int withdraw=inputWithdraw.toInt();
      if(accBalance<withdraw) {
        bot.sendMessage(myChatId, "Your Entered amount is more than your current balance ", "");
        Serial.println("Your Entered amount is more than your current balance ");
        availableCommands();
      }
      else {
        int x=withdrawAmt(withdraw);
        if(x) {
          bot.sendMessage(myChatId,"Required notes not available to provide","");
          Serial.println("Cancel Withdrawing");
          availableCommands();
          continue;
        }
        accBalance-=withdraw;
        char temp[25],temp2[25];
      itoa(accBalance,temp,10);
      itoa(withdraw,temp2,10);
      String with(temp2);
      String bal(temp);
        bot.sendMessage(myChatId,"Transaction Successful for the withdrawal of amount "+with+" \n your current balance is "+bal,"");
        Serial.println("Withdrawal amount is "+with);
        availableCommands();
      }
    }
    
    if (request == "/log_out") {
      isVerified=false;
      bot.sendMessage(myChatId,"You have logged out successfully","");
      availableCommands();
    }
    writeDataIntoEEPROM();
    sendBalToThinkspeak();
  }
  
}

void setup() {
  Serial.begin(115200);


   EEPROM.begin(5);
   readDataFromEEPROM();
  pinMode(4,INPUT);
 pinMode(2,INPUT);
  pinMode(15,INPUT);
  pinMode(13,INPUT);
  pinMode(12,INPUT);
  pinMode(14,INPUT);
  pinMode(27,INPUT);
  pinMode(32,INPUT);
  pinMode(33,INPUT);
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(MySSID, myPassword);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {
  if (millis() > prevRunTime + produceDelayRequest)  {
    int arrivedMessage = bot.getUpdates(bot.last_message_received + 1);

    while(arrivedMessage) {
      Serial.println("got response");
      respondArrivedRequest(arrivedMessage);
      arrivedMessage = bot.getUpdates(bot.last_message_received + 1);
    }
    prevRunTime = millis();
  }
}

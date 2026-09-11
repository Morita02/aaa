#include <Arduino.h>

#include <XBOXUSB.h>
#include "mu3.h"

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

const int analogPin = 32;
int vol;
int tx_vol;
char sendKey;

Mu3 mu3;
USB Usb;
XBOXUSB xbox(&Usb);

bool printAngle = false, printTouch = false;
uint16_t lastMessageCounter = -1;
uint8_t player_led_mask = 0;
bool microphone_led = false;

class Origin{
private:
  Origin *next_node;
  static Origin *start_node;
  static Origin **previous_node_s_next_node;
public:
  Origin();
  //~Origin(){};
  virtual void setup() = 0;
  virtual void loop() = 0;
  static void setup_all(){
    Origin *current;
    for(current = start_node; current != nullptr; current = current->next_node){
      current->setup();
    }
    Serial.write("setup complete\n");
  }
  static void loop_all(){
    Origin *current;
    for(current = start_node; current != nullptr; current = current->next_node){
      current->loop();
    }
  }
  static void print_all(){
    Origin *current;
    for(current = start_node; current != nullptr; current = current->next_node){
      Serial.print((unsigned int)current);
      Serial.print("\n");
    }
  }
};
Origin *Origin::start_node = nullptr;
Origin **Origin::previous_node_s_next_node = nullptr;
Origin::Origin(){
  next_node = nullptr;
  if(start_node == nullptr){
    start_node = this;
  }else{
    *previous_node_s_next_node = this;
  }
  previous_node_s_next_node = &next_node;
}
/*
Origin::~Origin(){
  Serial.write("delete start\n");
  Origin *current;
  Origin *previous_node = nullptr;
  if(start_node == this){
    Serial.write("a");
    current = start_node;
    start_node = current->next_node;
    Serial.write("delete okk\n");
  }else{
    Serial.write("b");
    for(current = start_node; current != this ; current = current->next_node){
      previous_node = current;
      Serial.write("c");
    }
    previous_node->next_node = current->next_node;
    Serial.write("delete ok\n");
  }
}
*/
class Limitswitch:Origin{
private:
  byte limitswitchpin;
  bool ispositive;
public:
  Limitswitch(byte pin, bool ispositive_arg = true);
  void setup()override;
  void loop()override{}
  bool isp(){
    if(ispositive){
      if(digitalRead(limitswitchpin) == HIGH){
        return true;
      }else{
        return false;
      }
    }else{
      if(digitalRead(limitswitchpin) == LOW){
        return true;
      }else{
        return false;
      }
    }
  }
  operator bool(){return isp();}
};
Limitswitch::Limitswitch(byte pin, bool ispositive_arg){
  limitswitchpin = pin;
  ispositive = ispositive_arg;
}
void Limitswitch::setup(){
  pinMode(limitswitchpin, INPUT_PULLUP);
  Serial.write("switch setup ok\n");
}

class regularC{
private:
  unsigned long interval;
  unsigned long nextTime;
public:
  regularC(unsigned long intervalArg,unsigned long start=0):
    interval(intervalArg)
  {
    nextTime=start;
  }
  bool ist(){
    if(nextTime<millis()){
      nextTime=interval+nextTime;
      return true;
    }else{
      return false;
    }
  }
  operator bool(){return ist();}
  void set(unsigned long val){interval=val;}
  unsigned long read(){return interval;}
};

Limitswitch first_row(32, false);
Limitswitch second_row(33, false);
Limitswitch third_row(25, false);
Limitswitch fourth_row(26, false);

char sendmessage[64];

void getMu3(const char* num){
  //Serial.println(num);
}

char checkmove(){
  char button;
  if(xbox.getButtonClick(R3)!=0)button='c';
  else if(xbox.getButtonClick(L3)!=0)button='v';
  else if(xbox.getButtonClick(B)!=0)button='h';
  else if(xbox.getButtonClick(A)!=0)button='i';
  else if(xbox.getButtonClick(Y)!=0)button='o';
  else if(xbox.getButtonClick(X)!=0)button='p';
  else if(xbox.getButtonPress(UP)!=0)button='w';
  else if(xbox.getButtonPress(DOWN)!=0)button='s';
  else if(xbox.getButtonPress(LEFT)!=0)button='a';
  else if(xbox.getButtonPress(RIGHT)!=0)button='d';
  else if(xbox.getButtonPress(L1)!=0)button='l';
  else if(xbox.getButtonPress(R1)!=0)button='r';
  else if(xbox.getButtonPress(L2)!=0)button='k';
  else if(xbox.getButtonPress(R2)!=0)button='t';
  else if(xbox.getButtonClick(OPTIONS)!=0)button='g';
  else button='n'; 
  return button;
}


char surinukecheck(){
  char val;
  if(first_row)val='1';
  else if(second_row)val='2';
  else if(third_row)val='3';
  else if(fourth_row)val='4';
  else val='0';
  return val;
}

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif

  // if (Usb.Init() == -1) {
  //   Serial.print(F("\r\nOSC did not start"));
  //   while (1); // Halt
  // }

  //Serial.print(F("\r\nxbox USB Library Started"));
  Serial1.begin(19200, SERIAL_8N1,21,22);//rx,tx　serial2のデフォルトピンなんか調子悪いため。
  delay(1000);
  //while(!Serial1);
  mu3.set(2,110,5,6);//ch,groupID,myNum,sendNum 1から254までで適当に決める。相手と合わせる。
  mu3.attach(getMu3);
  Serial1.print("@rc");
  Serial1.print("\r\n");
  Origin::setup_all();
}

void loop() {
  char val;
  char key;
  static regularC checkxboxTime(50);//50
  mu3.check();
  //Usb.Task();
  vol = analogRead(analogPin);
  //Serial.println(vol);
  tx_vol = map(vol, 0, 4095, 0, 100);

  sendKey = (tx_vol & 0xFF);
  //Serial.println(tx_vol);

  mu3.send(&sendKey);

  //Serial.print("sended");
  
  // if(checkxboxTime){
  //   if (xbox.Xbox360Connected){
  //     //&& lastMessageCounter != xbox.getMessageCounter())
  //     //lastMessageCounter = xbox.getMessageCounter();

  //     //key = checkmove();
  //     //val = surinukecheck();
  //     //sprintf(sendmessage,"%c",key);
  //     mu3.send(sendmessage);
  //     Serial.print(sendmessage);
  //   }
  // }
  /*static regularC checkRSSILevelTime(1000);
  if(checkRSSILevelTime){
    
  }*/
} 
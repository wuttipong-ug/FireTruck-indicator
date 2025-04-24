#include <Arduino.h>
#include <HardwareSerial.h>

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"  // Ensure this is included

#include "STM32_CAN.h"

#define Main_CAN_ID 0x069
#define LED_Buik PC13

unsigned long previousTime = 0;
unsigned long previousTime2 = 0;

static int failCount = 0;

HardwareSerial mySerial(USART1);

STM32_CAN Can( CAN1, DEF );  //Use PA11/12 pins for CAN1.

static CAN_message_t CAN_RX_msg;
static CAN_message_t CAN_TX_msg;

// โครงสร้างข้อมูลของ TPIC6B595
struct TPIC6B595 {
    uint8_t dataPin;
    uint8_t clockPin;
    uint8_t latchPin;
    uint32_t ledState; // เก็บสถานะ LED (32-bit สำหรับ groupA, 24-bit สำหรับ groupB)
};

// กำหนดขาให้แต่ละกลุ่ม
TPIC6B595 groupB = {PA0, PA1, PA2, 0};  // 4 TPIC6B595 = 32 บิต
TPIC6B595 groupA = {PA3, PA4, PA5, 0};  // 3 TPIC6B595 = 24 บิต


// ฟังก์ชันส่งข้อมูลไปยัง TPIC6B595 หลายตัว
void shiftOutTPIC(TPIC6B595 &group, uint8_t numChips) {
    digitalWrite(group.latchPin, LOW);
    for (int i = (numChips * 8) - 1; i >= 0; i--) {
        digitalWrite(group.clockPin, LOW);
        digitalWrite(group.dataPin, (group.ledState >> i) & 0x01);
        digitalWrite(group.clockPin, HIGH);
    }
    digitalWrite(group.latchPin, HIGH);
}

// ฟังก์ชันควบคุม LED ทีละดวง
void setLED(TPIC6B595 &group, uint8_t numChips, uint8_t index) {
    if (index >= numChips * 8) return; // ป้องกันค่าเกินบิต
    // group.ledState = (uint32_t)1 << index; // เปิดเฉพาะ LED ที่เลือก
    group.ledState = (1UL << (index + 1)) - 1;          // เปิด LED ทุกดวงตั้งแต่ 0 ถึง index
    shiftOutTPIC(group, numChips);
}

void setup() {
    mySerial.begin(115200);
    pinMode(LED_Buik, OUTPUT);

    // Initialize CAN

    // ตั้งค่าขาเป็น OUTPUT
    pinMode(groupA.dataPin, OUTPUT);
    pinMode(groupA.clockPin, OUTPUT);
    pinMode(groupA.latchPin, OUTPUT);

    pinMode(groupB.dataPin, OUTPUT);
    pinMode(groupB.clockPin, OUTPUT);
    pinMode(groupB.latchPin, OUTPUT);

    digitalWrite(groupA.latchPin, LOW);
    digitalWrite(groupB.latchPin, LOW);

    Can.begin();
    Can.setBaudRate(250000);  //250KBPS
    mySerial.println("Setuped..");
    for(int i = 0; i<24; i++){
      setLED(groupB, 3, i);  // Group A (32-bit)
      delay(20);
    }
    for(int i = 0; i<24; i++){
      setLED(groupB, 3, 23-i);  // Group A (32-bit)
      delay(20);
    }
    for(int i = 0; i<32; i++){
    setLED(groupA, 4, i);  // Group B (24-bit)
    mySerial.println(i);
    delay(20);
    }
    for(int i = 0; i<32; i++){
      setLED(groupA, 4, 31-i);  // Group B (24-bit)
      mySerial.println(i);
      delay(20);
    }
}

void loop() {

  if (Can.read(CAN_RX_msg)) {
    if (CAN_RX_msg.id == 0x069) {
        mySerial.print("index0: "); mySerial.print(CAN_RX_msg.buf[0]); mySerial.print("index1: "); mySerial.print(CAN_RX_msg.buf[1]);
        mySerial.println();
        setLED(groupA, 4, CAN_RX_msg.buf[0]);
        setLED(groupB, 3, CAN_RX_msg.buf[1]);
        // mySerial.println(CAN_RX_msg.buf[0]);
    }
  }

  unsigned long currentTime2 = millis();
  if (currentTime2 - previousTime2 >= 1000) {
    previousTime2 = currentTime2;  //  biuk
    digitalToggle(LED_Buik);
    CAN_TX_msg.id = (0x121);
    CAN_TX_msg.len = 8;
    CAN_TX_msg.buf[0] =  0;   
    bool send = Can.write(CAN_TX_msg);
    mySerial.println(send ? "CAN sent OK" : "CAN send failed");
    if (!send) {
        failCount++;
        mySerial.println("CAN write failed");
      
        if (failCount >= 2) {
          mySerial.println("Resetting CAN...");
      
          __HAL_RCC_CAN1_FORCE_RESET();
          delay(10);
          __HAL_RCC_CAN1_RELEASE_RESET();
          delay(10);
      
          Can.begin();
          Can.setBaudRate(250000);
      
          failCount = 0;
        }
      } else {
        failCount = 0;
      }
  }
  
  
  delay(5);


}

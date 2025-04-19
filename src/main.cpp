#include <Arduino.h>
#include <HardwareSerial.h>

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"  // Ensure this is included

#include "STM32_CAN.h"

#define Main_CAN_ID 0x069

unsigned long previousTime = 0;


HardwareSerial mySerial(USART1);

STM32_CAN Can( CAN1, DEF );  //Use PA11/12 pins for CAN1.

static CAN_message_t CAN_RX_msg;

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

void blink() {
    digitalWrite(PC13, LOW);
    delay(50);
    digitalWrite(PC13, HIGH);
    delay(50);
}

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
    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, HIGH);

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
}

void loop() {

  if (Can.read(CAN_RX_msg)) {
    if (CAN_RX_msg.id == 0x069) {

        setLED(groupA, 4, CAN_RX_msg.buf[0]);
        setLED(groupB, 3, CAN_RX_msg.buf[1]);
        // mySerial.println(CAN_RX_msg.buf[0]);
    }
  }

//   for(int i = 0; i<24; i++){
//     setLED(groupB, 3, i);  // Group A (32-bit)
//     delay(100);
//   }
//   for(int i = 0; i<32; i++){
//   setLED(groupA, 4, i);  // Group B (24-bit)
//   mySerial.println(i);
//   delay(200);
//   }
}

#include <Arduino.h>
#include "config.h"
#include "eXoCAN.h"

HardwareSerial mySerial(USART1);
eXoCAN can;

// โครงสร้างข้อมูลของ TPIC6B595
struct TPIC6B595 {
    uint8_t dataPin;
    uint8_t clockPin;
    uint8_t latchPin;
    uint32_t ledState; // เก็บสถานะ LED (32-bit สำหรับ groupA, 24-bit สำหรับ groupB)
};

// กำหนดขาให้แต่ละกลุ่ม
TPIC6B595 groupA = {PA0, PA1, PA2, 0};  // 4 TPIC6B595 = 32 บิต
TPIC6B595 groupB = {PA3, PA4, PA5, 0};  // 3 TPIC6B595 = 24 บิต

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
    group.ledState = (uint32_t)1 << index; // เปิดเฉพาะ LED ที่เลือก
    shiftOutTPIC(group, numChips);
}

void setup() {
    mySerial.begin(115200);
    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, HIGH);

    // Initialize CAN
    can.begin(STD_ID_LEN, BR250K, PORTA_11_12_XCVR);
    mySerial.println("✅ CAN Bus Initialized - Listening for Messages...");

    // ตั้งค่าขาเป็น OUTPUT
    pinMode(groupA.dataPin, OUTPUT);
    pinMode(groupA.clockPin, OUTPUT);
    pinMode(groupA.latchPin, OUTPUT);

    pinMode(groupB.dataPin, OUTPUT);
    pinMode(groupB.clockPin, OUTPUT);
    pinMode(groupB.latchPin, OUTPUT);

    digitalWrite(groupA.latchPin, LOW);
    digitalWrite(groupB.latchPin, LOW);
}

void loop() {
    volatile int rxID;
    volatile int rxFltrIdx;
    uint8_t rxData[8];

    // รับข้อมูล CAN ID 0x069
    if (can.receive(rxID, rxFltrIdx, rxData)) {
        if (rxID == 0x069) {
            blink();
            mySerial.print("📥 Received CAN ID: 0x");
            mySerial.print(rxID, HEX);
            mySerial.print(" | Data: ");

            for (int i = 0; i < 8; i++) {
                mySerial.print("0x");
                if (rxData[i] < 0x10) mySerial.print("0");
                mySerial.print(rxData[i], HEX);
                mySerial.print(" ");
            }
            mySerial.println();

            setLED(groupA, 4, rxData[1]);  // Group A (32-bit)
            setLED(groupB, 3, rxData[0]);  // Group B (24-bit)
        }
    }
}

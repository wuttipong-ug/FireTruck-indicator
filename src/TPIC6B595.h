#include "config.h"
// กำหนดขาของแต่ละชุด TPIC6B595
#define DATA1  PA0  // SER (Data) สำหรับชุดที่ 1
#define CLOCK1 PA1  // SRCK (Clock) สำหรับชุดที่ 1
#define LATCH1 PA2  // RCLK (Latch) สำหรับชุดที่ 1

#define DATA2  PA3  // SER (Data) สำหรับชุดที่ 2
#define CLOCK2 PA4  // SRCK (Clock) สำหรับชุดที่ 2
#define LATCH2 PA5  // RCLK (Latch) สำหรับชุดที่ 2

// เก็บสถานะ LED ของแต่ละชุด
uint32_t ledState1 = 0; // ชุดที่ 1 (32 ดวง)
uint32_t ledState2 = 0; // ชุดที่ 2 (24 ดวง)

// ฟังก์ชัน shiftOut พร้อม Debug

void shiftOut595(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint32_t data, uint8_t numRegisters) {
    digitalWrite(latchPin, LOW);
    for (int i = (numRegisters * 8) - 1; i >= 0; i--) {
        digitalWrite(clockPin, LOW);
        digitalWrite(dataPin, (data >> i) & 0x01);
        digitalWrite(clockPin, HIGH);
    }
    digitalWrite(latchPin, HIGH);
}

// ฟังก์ชันเปิด LED ดวงเดียวและดับดวงอื่นทั้งหมด
void setLED(uint8_t group, uint8_t index) {
    uint32_t *ledState;
    uint8_t dataPin, clockPin, latchPin;
    uint8_t numRegisters;

    if (group == 1) {
        if (index >= 32) {
            mySerial.println("[ERROR] LED index out of range (ชุดที่ 1 มี 32 ดวงเท่านั้น!)");
            return;
        }
        ledState = &ledState1;
        dataPin = DATA1;
        clockPin = CLOCK1;
        latchPin = LATCH1;
        numRegisters = 4;
    } 
    else if (group == 2) {
        if (index >= 24) {
            mySerial.println("[ERROR] LED index out of range (ชุดที่ 2 มี 24 ดวงเท่านั้น!)");
            return;
        }
        ledState = &ledState2;
        dataPin = DATA2;
        clockPin = CLOCK2;
        latchPin = LATCH2;
        numRegisters = 3;
    } 
    else {
        mySerial.println("[ERROR] ไม่พบชุดที่ต้องการ");
        return;
    }

    *ledState = (1UL << index); // เปิดเฉพาะดวงที่เลือก และปิดดวงอื่นทั้งหมด

    mySerial.print("Group ");
    mySerial.print(group);
    mySerial.print(" | LED ");
    mySerial.print(index);
    mySerial.println(" ON | Others OFF");
    
    shiftOut595(dataPin, clockPin, latchPin, *ledState, numRegisters);
}

void bink(){
    digitalWrite(PC13, LOW);
    delay(50);
    digitalWrite(PC13, HIGH);
    delay(50);
    digitalWrite(PC13, LOW);
    delay(50);
    digitalWrite(PC13, HIGH);
    delay(50);
  
  }
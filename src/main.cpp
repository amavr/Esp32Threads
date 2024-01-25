#include <Arduino.h>

#include <GyverOLED.h>
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

// #define MOTION_PIN GPIO_NUM_14;
#define MOTION_PIN GPIO_NUM_14

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello!");

    pinMode(MOTION_PIN, INPUT_PULLUP);

    oled.init();      // инициализация
    oled.clear();     // очистка
    oled.home();      // курсор в 0,0

    oled.setScale(2); // масштаб текста (1..4)

    oled.print("MOVE!");
    // oled.setScale(3); // масштаб текста (1..4)
    // oled.print("Привет!");

    // delay(1000);
    // oled.setScale(1);
    // // курсор на начало 3 строки
    // oled.setCursor(0, 3);
    // oled.print("GyverOLED v1.3.2");
    // // курсор на (20, 50)
    // oled.setCursorXY(20, 50);
    // float pi = 3.14;
    // oled.print("PI = ");
    // oled.print(pi);
}

void loop()
{
    int motion_sensor = digitalRead(MOTION_PIN);
    if(motion_sensor == HIGH)
    {
        oled.home();      // курсор в 0,0
        oled.print("MOVE!");
    }
    else{
        oled.home();      // курсор в 0,0
        oled.print("Тихо!");
    }
    vTaskDelay(100);
}
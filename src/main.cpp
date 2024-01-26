#include <Arduino.h>

#include <GyverOLED.h>

#define MOTION_PIN GPIO_NUM_12
#define MOTION_CALM_DOWN 2000

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

SemaphoreHandle_t sem;
static portMUX_TYPE spin_lock = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR detectsMovement()
{
    BaseType_t woken = pdFALSE;
    portENTER_CRITICAL_ISR(&spin_lock);
    xSemaphoreGiveFromISR(sem, &woken);
    portEXIT_CRITICAL_ISR(&spin_lock);
    if (woken)
    {
        portYIELD_FROM_ISR(woken);
    }
}

void taskMovement(void *pv)
{
    int last_state = 0;
    while (true)
    {
        int motion_sensor = digitalRead(MOTION_PIN);
        if (last_state != motion_sensor)
        {
            last_state = motion_sensor;
            if (motion_sensor == HIGH)
            {
                digitalWrite(LED_BUILTIN, HIGH);

                oled.home(); // курсор в 0,0
                oled.print("MOVE!");
            }
            else
            {
                digitalWrite(LED_BUILTIN, LOW);

                oled.home(); // курсор в 0,0
                oled.print("Тихо!");
            }
        }
        vTaskDelay(100);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello!");

    pinMode(MOTION_PIN, INPUT_PULLUP);

    // attachInterrupt(digitalPinToInterrupt(MOTION_PIN), detectsMovement, CHANGE);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    oled.init();  // инициализация
    oled.clear(); // очистка
    oled.home();  // курсор в 0,0

    oled.setScale(2); // масштаб текста (1..4)

    oled.print("INIT.");

    xTaskCreate(taskMovement, "Motion detection", 4096, NULL, 1, CONFIG_ARDUINO_RUNNING_CORE);
}

void loop()
{
}
#include <Arduino.h>

#include <GyverOLED.h>

#define MOTION_PIN GPIO_NUM_12
#define MOTION_CALM_DOWN 2000

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

SemaphoreHandle_t sem;
static portMUX_TYPE spin_lock = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t ISR = NULL;

void IRAM_ATTR isrMovement()
{
    // portENTER_CRITICAL_ISR(&spin_lock);
    // static BaseType_t woken = pdFALSE;
    // xSemaphoreGiveFromISR(sem, &woken);
    // portEXIT_CRITICAL_ISR(&spin_lock);
    // if (woken == pdTRUE)
    // {
    //     portYIELD_FROM_ISR(woken);
    // }
    xTaskResumeFromISR(ISR);
}

void taskMovement(void *pv)
{
    while (true)
    {
        // xSemaphoreTake(sem, portMAX_DELAY);
        vTaskSuspend(NULL);
        int motion_sensor = digitalRead(MOTION_PIN);
        if (motion_sensor == HIGH)
        {
            digitalWrite(LED_BUILTIN, HIGH);

            oled.home(); // курсор в 0,0
            oled.print("MOVE!");
        }
        else
        {
            digitalWrite(LED_BUILTIN, LOW);
            oled.clear();
        }
    }
}

void setup()
{
    pinMode(MOTION_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(LED_BUILTIN, LOW);

    oled.init();  // инициализация
    oled.clear(); // очистка
    oled.home();  // курсор в 0,0

    oled.setScale(2); // масштаб текста (1..4)

    oled.print("INIT");

    xTaskCreatePinnedToCore(taskMovement, "Motion detection", 4096, NULL, 12, &ISR, CONFIG_ARDUINO_RUNNING_CORE);
    attachInterrupt(digitalPinToInterrupt(MOTION_PIN), isrMovement, CHANGE);
}

void loop()
{
    vTaskDelay(100);
}
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>

#define WIFI_SETTINGS_NAME "wifi"

SemaphoreHandle_t xWiFiConnectionSemaphore;

const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <title>WiFi Configuration</title>
    <style>
        body {font-family: 'Segoe UI', sans-serif;}
        span {width: 80px;display: inline-block;text-align: right;margin-right: 8px;}
        input {border-radius: 4px;border-width: 1px;height: 26px;}
        div {margin-bottom: 10px;}
    </style>
</head>
<body>
    <h1>Configure WiFi</h1>
    <form action="/wifi" method="GET">
        <div><span>SSID</span><input type="text" name="ssid" value="amavr-d"></div>
        <div><span>Password</span><input type="password" name="password" value="220666abba"></div>
        <div><span>&nbsp;</span><input type="submit" value="Connect"></div>
    </form>
</body>
</html>)rawliteral";

// Обработчик подключения к WiFi
bool connectWiFi() {
    WiFi.mode(WIFI_STA);

    Preferences prefs;
    prefs.begin(WIFI_SETTINGS_NAME);
    String ssid = prefs.getString("ssid");
    String pass = prefs.getString("pass");
    prefs.end();

    Serial.println("Prefs");
    Serial.println(ssid);
    Serial.println(pass);

    if (ssid.length() == 0) {
        Serial.println("ssid is empty");
        return false;
    }

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("WiFi.begin");

    for (int i = 0; i < 3; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi status = connected");
            xSemaphoreGive(xWiFiConnectionSemaphore);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    Serial.println("WiFi status != connected");
    return false;
}

// Обработчик корневого URL
void onIndexRequest(AsyncWebServerRequest* request) {
    request->send(200, "text/html", html);
}

// Обработчик формы подключения
void onWiFiRequest(AsyncWebServerRequest* request) {
    // display params
    Serial.println("GET /wifi?....");

    size_t count = request->params();
    for (size_t i = 0; i < count; i++) {
        const AsyncWebParameter* p = request->getParam(i);
        Serial.printf("PARAM[%u]: %s = '%s'\n", i, p->name().c_str(), p->value().c_str());
    }

    if (request->hasParam("ssid") && request->hasParam("password")) {
        String ssid = request->getParam("ssid")->value();
        String pass = request->getParam("password")->value();

        Preferences prefs;

        prefs.begin(WIFI_SETTINGS_NAME);
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        prefs.end();

        bool result = connectWiFi();

        if (result) {
            request->send(200, "text/plain", "Connected to WiFi!");
        } else {
            request->send(500, "text/plain", "Failed to connect");
        }
    }
}

// Задача для веб-сервера
void webServerTask(void* pvParameters) {
    Serial.println("BEG::Web server task run");

    // Инициализация серверов HTTP & DNS
    AsyncWebServer server(80);
    DNSServer dnsServer;

    server.on("/", HTTP_GET, onIndexRequest);
    server.on("/wifi", HTTP_GET, onWiFiRequest);
    // server.on("/wifi", HTTP_POST, onWiFiRequest);
    // server.on("/wifi", HTTP_ANY, onWiFiRequest);
    server.onNotFound(onIndexRequest);

    server.begin();
    Serial.println("BEG::Web server");

    int cycles = 0;

    // Настройки по умолчанию для точки доступа
    IPAddress addrAP(192, 168, 4, 1);
    bool connected = false;
    bool is_ap_mode = false;

    digitalWrite(BUILTIN_LED, true);
    while (!connected && cycles++ < 60) {
        if (!is_ap_mode) {
            // требуется для автоматической переадресации
            // на HTML страницу ввода SSID & Password
            dnsServer.start(53, "*", addrAP);
            Serial.println("BEG::DNS server");

            WiFi.mode(WIFI_AP);
            IPAddress subnet(255, 255, 255, 0);
            WiFi.softAPConfig(addrAP, addrAP, subnet);
            WiFi.softAP("ESP32 Config");

            is_ap_mode = true;

            Serial.println("AP mode activated");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    server.end();
    Serial.println("END::Web server");
    dnsServer.stop();
    Serial.println("END::DNS server");

    digitalWrite(BUILTIN_LED, false);

    Serial.println("END::Web server task run");

    xSemaphoreGive(xWiFiConnectionSemaphore);
    vTaskDelete(NULL);
}

// Задача для веб-сервера
void wifiConnectionTask(void* pvParameters) {
    // Инициализация WiFi
    WiFi.mode(WIFI_STA);
    // Инициализация семафора
    xWiFiConnectionSemaphore = xSemaphoreCreateBinary();

    while (1) {
        if (WiFi.status() != WL_CONNECTED) {
            if (!connectWiFi()) {
                xTaskCreate(webServerTask, "webServerTask", 8192, NULL, 5, NULL);

                // Ждём, пока семафор не будет "дан"
                // (pdMS_TO_TICKS(10000) - максимум 10 секунд, portMAX_DELAY - бесконечно)
                if (xSemaphoreTake(xWiFiConnectionSemaphore, portMAX_DELAY) == pdTRUE) {
                    Serial.println("EVT::Семафор получен");
                } else {
                    Serial.println("EVT::Семафор таймаут");
                }
            }
        } else {
            digitalWrite(BUILTIN_LED, true);
            vTaskDelay(pdMS_TO_TICKS(200));
            digitalWrite(BUILTIN_LED, false);
            vTaskDelay(pdMS_TO_TICKS(4800));
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, false);

    // Запуск задачи мониторинга соединения с WiFi
    xTaskCreate(wifiConnectionTask, "WiFiConnection", 8192, NULL, 5, NULL);
}

void loop() {
    // Основной цикл оставляем пустым
}
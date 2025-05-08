#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Настройки по умолчанию для точки доступа
static IPAddress addrAP(192, 168, 4, 1);

static DNSServer dnsServer;

// Веб-сервер
AsyncWebServer server(80);

// Флаги состояния
bool connectedToWiFi = false;
bool apModeActive = false;

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
        <div><span>SSID</span><input type="text" name="ssid"></div>
        <div><span>Password</span><input type="password" name="password"></div>
        <div><span>&nbsp;</span><input type="submit" value="Connect"></div>
    </form>
</body>
</html>)rawliteral";

// Обработчик подключения к WiFi
void handleWiFiConnect(const String &ssid, const String &password)
{
    WiFi.begin(ssid.c_str(), password.c_str());

    for (int i = 0; i < 20; i++)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            connectedToWiFi = true;
            apModeActive = false;
            break;
        }
        delay(500);
    }
}

// Обработчик корневого URL
void onIndexRequest(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", html);
}

// Обработчик формы подключения
void onWiFiRequest(AsyncWebServerRequest *request)
{
    // display params
    size_t count = request->params();
    for (size_t i = 0; i < count; i++)
    {
        const AsyncWebParameter *p = request->getParam(i);
        Serial.printf("PARAM[%u]: %s = '%s'\n", i, p->name().c_str(), p->value().c_str());
    }

    if (request->hasParam("ssid") && request->hasParam("password"))
    {
        String ssid = request->getParam("ssid")->value();
        String password = request->getParam("password")->value();
        Serial.println(ssid);
        Serial.println(password);

        handleWiFiConnect(ssid, password);

        if (connectedToWiFi)
        {
            request->send(200, "text/plain", "Connected to WiFi!");
        }
        else
        {
            request->send(500, "text/plain", "Failed to connect");
        }
    }
}

// Задача для веб-сервера
void webServerTask(void *pvParameters)
{
    // Инициализация сервера
    server.on("/", HTTP_GET, onIndexRequest);
    server.on("/wifi", HTTP_GET, onWiFiRequest);
    // server.on("/wifi", HTTP_POST, onWiFiRequest);
    // server.on("/wifi", HTTP_ANY, onWiFiRequest);
    // server.onNotFound(onIndexRequest);
    server.begin();

    while (1)
    {
        if (!connectedToWiFi)
        {
            digitalWrite(BUILTIN_LED, true);

            if (!apModeActive)
            {
                dnsServer.start(53, "*", addrAP);

                WiFi.mode(WIFI_AP);
                IPAddress subnet(255, 255, 255, 0);
                WiFi.softAPConfig(addrAP, addrAP, subnet);
                WiFi.softAP("ESP32 Config");

                apModeActive = true;

                server.onNotFound(onIndexRequest);

                Serial.println("AP mode activated");
            }
        }
        else
        {
            digitalWrite(BUILTIN_LED, false);
            Serial.println(WiFi.localIP());
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        // delay(1000);
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, false);

    // Инициализация WiFi
    WiFi.mode(WIFI_STA);

    // Создание задачи веб-сервера
    xTaskCreate(webServerTask, "webServerTask", 8192, NULL, 5, NULL);
}

void loop()
{
    // Основной цикл оставляем пустым
}
/*
 * Copyright (c) 2026 Michel Diego. Todos os direitos reservados.
 * * Sistema de Monitoramento e Controle IIoT (Malha Fechada)
 * Firmware de borda para ESP32, sensores industriais e atuadores.
 */

#include "config.h"

unsigned long lastPublishTime = 0;           
unsigned long lastSystemInfoTime = 0;        
const long publishInterval = 1000;           
const long systemInfoInterval = 60000;       

// Variáveis Globais
bool emergencyMode = false;       
bool pumpState = false;           
int liquidLevel = 150;            
String pumpCommand = "";          

float raio = 5.400;
float alturaMax = 20.34;

float lastTemperature = -1;
int lastLiquidLevel = -1;
bool lastPumpState = false;
bool lastTempState = false;
bool commandToTurnOnFromEmergencyTopic = false;

float readTemperature(); 
int readLiquidLevel();  

WiFiClient net;
PubSubClient client(net);
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

unsigned long lastTemperatureRead = 0; 

time_t initialTime;
unsigned long initialMillis;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

void checkEmergencyButton(); 
void controlPump(bool state); 
void messageHandler(char* topic, byte* payload, unsigned int length);  
void connectToWiFi();
void reconnectWiFi();
void reconnectMQTT();
bool getTimeFromApi();
void updateLEDs(float temperature, int liquidLevel, bool pumpState);
String formatLiquidLevel(int liquidLevel);
void publishData(float temperature, int liquidLevel, bool pumpState);
void publishTemperature(float temperature);
void publishLiquidLevel(int liquidLevel);
void publishPumpState();
void publishSystemInfo();
bool isPumpOn();

bool isCommandFromTopic(const char* topic, const char* expectedTopic) {
  return strcmp(topic, expectedTopic) == 0;
}

void setup() {
    Serial.begin(115200);
    delay(100);

    lcd.init();
    lcd.backlight();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INICIALIZANDO");
    lcd.setCursor(0, 1);
    lcd.print("AGUARDE...");
    delay(2000); 

    sensors.begin();
    Serial.println("Inicializando sensor de temperatura...");

    Serial.print("Sensores encontrados: ");
    Serial.println(sensors.getDeviceCount());
    if (sensors.getDeviceCount() == 0) {
        Serial.println("⚠ Nenhum sensor DS18B20 detectado!");
    }

    pinMode(EMERGENCY_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED_VERMELHA1, OUTPUT);
    pinMode(LED_VERDE1, OUTPUT);
    pinMode(LED_VERMELHA2, OUTPUT);
    pinMode(LED_VERDE2, OUTPUT);
    pinMode(LED_VERMELHA3, OUTPUT);
    pinMode(LED_VERDE3, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    connectToWiFi();
    timeClient.begin();

    Serial.println("Obtendo horário via NTP...");
    timeClient.update();

    if (!timeClient.forceUpdate()) {
        Serial.println("Falha ao obter horário via NTP. Tentando via API...");
        if (!getTimeFromApi()) {
            Serial.println("Falha ao obter horário da API.");
        } else {
            Serial.println("Horário obtido da API com sucesso.");
        }
    } else {
        Serial.println("Horário obtido via NTP com sucesso.");
    }

    client.setServer(MQTT_BROKER_ADDRESS, MQTT_PORT);
    client.setCallback(messageHandler);
    reconnectMQTT();
}

void loop() {
    unsigned long currentMillis = millis();
    timeClient.update();
    checkEmergencyButton();
    reconnectWiFi();
    reconnectMQTT();

    if (currentMillis - lastTemperatureRead >= 2000) {
        lastTemperatureRead = currentMillis;

        float temperature = readTemperature();  
        int currentLevel = readLiquidLevel();    
        pumpState = isPumpOn();                 

        updateLEDs(temperature, currentLevel, pumpState);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("LIQUIDO: ");
        lcd.print(formatLiquidLevel(currentLevel));
        lcd.setCursor(0, 1);
        lcd.print("TEMPERATURA: ");
        lcd.print(temperature);
        lcd.print(" C");

        publishData(temperature, currentLevel, pumpState);
    }

    if (currentMillis - lastSystemInfoTime >= systemInfoInterval) {
        lastSystemInfoTime = currentMillis;
        publishSystemInfo();
    }

    client.loop();
}

void controlPump(bool state) {
    Serial.print("Controlando bomba para: ");
    Serial.println(state ? "Ligada" : "Desligada");

    if (emergencyMode) {
        digitalWrite(RELAY_PIN, state ? HIGH : LOW);
        Serial.println("Modo de emergência ativado.");
    } else {
        if (state && liquidLevel < MIN_LIQUID_LEVEL) {
            Serial.println("Nível de líquido abaixo do mínimo.");
            return; 
        }
        digitalWrite(RELAY_PIN, state ? HIGH : LOW);
        Serial.println(state ? "Modo normal: bomba ligada." : "Modo normal: bomba desligada.");
    }
    pumpState = state;
}

float readTemperature() {
    sensors.requestTemperatures();  
    delay(750);  
    float tempC = sensors.getTempCByIndex(0); 

    if (tempC == -127.00) { 
        Serial.println("⚠ Erro: Sensor desconectado!");
        return -1;  
    }
    return tempC; 
}

int readLiquidLevel() {
    float somaAlturas = 0;
    float alturaMaxAjustada = alturaMax + 0.28; 
    float alturaLiquidoCm;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        digitalWrite(TRIGGER_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIGGER_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIGGER_PIN, LOW);

        float tempoPulso = pulseIn(ECHO_PIN, HIGH);
        float distancia = tempoPulso / 58.0; 
        alturaLiquidoCm = alturaMaxAjustada - distancia;

        if (alturaLiquidoCm < 0) alturaLiquidoCm = 0;
        somaAlturas += alturaLiquidoCm;  
        delay(50);  
    }

    float alturaMedia = somaAlturas / NUM_SAMPLES;
    float volumeCm3 = PI * pow(raio, 2) * alturaMedia;  
    return static_cast<int>(volumeCm3);
}

void updateLEDs(float temperature, int currentLevel, bool pumpState) {
  digitalWrite(LED_VERMELHA1, currentLevel < LEVEL_THRESHOLD);
  digitalWrite(LED_VERDE1, currentLevel >= LEVEL_THRESHOLD);
  digitalWrite(LED_VERMELHA2, temperature >= TEMPERATURE_THRESHOLD);
  digitalWrite(LED_VERDE2, temperature < TEMPERATURE_THRESHOLD);
  digitalWrite(LED_VERMELHA3, !pumpState); 
  digitalWrite(LED_VERDE3, pumpState); 
}

String formatLiquidLevel(int currentLevel) {
  if (currentLevel >= 1000) {
    float liters = currentLevel / 1000.0;
    return String(liters, 3) + " L";
  } else {
    return String(currentLevel) + " ML";
  }
}

void publishData(float temperature, int currentLevel, bool pumpState) {
    if (temperature != -1) publishTemperature(temperature);
    publishLiquidLevel(currentLevel);
    publishPumpState();
}

void publishPumpState() {
    DynamicJsonDocument jsonDoc(128);
    jsonDoc["estado"] = isPumpOn() ? "Ligada" : "Desligada";
    jsonDoc["timestamp"] = getTimestamp();  
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    client.publish(TOPIC_PUMP_STATE, jsonString.c_str());
}

void publishLiquidLevel(int currentLevel) {
    DynamicJsonDocument jsonDoc(128);
    jsonDoc["liquidLevel"] = currentLevel;
    jsonDoc["timestamp"] = getTimestamp();  
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    client.publish(TOPIC_LIQUID_LEVEL, jsonString.c_str());
}

void publishTemperature(float temperature) {
    if (temperature == -1) return;
    DynamicJsonDocument jsonDoc(128);
    jsonDoc["temperature"] = String(temperature, 2).toFloat(); 
    jsonDoc["timestamp"] = getTimestamp();  
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    client.publish(TOPIC_TEMPERATURE, jsonString.c_str());
}

void publishSystemInfo() {
    int freeHeap = ESP.getFreeHeap();                 
    int flashSize = ESP.getFlashChipSize();           
    int flashFree = ESP.getFreeSketchSpace();         
    float internalTemp = temperatureRead();           

    unsigned long uptimeMillis = millis();
    unsigned long uptimeSecs = uptimeMillis / 1000;
    unsigned long uptimeMins = uptimeSecs / 60;
    unsigned long uptimeHours = uptimeMins / 60;

    String ssid = WiFi.SSID();                        
    String ip = WiFi.localIP().toString();            
    long rssi = WiFi.RSSI();                          
    bool wifiConnected = WiFi.isConnected();          
    int cpuUsage = 100 - ((freeHeap * 100) / ESP.getHeapSize());

    DynamicJsonDocument jsonDoc(256);
    jsonDoc["freeHeap"] = freeHeap;
    jsonDoc["flashSize"] = flashSize / 1024; 
    jsonDoc["flashFree"] = flashFree / 1024; 
    jsonDoc["internalTemp"] = internalTemp;
    jsonDoc["uptimeHours"] = uptimeHours;
    jsonDoc["uptimeMins"] = uptimeMins % 60;
    jsonDoc["uptimeSecs"] = uptimeSecs % 60;
    jsonDoc["ssid"] = ssid;
    jsonDoc["ip"] = ip;
    jsonDoc["rssi"] = rssi;
    jsonDoc["wifiConnected"] = wifiConnected;
    jsonDoc["cpuUsage"] = cpuUsage;

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    client.publish(TOPIC_SYSTEM_INFO, jsonString.c_str());
}

void checkEmergencyButton() {
    static bool lastButtonState = HIGH; 
    bool currentButtonState = digitalRead(EMERGENCY_BUTTON_PIN);

    if (lastButtonState == HIGH && currentButtonState == LOW) {
        emergencyMode = true;
        controlPump(false); 
    }
    if (lastButtonState == LOW && currentButtonState == HIGH) {
        if (emergencyMode) emergencyMode = false;
    }
    lastButtonState = currentButtonState;
}

String getTimestamp() {
    time_t now = timeClient.getEpochTime();  
    struct tm timeinfo;
    char buffer[80];
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    strcat(buffer, "-03:00");  
    return String(buffer);
}

bool getTimeFromApi() {
    const char* apiUrl = "http://worldtimeapi.org/api/timezone/America/Sao_Paulo";
    HTTPClient http;
    http.begin(apiUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument jsonDoc(2048);
        deserializeJson(jsonDoc, payload);
        String dateTimeStr = jsonDoc["datetime"];
        struct tm timeinfo;
        if (strptime(dateTimeStr.c_str(), "%Y-%m-%dT%H:%M:%S", &timeinfo) != NULL) {
            initialTime = mktime(&timeinfo);  
            initialMillis = millis();         
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}

bool isPumpOn() {
  return digitalRead(RELAY_PIN) == HIGH;
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
    String command = "";
    for (unsigned int i = 0; i < length; i++) {
        command += (char)payload[i];
    }
    
    DynamicJsonDocument jsonDoc(128);
    deserializeJson(jsonDoc, command);
    String action = jsonDoc["action"].as<String>();
    
    if (String(topic) == TOPIC_EMERGENCY_CONTROL) {
        if (action == "ligar") {
            emergencyMode = true;
            controlPump(true); 
        } else if (action == "desligar") {
            emergencyMode = false;
            controlPump(false); 
        }
    } 
    else if (String(topic) == TOPIC_PUMP_CONTROL_TEMP) {
        if (emergencyMode) return; 
        if (action == "ligar") {
            int currentLevel = readLiquidLevel();
            if (currentLevel >= 100) {
                controlPump(true); 
            }
        } else if (action == "desligar") {
            controlPump(false); 
        }
    }
}

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentatives < 20) { 
        delay(500);
        tentativas++;
    }
}

void reconnectWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }
    }
}

void reconnectMQTT() {
    if (!client.connected()) {
        while (!client.connected()) {
            String clientId = "ESP32Client_" + String(random(0xffff));  
            if (client.connect(clientId.c_str())) {
                client.subscribe(TOPIC_PUMP_CONTROL_TEMP);
                client.subscribe(TOPIC_EMERGENCY_CONTROL);
            } else {
                delay(10000); 
            }
        }
    }
}

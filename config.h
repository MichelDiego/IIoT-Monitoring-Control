/*
 * Copyright (c) 2026 Michel Diego. Todos os direitos reservados.
 * * Este código é de propriedade exclusiva de Michel Diego.
 * Proibida a reprodução, modificação ou uso comercial sem autorização prévia.
 * Desenvolvido para fins de portfólio técnico.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TimeLib.h>

// Definições de pinos
#define SDA_PIN 21
#define SCL_PIN 22
#define TRIGGER_PIN 4
#define ECHO_PIN 5
#define EMERGENCY_BUTTON_PIN 18
#define SENSOR_PIN 15
#define RELAY_PIN 19
#define LED_VERMELHA1 12
#define LED_VERDE1 13
#define LED_VERMELHA2 14
#define LED_VERDE2 25
#define LED_VERMELHA3 26
#define LED_VERDE3 27

#define NUM_SAMPLES 10  // Número de leituras para fazer a média
#define TEMPERATURE_THRESHOLD 40
#define MIN_LEVEL_THRESHOLD 100
#define LEVEL_THRESHOLD 500

// Definindo o nível mínimo de líquido necessário
const int MIN_LIQUID_LEVEL = 100;

// Definições dos tópicos MQTT
#define TOPIC_PUMP_STATE "esp32/pub/bombaestado"
#define TOPIC_PUMP_CONTROL_TEMP "esp32/sub/controletemp"
#define TOPIC_EMERGENCY_CONTROL "esp32/sub/controleemergencia"
#define TOPIC_TEMPERATURE "esp32/pub/temperatura"
#define TOPIC_LIQUID_LEVEL "esp32/pub/liquido"
#define TOPIC_SYSTEM_INFO "esp32/pub/systeminfo"

// Configurações de Wi-Fi (Protegido para o GitHub)
const char WIFI_SSID[] = "SUA_REDE_WIFI_AQUI";      
const char WIFI_PASSWORD[] = "SUA_SENHA_WIFI_AQUI";   

// Endereço do Broker MQTT (Protegido para o GitHub)
#define MQTT_BROKER_ADDRESS "192.168.X.X" 
#define MQTT_PORT 1883                     

extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

#endif // CONFIG_H

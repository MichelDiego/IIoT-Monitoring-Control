# Sistema de Monitoramento e Controle IIoT

Este repositório contém o código-fonte e o fluxo de integração de um sistema completo de Internet das Coisas Industrial (IIoT) desenvolvido para monitoramento preditivo e controle de fluidos (nível e temperatura). 

O projeto integra hardware embarcado na borda (Edge) com um middleware de processamento e tomada de decisão (Fog Computing), enviando dados para bancos de dados de séries temporais e relacionais.

## 📌 Arquitetura do Sistema

O ecossistema foi projetado com foco em resiliência, baixa latência e redundância de dados:

* **Hardware (Edge):** Microcontrolador ESP32 programado em C/C++ utilizando chamadas não-bloqueantes (`millis()`) para garantir processamento contínuo.
* **Comunicação:** Protocolo MQTT (Publish/Subscribe) operando sobre Wi-Fi.
* **Middleware (Fog):** Node-RED atuando como cérebro da rede, responsável por sanitizar payloads, executar lógicas de controle automático (emergência) e gerenciar o fluxo de dados.
* **Persistência de Dados (Dual Database):**
  * **MySQL:** Armazenamento relacional para logs de auditoria, eventos do sistema e informações estruturadas.
  * **InfluxDB:** Banco de dados de séries temporais otimizado para os dados dos sensores (temperatura e nível).
* **Interface (UI) e Alertas:** Dashboard interativo com componentes customizados em Vue.js via Node-RED e integração para alertas.

## 🛠️ Tecnologias e Bibliotecas Utilizadas

**Embarcados:**
* C++ (Arduino Core)
* `PubSubClient` (Comunicação MQTT)
* `ArduinoJson` (Serialização e Desserialização de pacotes)
* `NTPClient` e requisições HTTP REST (Sincronização de relógio de tempo real - RTC)
* `DallasTemperature` e `OneWire` (Leitura do sensor DS18B20)

**Infraestrutura e Servidor:**
* Node-RED
* Broker MQTT (Mosquitto/Local)
* Bancos de Dados: MySQL e InfluxDB (v2.0)

## 🚀 Principais Funcionalidades

1. **Monitoramento em Tempo Real:** Leitura constante de volume de líquido (sensor ultrassônico calibrado) e temperatura, convertendo dados brutos em porcentagem e graus Celsius.
2. **Telemetria do Sistema:** O ESP32 publica sua própria saúde operacional a cada minuto (Uptime, RAM Livre, Temperatura do Chip, Força do Sinal Wi-Fi - RSSI).
3. **Controle Bidirecional:** A bomba de fluidos pode ser acionada remotamente pelo painel ou fisicamente pelo botão de emergência no hardware.
4. **Automação de Malha Fechada:** O Node-RED avalia constantemente as variáveis. Se a temperatura ultrapassar 40 graus e houver nível mínimo (100 ml), o sistema aciona comandos automáticos de resfriamento (ligar a bomba).
5. **Fallback de Sincronização:** Sistema tolerante a falhas na obtenção de horário. Tenta buscar via servidor NTP (pool.ntp.org) e, em caso de falha, aciona uma API HTTP secundária.

## ⚙️ Como Funciona o Fluxo MQTT

O sistema utiliza tópicos bem definidos para separar a publicação de dados da subscrição de comandos:

* **Publicação (ESP32 -> Node-RED):**
  * `esp32/pub/temperatura`: Dados térmicos formatados.
  * `esp32/pub/liquido`: Dados de volume lidos pelo sensor ultrassônico.
  * `esp32/pub/bombaestado`: Retorno de estado (feedback de que o relé realmente atracou).
  * `esp32/pub/systeminfo`: Dados de saúde do microcontrolador.
* **Subscrição (Node-RED -> ESP32):**
  * `esp32/sub/controletemp`: Recebe comandos automáticos ou manuais da dashboard.
  * `esp32/sub/controleemergencia`: Recebe sinais para sobrepor as lógicas padrões de funcionamento.

## 📸 Demonstração

[![Google Drive](https://img.shields.io/badge/-Repositório_em_Campo_(Google_Drive)-0D1117?style=for-the-badge&logo=googledrive&logoColor=34A853)](https://drive.google.com/drive/u/1/folders/1CbQxDUkJ3atlZN0r8CcHZN9Mul5aLsuT)

## ⚠️ Instruções para Uso

Para replicar este projeto localmente, certifique-se de configurar as credenciais no arquivo `config.h`. 

```cpp
// Configurações de Wi-Fi e Broker
const char WIFI_SSID[] = "NOME_DA_SUA_REDE";
const char WIFI_PASSWORD[] = "SENHA_DA_REDE";
#define MQTT_BROKER_ADDRESS "IP_DO_SEU_BROKER"

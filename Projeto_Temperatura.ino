// --- Bibliotecas ---
#include <SoftwareSerial.h>

// RX pino 2, TX pino 3
SoftwareSerial esp8266(2, 3);

// Definições
#define DEBUG true
#define ntcPin A0  // Pino analógico para o NTC

// Variáveis para leitura do NTC
float vout = 0.0;
float Rntc = 0.0;
float TempC = 0.0;

// URL do seu Google Apps Script
const String googleScriptUrl = "https://script.google.com/macros/s/AKfycbxKGF6DpXgVrBlGzc1lOYPBu2T8eEcGJCuXtPujMjQo2MMkYUcSlXEinLjSaHz-7OAOfA/exec?temp=";

// =============================================================================================================
// --- Protótipo das Funções Auxiliares ---
String sendData(String command, const int timeout, boolean debug);

// =============================================================================================================
// --- Configurações Iniciais ---
void setup()
{
    Serial.begin(9600);
    esp8266.begin(9600);
    
    Serial.println("Iniciando ESP8266 e conectando ao Wi-Fi...");
    
    // Reiniciar o ESP8266
    String resetResponse = sendData("AT+RST\r\n", 3000, DEBUG);
    delay(2000);
    Serial.println(resetResponse);
    
    // Conectar à rede Wi-Fi
    String wifiCmd = "AT+CWJAP=\"Tek Fibra - Roberto\",\"1234509876\"\r\n";
    //String wifiCmd = "AT+CWJAP=\"WIFI-IOT\",\"\"\r\n";
    String response = sendData(wifiCmd, 5000, DEBUG);
    delay(5000);

    // Verifique se a conexão foi bem-sucedida
    if (response.indexOf("WIFI GOT IP") != -1) {
        Serial.println("Conectado à rede Wi-Fi com sucesso!");
    } else {
        Serial.println("Falha ao conectar à rede Wi-Fi.");
        return;  // Para a execução se a conexão falhar
    }

    // Verifica o endereço IP
    String ipResponse = sendData("AT+CIFSR\r\n", 2000, DEBUG);
    Serial.println(ipResponse);
}

// =============================================================================================================
// --- Loop Infinito ---
void loop()
{
    // Leitura do sensor NTC
    int valorAnalogico = analogRead(ntcPin);  // Lê o valor analógico
    vout = valorAnalogico * (5.0 / 1023.0);  // Converte para tensão
    Rntc = (10000 * vout) / (5.0 - vout);    // Calcula a resistência do NTC
            
    // Cálculo da temperatura
    TempC = 1 / (0.001129148 + (0.000234125 * log(Rntc)) + (0.0000000876741 * pow(log(Rntc), 3))) - 273.15;

    Serial.print("Temperatura: ");
    Serial.print(TempC);
    Serial.println(" °C");

    // Enviar dados para o Google Sheets
    String url = googleScriptUrl + String(TempC);
    Serial.println("Enviando dados para: " + url); // Adicionada para debug

    // Inicia a conexão HTTP
    String startCommand = "AT+CIPSTART=\"TCP\",\"script.google.com\",443\r\n"; // Conectar ao host correto
    String startResponse = sendData(startCommand, 5000, DEBUG); // Tempo de resposta ajustado
    delay(5000);  // Atraso aumentado para garantir que a conexão seja estabelecida

    // Preparar e enviar o comando AT+CIPSEND
    String httpRequest = "GET /macros/s/AKfycbxKGF6DpXgVrBlGzc1lOYPBu2T8eEcGJCuXtPujMjQo2MMkYUcSlXEinLjSaHz-7OAOfA/exec?temp=" + String(TempC) + " HTTP/1.1\r\nHost: script.google.com\r\nConnection: close\r\n\r\n";
    int requestLength = httpRequest.length(); // Calculando o comprimento da requisição corretamente
    String sendCommand = "AT+CIPSEND=" + String(requestLength) + "\r\n";  // Enviar o comprimento correto
    sendData(sendCommand, 1000, DEBUG);
    delay(5000);  // Atraso aumentado após o CIPSEND

    // Enviar a requisição HTTP
    Serial.print("HTTP Request: ");
    Serial.println(httpRequest);  // Debug da requisição HTTP
    sendData(httpRequest, 5000, DEBUG);  // Aumentado o timeout para o envio da requisição
    delay(5000);  // Atraso para garantir resposta do servidor

    // Ler a resposta do servidor
    String response = "";
    while (esp8266.available()) {
        char c = esp8266.read();
        response += c;
    }
    Serial.print("Resposta do servidor: ");
    Serial.println(response); // Adicionada para debug

    // Fechar a conexão
    sendData("AT+CIPCLOSE\r\n", 1000, DEBUG);
    delay(30000);  // Espera 30 segundos antes da próxima leitura
}

// --- Função Auxiliar para Envio de Comandos AT ---
String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    esp8266.print(command);
    long int time = millis();

    while ((time + timeout) > millis())
    {
        while (esp8266.available())
        {
            char c = esp8266.read();
            response += c;
        }
    }

    if (debug) {
        Serial.print(response);
    }

    return response;
}

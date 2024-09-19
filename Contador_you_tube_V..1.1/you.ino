#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <YoutubeApi.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

#define API_KEY "AIzaSyDqlHJnE_93B3mF7ElB19uZ1jEqwoiM0J0" // API key do YouTube
#define CHANNEL_ID "UCzil8HZREYfslMpU7VNv28w" // ID do canal 
#define WEATHER_API_KEY "035cc27ca5ac27b05c80a10925f86716" // API key do OpenWeatherMap
#define CITY_ID "3457359" // ID da cidade (São Paulo, Brasil por exemplo)

String utf8ToAscii(String str) {
  str.replace("ç", "c");
  str.replace("á", "a");
  str.replace("ã", "a");
  str.replace("â", "a");
  str.replace("é", "e");
  str.replace("ê", "e");
  str.replace("í", "i");
  str.replace("ó", "o");
  str.replace("õ", "o");
  str.replace("ô", "o");
  str.replace("ú", "u");
  str.replace("ü", "u");
  str.replace("Á", "A");
  str.replace("Ã", "A");
  str.replace("Â", "A");
  str.replace("É", "E");
  str.replace("Ê", "E");
  str.replace("Í", "I");
  str.replace("Ó", "O");
  str.replace("Õ", "O");
  str.replace("Ô", "O");
  str.replace("Ú", "U");
  str.replace("Ü", "U");
  return str;
}


char ssid[] = "WELINTON";         // Nome da sua rede WiFi
char password[] = "petter9125"; // Senha da sua rede WiFi

LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do LCD

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);

unsigned long timeBetweenRequests = 60000; // 60 segundos entre cada requisição
unsigned long nextRunTime;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;  // Offset de tempo para GMT-3 (Horário de Brasília)
const int daylightOffset_sec = 3600;  // Ajuste para horário de verão (se aplicável)

// Função para obter a previsão do tempo
void getWeather() {
  WiFiClient client;
  const char* server = "api.openweathermap.org";
  // Adiciona o parâmetro lang=pt_br para a descrição do clima em português
  String url = "/data/2.5/weather?id=" + String(CITY_ID) + "&units=metric&lang=pt_br&appid=" + WEATHER_API_KEY;

  if (client.connect(server, 80)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" + 
                 "Connection: close\r\n\r\n");
  } else {
    Serial.println("Erro ao conectar ao servidor de clima.");
    return;
  }

  // Aguarda a resposta do servidor
  while (client.connected() && !client.available()) {
    delay(10);
  }

  // Ignora os cabeçalhos HTTP até encontrar uma linha vazia
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Lê o corpo da resposta (JSON)
  String payload = client.readString();
  client.stop();

  // Converte a resposta JSON para um objeto manipulável
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Erro ao analisar JSON: ");
    Serial.println(error.c_str());
    Serial.println("Payload recebido:");
    Serial.println(payload);  // Imprime o payload recebido para depuração
    return;
  }

  // Extrai os dados do JSON
  const char* cityName = doc["name"]; // Nome da cidade
  float temp = doc["main"]["temp"];   // Temperatura em Celsius
  const char* description = doc["weather"][0]["description"]; // Descrição do clima (em português agora)

  // Exibe a previsão do tempo no LCD
  lcd.clear();
  
  // Exibe o nome da cidade na primeira linha
  lcd.setCursor(3, 0);
  lcd.print(cityName); // Exibe o nome da cidade
  
  // Exibe a temperatura na segunda linha
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temp);    // Exibe a temperatura em Celsius
  lcd.print(" C");

  // Aguarda 5 segundos para que a temperatura seja exibida
  delay(2000);

// Exibe a descrição do clima na segunda linha
lcd.clear();
lcd.setCursor(2, 0);
lcd.print("Clima Atual: ");
lcd.setCursor(0, 1);
lcd.print(utf8ToAscii(String(description))); // Converte e exibe a descrição do clima

  // Exibe no terminal serial para depuração
  Serial.println("Previsão do tempo:");
  Serial.print("Cidade: ");
  Serial.println(cityName);
  Serial.print("Temperatura: ");
  Serial.println(temp);
  Serial.print("Descrição: ");
  Serial.println(description);

  delay(2000); // Exibe por 5 segundos
}



void setup() {
  Wire.begin(4, 5); // SDA = GPIO 4, SCL = GPIO 5 (ajustado para o ESP8266)
  lcd.begin(16, 2); // Inicializa o LCD com 16 colunas e 2 linhas
  lcd.backlight();  // Ativa a luz de fundo do LCD

  Serial.begin(115200); // Inicializa a comunicação serial

  // Configura o WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Conectando ao WiFi
  lcd.setCursor(0, 0);
  lcd.print("Conectando:");
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.print(".");
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectado!");

  // Se estiver usando ESP8266 Core 2.5 RC, descomente a linha a seguir
  client.setInsecure();

  // Configura o NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void showDateTime() {
  // Obtém a hora atual
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro de tempo");
    Serial.println("Erro ao obter tempo.");
    return;
  }

  // Exibe data e hora no LCD
  char timeStr[16]; // Buffer para armazenar a string da hora
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print(timeStr);

  char dateStr[16]; // Buffer para armazenar a string da data
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
  lcd.setCursor(3, 1);
  lcd.print(dateStr);

  delay(1000); // Exibe a data e hora por 2 segundos antes de continuar
}

void getYoutubeSubscribers() {
  // Faz a requisição para obter as estatísticas do canal
  if (api.getChannelStatistics(CHANNEL_ID)) {

     lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Canal");
    
    lcd.setCursor(4, 1);
    lcd.print("You Tube");
    delay(2000); // Aguarda 2 segundos antes de exibir o próximo dado
    // Exibe subscritores no LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Inscritos:");
    lcd.setCursor(11, 0);
    lcd.print(api.channelStats.subscriberCount);

    // Exibe subscritores no terminal serial
    Serial.println("Informações do Canal YouTube:");
    Serial.print("Inscritos: ");
    Serial.println(api.channelStats.subscriberCount);

    

    // Exibe visualizações no LCD
    //lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Views:");
    lcd.setCursor(7, 1);
    lcd.print(api.channelStats.viewCount);



    
    // Exibe visualizações no terminal serial
    Serial.print("Views: ");
    Serial.println(api.channelStats.viewCount);
  } else {
    // Se houver erro, exibe mensagem genérica
    Serial.println("Erro ao obter dados do canal.");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro ao obter");
    lcd.setCursor(0, 1);
    lcd.print("dados do canal.");
  }
}

void loop() {
  // Verifica se é hora de buscar os dados novamente
  if (millis() > nextRunTime) {
    showDateTime(); // Exibe a data e hora antes de mostrar os dados do YouTube
    getWeather(); // Exibe a previsão do tempo antes dos dados do YouTube
    getYoutubeSubscribers();
    nextRunTime = millis() + timeBetweenRequests;
  }
}

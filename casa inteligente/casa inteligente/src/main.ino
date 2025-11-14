#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include "config.h"

// --- Instâncias ---
WebServer server(80);
DHT dht(PIN_DHT, DHTTYPE);

// Estado do sistema
bool lightState = false;
bool fanState = false;
bool fanAutoMode = true;
bool pirState = false;
unsigned long lastStatusMillis = 0;
float lastTemp = 0.0;
float lastHum = 0.0;

// ---- HTML simples (página) ----
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Casa Inteligente - ESP32</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body{font-family: Arial; margin: 20px;}
      button{padding:10px 20px; margin:8px;}
      .card{border:1px solid #ddd; padding:12px; border-radius:6px; margin-bottom:12px;}
    </style>
  </head>
  <body>
    <h2>Casa Inteligente - ESP32</h2>
    <div class="card">
      <h3>Iluminação</h3>
      <button onclick="fetch('/toggleLight').then(()=>update())">Alternar Luz</button>
      <div id="light"></div>
    </div>
    <div class="card">
      <h3>Temperatura e Ventilação</h3>
      <div>Temperatura: <span id="temp"></span> °C</div>
      <div>Umidade: <span id="hum"></span> %</div>
      <div>Ventilador: <span id="fan"></span></div>
      <button onclick="fetch('/setFanOn').then(()=>update())">Forçar Ligar Ventilador</button>
      <button onclick="fetch('/setFanOff').then(()=>update())">Forçar Desligar Ventilador</button>
      <button onclick="fetch('/resetFanAuto').then(()=>update())">Voltar ao Automático</button>
    </div>
    <div class="card">
      <h3>Segurança</h3>
      <div>Sensor PIR (movimento): <span id="pir"></span></div>
      <div id="alarm"></div>
    </div>

    <script>
      async function update(){
        const res = await fetch('/status');
        const data = await res.json();
        document.getElementById('light').innerText = data.light ? 'Ligada' : 'Desligada';
        document.getElementById('temp').innerText = data.temp.toFixed(1);
        document.getElementById('hum').innerText = data.hum.toFixed(1);
        document.getElementById('fan').innerText = data.fan ? 'Ligado' : 'Desligado';
        document.getElementById('pir').innerText = data.pir ? 'Movimento detectado' : 'Sem movimento';
        document.getElementById('alarm').innerText = data.pir ? 'ALERTA: Intruso!' : '';
      }
      setInterval(update, 2000);
      window.onload = update;
    </script>
  </body>
</html>
)rawliteral";

// ---- Funções auxiliares ----
void handleRoot(){
  server.sendHeader("Content-Type", "text/html; charset=utf-8");
  server.send(200, "text/html", index_html);
}

void handleToggleLight(){
  lightState = !lightState;
  digitalWrite(PIN_LIGHT, lightState ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void handleSetFanOn(){
  fanAutoMode = false;
  fanState = true;
  digitalWrite(PIN_FAN, fanState ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void handleSetFanOff(){
  fanAutoMode = false;
  fanState = false;
  digitalWrite(PIN_FAN, fanState ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void handleResetFanAuto(){
  fanAutoMode = true;
  server.send(200, "text/plain", "OK");
}

void handleStatus(){
  char buff[256];
  // simples JSON
  snprintf(buff, sizeof(buff),
           "{\"light\":%d,\"temp\":%.2f,\"hum\":%.2f,\"fan\":%d,\"pir\":%d}",
           lightState ? 1 : 0, lastTemp, lastHum, fanState ? 1 : 0, pirState ? 1 : 0);
  server.send(200, "application/json", buff);
}

void setup(){
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== Projeto Casa Inteligente - ESP32 ===");

  // pinos
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_PIR, INPUT);

  digitalWrite(PIN_LIGHT, LOW);
  digitalWrite(PIN_FAN, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  // sensor DHT
  dht.begin();

  // conecta WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Falha ao conectar. Reinicie ou verifique as credenciais.");
  }

  // rotas do servidor
  server.on("/", handleRoot);
  server.on("/toggleLight", handleToggleLight);
  server.on("/setFanOn", handleSetFanOn);
  server.on("/setFanOff", handleSetFanOff);
  server.on("/resetFanAuto", handleResetFanAuto);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop(){
  server.handleClient();

  unsigned long now = millis();
  if (now - lastStatusMillis >= STATUS_UPDATE_INTERVAL) {
    // Leitura do DHT (pode falhar ocasionalmente)
    float t = NAN, h = NAN;
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (isnan(t) || isnan(h)) {
      Serial.println("Falha leitura DHT");
    } else {
      lastTemp = t;
      lastHum = h;
    }

    // Leitura PIR
    int pirVal = digitalRead(PIN_PIR);
    bool newPir = (pirVal == HIGH);
    if (newPir && !pirState){
      // houve detecção agora
      Serial.println("Movimento detectado!");
      digitalWrite(PIN_BUZZER, HIGH); // alarma curto
      delay(200);
      digitalWrite(PIN_BUZZER, LOW);
    }
    pirState = newPir;

    // Lógica do ventilador (modo automático)
    if (fanAutoMode) {
      if (!isnan(lastTemp)) {
        if (lastTemp >= TEMP_THRESHOLD) {
          fanState = true;
        } else {
          fanState = false;
        }
        digitalWrite(PIN_FAN, fanState ? HIGH : LOW);
      }
    }

    lastStatusMillis = now;
  }
}
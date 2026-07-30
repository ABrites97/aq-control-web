#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <OneWire.h>
#include <RTClib.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

// ===============================
// WIFI
// ===============================

const char* ssid = "Brites Silva";
const char* password = "SilvaBrites";


// ===============================
// RTC
// ===============================

RTC_DS3231 rtc;

// ===============================
// DS18B20
// ===============================

#define ONE_WIRE_BUS 25   // DS18B20 (duas sondas no mesmo fio)
#define RELE1_PIN 17      // Relé 1
#define RELE2_PIN 16      // Relé 2
#define RELE3_PIN 27      // Relé 3

#define TEMP1_LIGA 50.0
#define TEMP1_DESLIGA 49.0

#define TEMP2_LIGA 61.0
#define TEMP2_DESLIGA 60

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensores(&oneWire);

float tempCaldeira = 0;
float tempAQS = 0;

//-----------------------
// ESTADO RELES
//-----------------------
bool rele1Ligado = false;   // inicia desligado
bool rele2Ligado = false;   // inicia desligado
bool rele3Ligado = false;   // inicia desligado


// ===============================
// SERVIDOR
// ===============================

WebServer server(80);


// Variáveis da página

String horaAtual;
String dataAtual;



// ===============================
// HTML
// ===============================

String pagina(){


String html = R"rawliteral(

<!DOCTYPE html>

<html>

<script>

function atualizarHora(){

fetch('/hora')
.then(res => res.text())
.then(dados => {

let partes = dados.split("|");

document.getElementById("hora").innerHTML = partes[0];

document.getElementById("data").innerHTML = partes[1];

});

}


setInterval(atualizarHora,1000);

</script>


<head>


<script>

function atualizarTemperaturas(){
fetch('/temperaturas')
.then(resposta => resposta.text())
.then(dados => {
let valores = dados.split("|");

document.getElementById("tempCaldeira").innerHTML =
valores[0] + " °C";

document.getElementById("tempAQS").innerHTML =
valores[1] + " °C";

});

}

setInterval(atualizarTemperaturas,1000);

</script>



<meta charset="UTF-8">

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>AQ-CONTROL</title>


<style>

body{

background:#121212;
font-family:Arial;
color:white;
text-align:center;

}


.card{

background:#1e293b;
margin:20px;
padding:20px;
border-radius:15px;

}


.valor{

font-size:35px;
color:#00e676;

}


h1{

color:#ff9800;

}

</style>

</head>


<body>


<h1>🔥 AQ-CONTROL</h1>


<div class="card">

<h2>🕒 Data e Hora</h2>


<div id="hora" class="valor">

)rawliteral";

html += horaAtual;

html += R"rawliteral(

</div>


<p id="data">

)rawliteral";

html += dataAtual;

html += R"rawliteral(

</p>


</div>




<div class="card">

<h2>Modo</h2>

<h2>🟦 INVERNO</h2>

</div>



<div class="card">

<h2>🌡 Temperaturas</h2>

<p>🔥 Caldeira</p>

<div id="tempCaldeira" class="valor">

)rawliteral";

html += String(tempCaldeira,2);

html += R"rawliteral(

 °C

</div>


<p>🚿 AQS</p>

<div id="tempAQS" class="valor">

)rawliteral";

html += String(tempAQS,2);

html += R"rawliteral(

 °C

</div>




<div class="card">

<h2>⚡ Saídas</h2>

<p>Contacto Caldeira 🔴</p>

<p>Bomba Caldeira 🔴</p>

<p>Bomba Casa 🔴</p>


</div>



<button>ON</button>

<button>INVERNO</button>

<button>VERÃO</button>

<button>OFF</button>





</body>

</html>

)rawliteral";


return html;

}



// ===============================
// ATUALIZAR RTC
// ===============================

void lerRTC(){

DateTime agora = rtc.now();


char hora[10];

sprintf(hora,"%02d:%02d",  //sprintf(hora,"%02d:%02d:%02d",
agora.hour(),
agora.minute(),
agora.second());


horaAtual = String(hora);



char data[12];

sprintf(data,"%02d/%02d/%04d",
agora.day(),
agora.month(),
agora.year());


dataAtual = String(data);

}

// ===============================
// DS18B20
// ===============================

//void lerTemperaturas(){

//sensores.requestTemperatures();

//tempCaldeira = sensores.getTempCByIndex(0);

//tempAQS = sensores.getTempCByIndex(1);

//}

// ===============================
// SETUP
// ===============================

void setup(){

server.on("/hora", [](){

lerRTC();

String resposta = horaAtual + "|" + dataAtual;

server.send(200, "text/plain", resposta);

});

Serial.begin(115200);


// I2C

//Wire.begin(21,22);
Wire.begin();

if(!rtc.begin()){
Serial.println("RTC não encontrado!");
while(1);

}


if(rtc.lostPower()){

Serial.println("RTC sem hora. A ajustar...");

// data/hora do computador no momento da compilação

rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}

sensores.begin();

Serial.println("DS18B20 iniciado");


WiFi.begin(ssid,password);


Serial.print("WiFi");


while(WiFi.status()!=WL_CONNECTED){

delay(500);

Serial.print(".");

}


Serial.println();

Serial.println("WiFi ligado");


Serial.print("IP:");

Serial.println(WiFi.localIP());



server.on("/", [](){

lerRTC();

server.send(200,
"text/html; charset=utf-8",
pagina());


});

server.on("/temperaturas", [](){

lerTemperaturas();

String resposta = 
String(tempCaldeira,2)
+
"|"
+
String(tempAQS,2);

server.send(200,"text/plain",resposta);


});

//-----------------------
// RELES
//-----------------------

  pinMode(RELE1_PIN, OUTPUT);
  pinMode(RELE2_PIN, OUTPUT);
  pinMode(RELE3_PIN, OUTPUT);

  // Relé desligado no arranque
  // Este módulo é ativo LOW:
  digitalWrite(RELE1_PIN, HIGH);
  digitalWrite(RELE2_PIN, HIGH);
  digitalWrite(RELE3_PIN, HIGH);



server.begin();


Serial.println("AQ-CONTROL online");


Serial.println("Reles iniciados");

}

// ===============================
// LOOP
// ===============================

void loop(){

lerTemperaturas();

server.handleClient();

delay(1000);

// -----------------------------
  // CONTROLO DO RELÉ 2
  // Usa a Sonda 1 (temp1)
  // -----------------------------

  if (!rele2Ligado && tempCaldeira >= TEMP1_LIGA) {

    rele2Ligado = true;

    digitalWrite(RELE2_PIN, LOW);

    Serial.println("RELE LIGADO");
  }


  if (rele2Ligado && tempCaldeira <= TEMP1_DESLIGA) {

    rele2Ligado = false;

    digitalWrite(RELE2_PIN, HIGH);

    Serial.println("RELE DESLIGADO");
  }

  // -----------------------------
  // CONTROLO DO RELÉ 3
  // Usa a Sonda 2 (temp2)
  // -----------------------------

  if (!rele3Ligado && tempAQS >= TEMP2_LIGA) {

    rele3Ligado = true;

    digitalWrite(RELE3_PIN, LOW);

    Serial.println("RELE LIGADO");
  }


  if (rele3Ligado && tempAQS <= TEMP2_DESLIGA) {

    rele3Ligado = false;

    digitalWrite(RELE3_PIN, HIGH);

    Serial.println("RELE DESLIGADO");
  }


}

void lerTemperaturas(){

sensores.requestTemperatures();

tempCaldeira = sensores.getTempCByIndex(0);

tempAQS = sensores.getTempCByIndex(1);


Serial.print("Caldeira: ");
Serial.println(tempCaldeira);


Serial.print("AQS: ");
Serial.println(tempAQS);

}

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

<head>

<meta charset="UTF-8">

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>AQ-CONTROL</title>


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



function atualizarTemperaturas(){

fetch('/temperaturas')

.then(res => res.text())

.then(dados => {

let valores = dados.split("|");


document.getElementById("tempCaldeira").innerHTML =
valores[0] + " °C";


document.getElementById("tempAQS").innerHTML =
valores[1] + " °C";


});

}




function atualizarReles(){

fetch('/reles')

.then(res => res.text())

.then(dados => {


let r = dados.split("|");


estadoRele("rele1",r[0]);

estadoRele("rele2",r[1]);

estadoRele("rele3",r[2]);


});

}



function estadoRele(id,estado){


let elemento = document.getElementById(id);


if(estado=="1"){

elemento.innerHTML="● ON";
elemento.className="ligado";

}

else{

elemento.innerHTML="● OFF";
elemento.className="desligado";

}


}



window.onload=function(){


atualizarHora();

atualizarTemperaturas();

atualizarReles();



setInterval(atualizarHora,1000);

setInterval(atualizarTemperaturas,1000);

setInterval(atualizarReles,1000);


};


</script>




<style>


body{

background:#111827;

font-family:Arial, sans-serif;

color:white;

text-align:center;

margin:0;

}

.rele{

display:flex;
justify-content:center;
align-items:center;
gap:12px;

font-size:20px;

margin:15px;

}

h1{

background:#0f172a;

padding:20px;

margin:0;

color:#ff9800;

}



.card{

background:#1e293b;
margin:15px auto;
padding:20px;
border-radius:18px;
width:90%;
max-width:420px;
text-align:center;

}



.titulo{

font-size:22px;

margin-bottom:15px;

}



.valor{

font-size:38px;

font-weight:bold;

color:#00e676;

}



.data{

font-size:20px;

color:#cbd5e1;

}



.linha{

display:flex;
justify-content:center;
align-items:center;
text-align:center;
gap:10px;

font-size:22px;
margin:15px;

}



.ligado{

color:#00ff00;

font-weight:bold;

}



.desligado{

color:#ff3333;

font-weight:bold;

}



button{


background:#334155;

color:white;

border:0;

padding:12px;

margin:5px;

border-radius:10px;

font-size:16px;

}



button:hover{

background:#ff9800;

}



</style>


</head>



<body>


<h1>🔥 AQ-CONTROL</h1>



<div class="card">


<div class="titulo">

🕒 Data e Hora

</div>


<div id="hora" class="valor">

)rawliteral";


html += horaAtual;


html += R"rawliteral(

</div>


<div id="data" class="data">

)rawliteral";


html += dataAtual;


html += R"rawliteral(

</div>


</div>





<div class="card">


<div class="titulo">

⚙️ Modo Caldeira

</div>



<button>ON</button>

<button>INVERNO</button>

<button>VERÃO</button>

<button>OFF</button>


</div>





<div class="card">

<div class="titulo">
🌡 Temperaturas
</div>


<div class="linha">

<span>🔥 Caldeira:</span>

<span id="tempCaldeira" class="temperatura">
52.0 °C
</span>

</div>


<div class="linha">

<span>🚿 AQS:</span>

<span id="tempAQS" class="temperatura">
60.0 °C
</span>

</div>


</div>







<div class="card">

<div class="titulo">
⚡ Saídas
</div>


<div class="rele">

<span>Contacto Caldeira:</span>

<span id="rele1" class="desligado">
🔴 OFF
</span>

</div>


<div class="rele">

<span>Bomba Caldeira:</span>

<span id="rele2" class="desligado">
🔴 OFF
</span>

</div>


<div class="rele">

<span>Bomba Casa:</span>

<span id="rele3" class="desligado">
🔴 OFF
</span>

</div>


</div>




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


// ===============================
// WIFI COM IP FIXO
// ===============================

IPAddress local_IP(192,168,68,200);
IPAddress gateway(192,168,68,1);
IPAddress subnet(255,255,252,0);


Serial.println("A configurar IP fixo...");


if (!WiFi.config(local_IP, gateway, subnet)) {

  Serial.println("Erro a configurar IP fixo");

}


WiFi.begin(ssid, password);


Serial.print("WiFi");


while (WiFi.status() != WL_CONNECTED) {

  delay(500);

  Serial.print(".");

}


Serial.println();

Serial.println("WiFi ligado");


Serial.print("IP: ");

Serial.println(WiFi.localIP());

// ===============================

Serial.print("WiFi");


while(WiFi.status()!=WL_CONNECTED){

delay(500);

Serial.print(".");

}


Serial.println();

Serial.println("WiFi ligado");


Serial.print("IP:");

Serial.println(WiFi.localIP());
Serial.print("Gateway: ");
Serial.println(WiFi.gatewayIP());

Serial.print("Mascara: ");
Serial.println(WiFi.subnetMask());



server.on("/", [](){

lerRTC();

server.send(200,"text/html; charset=utf-8",pagina());

});

server.on("/temperaturas", [](){

lerTemperaturas();

String resposta = 
String(tempCaldeira,2) + "|" +
String(tempAQS,2);

server.send(200,"text/plain",resposta);

});


//-----------------------
// RELES
//-----------------------
server.on("/reles", [](){

  String resposta = 
  String(rele1Ligado) + "|" +
  String(rele2Ligado) + "|" +
  String(rele3Ligado);

  server.send(200,"text/plain",resposta);

});


  pinMode(RELE1_PIN, OUTPUT);
  pinMode(RELE2_PIN, OUTPUT);
  pinMode(RELE3_PIN, OUTPUT);

  // Relé desligado no arranque
  // Este módulo é ativo LOW:
  digitalWrite(RELE1_PIN, HIGH);
  digitalWrite(RELE2_PIN, HIGH);
  digitalWrite(RELE3_PIN, HIGH);


// Arranca servidor

server.begin();

Serial.println("AQ-CONTROL online");

Serial.println("Reles iniciados");

}

// ===============================
// LOOP
// ===============================

void loop(){

    server.handleClient();

    static unsigned long ultimo = 0;

    if (millis() - ultimo >= 1000){

        ultimo = millis();

        lerTemperaturas();

}

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


//Serial.print("Caldeira: ");
//Serial.println(tempCaldeira);


//Serial.print("AQS: ");
//Serial.println(tempAQS);

}

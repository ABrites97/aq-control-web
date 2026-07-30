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
#define TEMP2_DESLIGA 60.0

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


//-----------------------
// HORARIOS
//-----------------------
int inicioInvernoH = 6,  inicioInvernoM = 0;
int fimInvernoH    = 22, fimInvernoM    = 0;

int inicioVeraoH = 7,  inicioVeraoM = 0;
int fimVeraoH    = 20, fimVeraoM    = 0;


// ===============================
// MODOS
// ===============================

enum ModoSistema {
  MODO_OFF,
  MODO_ON,
  MODO_INVERNO,
  MODO_VERAO
};

String modoAtual = "INVERNO";

//ModoSistema modoAtual = MODO_INVERNO;


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

elemento.innerHTML="🟢 ON";
elemento.className="ligado";

}

else{

elemento.innerHTML="🔴 OFF";
elemento.className="desligado";

}


}


// ============================
// MUDAR MODO
// ============================

function mudarModo(modo){

fetch('/modo?valor=' + modo)

.then(res => res.text())

.then(resposta => {

mostrarModo(resposta);

});

}

window.onload=function(){


atualizarHora();

atualizarTemperaturas();

atualizarReles();

mostrarModo("INVERNO");


setInterval(atualizarHora,1000);

setInterval(atualizarTemperaturas,1000);

setInterval(atualizarReles,1000);


};


function mostrarModo(modo){

// retirar o brilho de todos

document.getElementById("modoON").classList.remove("ativo");
document.getElementById("modoInverno").classList.remove("ativo");
document.getElementById("modoVerao").classList.remove("ativo");
document.getElementById("modoOFF").classList.remove("ativo");


// acender o modo escolhido

if(modo=="ON"){

document.getElementById("modoON").classList.add("ativo");

}


if(modo=="INVERNO"){

document.getElementById("modoInverno").classList.add("ativo");

}


if(modo=="VERAO"){

document.getElementById("modoVerao").classList.add("ativo");

}


if(modo=="OFF"){

document.getElementById("modoOFF").classList.add("ativo");

}

}


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

font-size:18px;
padding:12px 15px;
margin:5px;
border-radius:12px;
border:0;
background:#334155;
color:white;
cursor:pointer;

}


button:hover{

background:#ff9800;

}


/* modo selecionado */

.ativo{

background:#ff9800 !important;
color:white;

box-shadow:0 0 12px #ff9800;

}

.modos{

display:flex;
justify-content:center;
align-items:center;
flex-wrap:nowrap;
gap:5px;

}


.modos button{

font-size:18px;
padding:10px 14px;
margin:3px;

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



<div class="modos">

<button id="modoON" onclick="mudarModo('ON')">
ON
</button>


<button id="modoInverno" onclick="mudarModo('INVERNO')">
INVERNO
</button>


<button id="modoVerao" onclick="mudarModo('VERAO')">
VERÃO
</button>


<button id="modoOFF" onclick="mudarModo('OFF')">
OFF
</button>

</div>

</div>



<div class="card">

<div class="titulo">
🌡 Temperaturas
</div>


<div class="linha">

<span>🔥 Caldeira:</span>

<span id="tempCaldeira" class="temperatura">
--.- °C
</span>

</div>


<div class="linha">

<span>🚿 AQS:</span>

<span id="tempAQS" class="temperatura">
--.- °C
</span>

</div>


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
// CONTROLO DE RELÉS
// ===============================

bool dentroDoHorario(int inicioH, int inicioM, int fimH, int fimM){

  DateTime agora = rtc.now();
  int agoraMin  = agora.hour() * 60 + agora.minute();
  int inicioMin = inicioH * 60 + inicioM;
  int fimMin    = fimH * 60 + fimM;

  if (inicioMin <= fimMin){
    return (agoraMin >= inicioMin && agoraMin < fimMin);
  } else {
    // horário atravessa a meia-noite (ex: 22:00 às 06:00)
    return (agoraMin >= inicioMin || agoraMin < fimMin);
  }
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

//-----------------------
// MODOS
//-----------------------

server.on("/modo", [](){

if(server.hasArg("valor")){

modoAtual = server.arg("valor");

Serial.print("Modo alterado para: ");
Serial.println(modoAtual);

}

server.send(200,"text/plain",modoAtual);

});


server.on("/horario", [](){

  if (server.hasArg("modo") && server.hasArg("inicioH") && server.hasArg("inicioM")
      && server.hasArg("fimH") && server.hasArg("fimM")){

    String modo = server.arg("modo");
    int iH = server.arg("inicioH").toInt();
    int iM = server.arg("inicioM").toInt();
    int fH = server.arg("fimH").toInt();
    int fM = server.arg("fimM").toInt();

    if (modo == "INVERNO"){
      inicioInvernoH = iH; inicioInvernoM = iM;
      fimInvernoH = fH;   fimInvernoM = fM;
    } else if (modo == "VERAO"){
      inicioVeraoH = iH; inicioVeraoM = iM;
      fimVeraoH = fH;   fimVeraoM = fM;
    }

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Parametros em falta");
  }

});

}   // <-- isto fecha o setup(), não apagues esta chaveta



void controlarRele1(bool ligar){
  if (ligar && !rele1Ligado){
    rele1Ligado = true;
    digitalWrite(RELE1_PIN, LOW);
  }
  if (!ligar && rele1Ligado){
    rele1Ligado = false;
    digitalWrite(RELE1_PIN, HIGH);
  }
}

void controlarRele2(){
  if (!rele2Ligado && tempCaldeira >= TEMP1_LIGA) {
    rele2Ligado = true;
    digitalWrite(RELE2_PIN, LOW);
  }
  if (rele2Ligado && tempCaldeira <= TEMP1_DESLIGA) {
    rele2Ligado = false;
    digitalWrite(RELE2_PIN, HIGH);
  }
}

void controlarRele3(){
  if (!rele3Ligado && tempAQS >= TEMP2_LIGA) {
    rele3Ligado = true;
    digitalWrite(RELE3_PIN, LOW);
  }
  if (rele3Ligado && tempAQS <= TEMP2_DESLIGA) {
    rele3Ligado = false;
    digitalWrite(RELE3_PIN, HIGH);
  }
}

void desligarTudo(){
  controlarRele1(false);
  if (rele2Ligado){ rele2Ligado = false; digitalWrite(RELE2_PIN, HIGH); }
  if (rele3Ligado){ rele3Ligado = false; digitalWrite(RELE3_PIN, HIGH); }
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

  if (modoAtual == "OFF"){
    desligarTudo();
  }
  else if (modoAtual == "ON"){
    controlarRele1(true);
    controlarRele2();
    controlarRele3();
  }
  else if (modoAtual == "INVERNO"){
    controlarRele1(dentroDoHorario(inicioInvernoH, inicioInvernoM, fimInvernoH, fimInvernoM));
    controlarRele2();
    controlarRele3();
  }
  else if (modoAtual == "VERAO"){
    controlarRele1(dentroDoHorario(inicioVeraoH, inicioVeraoM, fimVeraoH, fimVeraoM));
    controlarRele2();
    if (rele3Ligado){ rele3Ligado = false; digitalWrite(RELE3_PIN, HIGH); } // rele 3 sempre off
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

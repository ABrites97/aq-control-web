#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <OneWire.h>
#include <RTClib.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <time.h>

#define ONE_WIRE_BUS 25   // DS18B20 (duas sondas no mesmo fio)
#define RELE1_PIN 17      // Relé 1
#define RELE2_PIN 16      // Relé 2
#define RELE3_PIN 27      // Relé 3

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensores(&oneWire);

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===============================
// WIFI
// ===============================

const char* ssid = "Brites Silva";
const char* password = "SilvaBrites";

unsigned long ultimaVerificacaoWiFi = 0;
bool modoAP = false; 
bool wifiEstavaLigado = true;

// ===============================
// RTC
// ===============================

RTC_DS3231 rtc;
bool rtcOK = true;

// ===============================
// HORA / NTP (Portugal Continental, com hora de verão automática)
// ===============================

// Regra POSIX: WET0WEST,M3.5.0/1,M10.5.0
// - WET (UTC+0) no inverno, WEST (UTC+1) no verão
// - Muda no último domingo de março (01:00 UTC) e último domingo de outubro (01:00 UTC)
const char* TZ_PORTUGAL = "WET0WEST,M3.5.0/1,M10.5.0";

bool horaSincronizadaAlgumaVez = false;   // já conseguimos sincronizar via NTP pelo menos uma vez?
unsigned long ultimaSincronizacaoNTP = 0; // millis() da última sincronização bem sucedida
const unsigned long INTERVALO_RESYNC_NTP = 24UL * 60UL * 60UL * 1000UL; // resincroniza a cada 24h

//-----------------------
// TEMPERATURAS
//-----------------------

float temp1Liga    = 50.0;
float temp1Desliga = 49.0;

float temp2Liga    = 61.0;
float temp2Desliga = 60.0;

float tempCaldeira = 0;
float tempAQS = 0;
bool sonda1OK = true;   
bool sonda2OK = true;  

//-----------------------
// ESTADO RELES
//-----------------------
bool rele1Ligado = false;   // inicia desligado
bool rele2Ligado = false;   
bool rele3Ligado = false;   

//-----------------------
// HORARIOS
//-----------------------
int inicioInvernoH = 17,  inicioInvernoM = 0;
int fimInvernoH    = 23, fimInvernoM    = 0;

int inicioVeraoH = 17,  inicioVeraoM = 0;
int fimVeraoH    = 22, fimVeraoM    = 0;

Preferences prefs; 

//-----------------------
// LCD
//-----------------------
int ecraLCD = 0;
unsigned long ultimaTrocaEcraLCD = 0;
unsigned long ultimaAtualizacaoLCD = 0;
const unsigned long TEMPO_ECRA_LCD = 6000;      // troca de ecrã a cada 5s
const unsigned long INTERVALO_REFRESH_LCD = 500; // refresca valores 2x/seg

String modoAtual = "OFF";

// ===============================
// SERVIDOR
// ===============================

WebServer server(80);

// Variáveis da página

String horaAtual;
String dataAtual;

void registarEvento(String texto);
void lcdLinha(int linha, String texto);

// ===============================
// HTML   (PAGINA WEB)
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

function atualizarStatus(){

  fetch('/status')
  .then(res => res.text())
  .then(dados => {

    let v = dados.split("|");

    document.getElementById("hora").innerHTML = v[0];
    document.getElementById("data").innerHTML = v[1];
    document.getElementById("tempCaldeira").innerHTML = (v[8]=="1" ? v[2] + " °C" : "ERRO");
    document.getElementById("tempAQS").innerHTML = (v[9]=="1" ? v[3] + " °C" : "ERRO");

    estadoRele("rele1", v[4]);   
    estadoRele("rele2", v[5]);   
    estadoRele("rele3", v[6]);   

    mostrarModo(v[7]);

    if (v[11] == "1"){
      document.title = "⚠️ AQ-CONTROL (modo emergência)";
    }

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

function pad(n){
  return n.toString().padStart(2,"0");
}

function atualizarHorarios(){

  fetch('/horarios')
  .then(res => res.text())
  .then(dados => {

    let v = dados.split("|");

    document.getElementById("invInicio").value = pad(v[0]) + ":" + pad(v[1]);
    document.getElementById("invFim").value    = pad(v[2]) + ":" + pad(v[3]);
    document.getElementById("verInicio").value = pad(v[4]) + ":" + pad(v[5]);
    document.getElementById("verFim").value    = pad(v[6]) + ":" + pad(v[7]);

  });

}

function guardarHorario(modo){

  let inicio, fim;

  if(modo=="INVERNO"){
    inicio = document.getElementById("invInicio").value;
    fim    = document.getElementById("invFim").value;
  } else {
    inicio = document.getElementById("verInicio").value;
    fim    = document.getElementById("verFim").value;
  }

  let [iH,iM] = inicio.split(":");
  let [fH,fM] = fim.split(":");

  fetch('/horario?modo=' + modo + '&inicioH=' + iH + '&inicioM=' + iM + '&fimH=' + fH + '&fimM=' + fM)
  .then(res => res.text())
  .then(resposta => {
    alert("Horário " + modo + " guardado!");
  });

}

function atualizarTempConfig(){

  fetch('/temp_config_atual')
  .then(res => res.text())
  .then(dados => {

    let v = dados.split("|");

    document.getElementById("t1liga").value    = v[0];
    document.getElementById("t1desliga").value = v[1];
    document.getElementById("t2liga").value    = v[2];
    document.getElementById("t2desliga").value = v[3];

  });

}

function guardarTempConfig(){

  let t1liga    = document.getElementById("t1liga").value;
  let t1desliga = document.getElementById("t1desliga").value;
  let t2liga    = document.getElementById("t2liga").value;
  let t2desliga = document.getElementById("t2desliga").value;

  fetch('/temp_config?t1liga=' + t1liga + '&t1desliga=' + t1desliga +
        '&t2liga=' + t2liga + '&t2desliga=' + t2desliga)
  .then(res => res.text().then(texto => ({status: res.status, texto: texto})))
  .then(r => {
    if (r.status == 200){
      alert("Temperaturas guardadas!");
    } else {
      alert(r.texto);   // mostra o erro de validação
    }
  });

}

function atualizarLog(){

  fetch('/log')
  .then(res => res.text())
  .then(dados => {
    document.getElementById("logTexto").innerText = dados;
  });

}

function limparLog(){

  if (!confirm("Apagar todo o histórico de eventos?")) return;

  fetch('/log_limpar')
  .then(() => atualizarLog());

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


atualizarStatus();
atualizarHorarios();
atualizarTempConfig();
atualizarLog(); 

setInterval(atualizarStatus,1000);
setInterval(atualizarLog, 10000);

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

input[type="time"]{
  font-size:18px;
  padding:6px 10px;
  border-radius:8px;
  border:0;
  background:white;
  color:#111827;
}

.campo-temp{
  position:relative;
  display:inline-block;
}

.input-temp{
  font-size:18px;
  padding:6px 34px 6px 10px;  
  border-radius:8px;
  border:0;
  background:white;
  color:#111827;
  width:90px;
  text-align:right;
}

.campo-temp .unidade{
  position:absolute;
  right:10px;
  top:50%;
  transform:translateY(-50%);
  font-size:16px;
  color:#334155;
  pointer-events:none; 
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

<span>Ordem Caldeira:</span>

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

<span>Bomba Aquecimento:</span>

<span id="rele3" class="desligado">
🔴 OFF
</span>

</div>


</div>

<div class="card">

<div class="titulo">
🕒 Horários
</div>

<div class="linha">
<span>Hora Início Inverno:</span>
<input type="time" id="invInicio">
</div>

<div class="linha">
<span>Hora Fim Inverno:</span>
<input type="time" id="invFim">
</div>

<button onclick="guardarHorario('INVERNO')">Guardar Inverno</button>

<div class="linha">
<span>Hora Início Verão:</span>
<input type="time" id="verInicio">
</div>

<div class="linha">
<span>Hora Fim Verão:</span>
<input type="time" id="verFim">
</div>

<button onclick="guardarHorario('VERAO')">Guardar Verão</button>

</div>


<div class="card">

<div class="titulo">
🌡️ Limites de Temperatura
</div>

<div class="linha">
<span>Bomba Caldeira ON:</span>
<div class="campo-temp">
<input type="number" step="0.1" id="t1liga" class="input-temp">
<span class="unidade">°C</span>
</div>
</div>

<div class="linha">
<span>Bomba Caldeira OFF:</span>
<div class="campo-temp">
<input type="number" step="0.1" id="t1desliga" class="input-temp">
<span class="unidade">°C</span>
</div>
</div>

<div class="linha">
<span>Bomba Radiadores ON:</span>
<div class="campo-temp">
<input type="number" step="0.1" id="t2liga" class="input-temp">
<span class="unidade">°C</span>
</div>
</div>

<div class="linha">
<span>Bomba Radiadores OFF:</span>
<div class="campo-temp">
<input type="number" step="0.1" id="t2desliga" class="input-temp">
<span class="unidade">°C</span>
</div>
</div>

<button onclick="guardarTempConfig()">Guardar Temperaturas</button>

</div>

<div class="card">

<div class="titulo">
📋 Histórico de Eventos
</div>

<div id="logTexto" style="text-align:left; font-size:14px; white-space:pre-wrap; background:#0f172a; padding:12px; border-radius:10px; max-height:300px; overflow-y:auto;">
A carregar...
</div>

<button onclick="limparLog()" style="margin-top:12px;">Limpar Histórico</button>

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

  if (!rtcOK){
    horaAtual = "--:--";
    dataAtual = "ERRO";
    return;
  }

  DateTime agora = rtc.now();

  char hora[10];
  sprintf(hora,"%02d:%02d", agora.hour(), agora.minute());
  horaAtual = String(hora);

  char data[12];
  sprintf(data,"%02d/%02d/%04d", agora.day(), agora.month(), agora.year());
  dataAtual = String(data);

}

// ===============================
// SINCRONIZAÇÃO NTP (fuso de Portugal Continental, com DST automático)
// ===============================

void configurarHoraNTP(){
  // configTzTime aplica o fuso horário do sistema (com regras de DST) e
  // inicia a sincronização por NTP em segundo plano.
  configTzTime(TZ_PORTUGAL, "pt.pool.ntp.org", "pool.ntp.org", "time.google.com");
}

// Tenta obter a hora por NTP e, se conseguir, acerta o RTC.
// Devolve true se a sincronização foi bem sucedida.
bool sincronizarRTCcomNTP(){

  if (!rtcOK){
    // Sem RTC físico não há onde gravar a hora sincronizada
    return false;
  }

  struct tm timeinfo;

  // getLocalTime tenta durante o tempo indicado (ms) até a hora estar disponível
  if (!getLocalTime(&timeinfo, 5000)){
    registarEvento("Falha ao sincronizar hora via NTP");
    return false;
  }

  rtc.adjust(DateTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  ));

  horaSincronizadaAlgumaVez = true;
  ultimaSincronizacaoNTP = millis();

  registarEvento("RTC sincronizado via NTP");
  return true;
}

// ===============================
// CONTROLO DE RELÉS
// ===============================

bool dentroDoHorario(int inicioH, int inicioM, int fimH, int fimM){

  if (!rtcOK) return false;   // <<< sem hora fiável, relé 1 fica desligado por segurança

  DateTime agora = rtc.now();
  int agoraMin  = agora.hour() * 60 + agora.minute();
  int inicioMin = inicioH * 60 + inicioM;
  int fimMin    = fimH * 60 + fimM;

  if (inicioMin <= fimMin){
    return (agoraMin >= inicioMin && agoraMin < fimMin);
  } else {
    return (agoraMin >= inicioMin || agoraMin < fimMin);
  }
}

// ===============================
// SETUP
// ===============================

void setup(){

server.on("/status", [](){

  String resposta = 
  horaAtual + "|" + dataAtual + "|" +
  String(tempCaldeira,2) + "|" + String(tempAQS,2) + "|" +
  String(rele1Ligado) + "|" + String(rele2Ligado) + "|" + String(rele3Ligado) + "|" +
  modoAtual + "|" +
  String(sonda1OK) + "|" + String(sonda2OK) + "|" + String(rtcOK)  + "|" + String(modoAP) + "|" +
  String(horaSincronizadaAlgumaVez);

  server.send(200,"text/plain",resposta);

});

Serial.begin(115200);

if (!LittleFS.begin(true)){   // true = formata automaticamente se necessário
  Serial.println("Erro ao iniciar LittleFS");
}

Wire.begin();

lcd.init();
lcd.backlight();
lcdLinha(0, "AQ-CONTROL");
lcdLinha(1, "A iniciar...");

if(!rtc.begin()){
  Serial.println("RTC não encontrado! A continuar em modo degradado.");
  rtcOK = false;
  registarEvento("RTC nao encontrado - modo degradado");
} else if (rtc.lostPower()){

  Serial.println("RTC sem hora guardada - a aguardar sincronizacao NTP");
  registarEvento("RTC sem hora guardada - a aguardar NTP");

}

prefs.begin("aqcontrol", false);

inicioInvernoH = prefs.getInt("invIH", inicioInvernoH);
inicioInvernoM = prefs.getInt("invIM", inicioInvernoM);
fimInvernoH    = prefs.getInt("invFH", fimInvernoH);
fimInvernoM    = prefs.getInt("invFM", fimInvernoM);

inicioVeraoH = prefs.getInt("verIH", inicioVeraoH);
inicioVeraoM = prefs.getInt("verIM", inicioVeraoM);
fimVeraoH    = prefs.getInt("verFH", fimVeraoH);
fimVeraoM    = prefs.getInt("verFM", fimVeraoM);

temp1Liga    = prefs.getFloat("t1Liga", temp1Liga);
temp1Desliga = prefs.getFloat("t1Desl", temp1Desliga);
temp2Liga    = prefs.getFloat("t2Liga", temp2Liga);
temp2Desliga = prefs.getFloat("t2Desl", temp2Desliga);

modoAtual = prefs.getString("modo", modoAtual);

sensores.begin();
sensores.setWaitForConversion(false);
Serial.println("DS18B20 iniciado");

// ===============================
// WIFI COM IP FIXO
// ===============================

IPAddress local_IP(192,168,68,200);
IPAddress gateway(192,168,68,1);
IPAddress subnet(255,255,252,0);


Serial.println("A configurar IP fixo...");

WiFi.mode(WIFI_STA);

if (!WiFi.config(local_IP, gateway, subnet)) {
  Serial.println("Erro a configurar IP fixo");
}

WiFi.begin(ssid, password);

Serial.print("WiFi");

int tentativas = 0;
while (WiFi.status() != WL_CONNECTED && tentativas < 30) {   // 30 x 500ms = 15s
  delay(500);
  Serial.print(".");
  tentativas++;
}

Serial.println();

if (WiFi.status() == WL_CONNECTED){

  Serial.println("WiFi ligado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("aqcontrol")){
    Serial.println("mDNS ativo: http://aqcontrol.local");
  }

  // Só faz sentido pedir hora por NTP quando há ligação à internet real
  configurarHoraNTP();
  sincronizarRTCcomNTP();

} else {

  // Rede de casa indisponível — cria rede de emergência
  Serial.println("WiFi de casa indisponivel - a criar rede de emergencia");

  modoAP = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("AQ-CONTROL", "1234");   // muda a password se quiseres

  Serial.print("Rede de emergencia criada. IP: ");
  Serial.println(WiFi.softAPIP());

  registarEvento("WiFi de casa falhou - modo de emergencia ativado");
  // Em modo AP não há internet, por isso não há NTP disponível.
  // O RTC (se existir e já tiver hora válida de uma sincronização anterior)
  // continua a ser usado normalmente.
}

server.on("/", [](){

//lerRTC();

server.send(200,"text/html; charset=utf-8",pagina());

});

server.on("/temperaturas", [](){

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

String modoAnterior = modoAtual;
modoAtual = server.arg("valor");

prefs.putString("modo", modoAtual);

registarEvento("Modo alterado: " + modoAnterior + " -> " + modoAtual);

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

  prefs.putInt("invIH", iH);
  prefs.putInt("invIM", iM);
  prefs.putInt("invFH", fH);
  prefs.putInt("invFM", fM);

} else if (modo == "VERAO"){
  inicioVeraoH = iH; inicioVeraoM = iM;
  fimVeraoH = fH;   fimVeraoM = fM;

  prefs.putInt("verIH", iH);
  prefs.putInt("verIM", iM);
  prefs.putInt("verFH", fH);
  prefs.putInt("verFM", fM);
}

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Parametros em falta");
  }

});

server.on("/horarios", [](){

  String resposta =
  String(inicioInvernoH) + "|" + String(inicioInvernoM) + "|" +
  String(fimInvernoH)    + "|" + String(fimInvernoM)    + "|" +
  String(inicioVeraoH)   + "|" + String(inicioVeraoM)   + "|" +
  String(fimVeraoH)      + "|" + String(fimVeraoM);

  server.send(200,"text/plain",resposta);

});

server.on("/temp_config", [](){

  if (server.hasArg("t1liga") && server.hasArg("t1desliga")
      && server.hasArg("t2liga") && server.hasArg("t2desliga")){

    float novoT1Liga    = server.arg("t1liga").toFloat();
    float novoT1Desliga = server.arg("t1desliga").toFloat();
    float novoT2Liga    = server.arg("t2liga").toFloat();
    float novoT2Desliga = server.arg("t2desliga").toFloat();

    // >>> VALIDAÇÃO — LIGA tem de ser maior que DESLIGA
    if (novoT1Liga <= novoT1Desliga || novoT2Liga <= novoT2Desliga){
      server.send(400, "text/plain", "Erro: o valor ON tem de ser maior que o OFF");
      return;
    }

    temp1Liga    = novoT1Liga;
    temp1Desliga = novoT1Desliga;
    temp2Liga    = novoT2Liga;
    temp2Desliga = novoT2Desliga;

    prefs.putFloat("t1Liga", temp1Liga);
    prefs.putFloat("t1Desl", temp1Desliga);
    prefs.putFloat("t2Liga", temp2Liga);
    prefs.putFloat("t2Desl", temp2Desliga);

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Parametros em falta");
  }

});

server.on("/temp_config_atual", [](){

  String resposta =
  String(temp1Liga,1) + "|" + String(temp1Desliga,1) + "|" +
  String(temp2Liga,1) + "|" + String(temp2Desliga,1);

  server.send(200,"text/plain",resposta);

});

server.on("/log", [](){

  File f = LittleFS.open("/log.txt", "r");

  if (!f){
    server.send(200, "text/plain", "(sem eventos registados)");
    return;
  }

  String conteudo = f.readString();
  f.close();

  if (conteudo.length() == 0) conteudo = "(sem eventos registados)";

  server.send(200, "text/plain", conteudo);

});

server.on("/log_limpar", [](){

  LittleFS.remove("/log.txt");
  server.send(200, "text/plain", "OK");

});

}   // <fecha o setup()


void controlarRele1(bool ligar){
  if (ligar && !rele1Ligado){
    rele1Ligado = true;
    digitalWrite(RELE1_PIN, LOW);
    registarEvento("Ordem Caldeira: ON");
  }
  if (!ligar && rele1Ligado){
    rele1Ligado = false;
    digitalWrite(RELE1_PIN, HIGH);
    registarEvento("Ordem Caldeira: OFF");
  }
}

void controlarRele2(){

  if (!sonda1OK){
    if (rele2Ligado){ rele2Ligado = false;
    digitalWrite(RELE2_PIN, HIGH); 
    registarEvento("Bomba Caldeira: OFF (sonda em erro)");
    }
    return;
  }

  if (!rele2Ligado && tempCaldeira >= temp1Liga) {
    rele2Ligado = true;
    digitalWrite(RELE2_PIN, LOW);
    registarEvento("Bomba Caldeira: ON (" + String(tempCaldeira,1) + "C)");

  }
  if (rele2Ligado && tempCaldeira <= temp1Desliga) {
    rele2Ligado = false;
    digitalWrite(RELE2_PIN, HIGH);
    registarEvento("Bomba Caldeira: OFF (" + String(tempCaldeira,1) + "C)");
  }
}

void controlarRele3(){

  if (!sonda2OK){
    if (rele3Ligado){ rele3Ligado = false; 
    digitalWrite(RELE3_PIN, HIGH); 
    registarEvento("Bomba Aquecimento: OFF (sonda em erro)");
    }
    return;
  }

  if (!rele3Ligado && tempAQS >= temp2Liga) {
    rele3Ligado = true;
    digitalWrite(RELE3_PIN, LOW);
    registarEvento("Bomba Aquecimento: ON (" + String(tempAQS,1) + "C)");
  }
  if (rele3Ligado && tempAQS <= temp2Desliga) {
    rele3Ligado = false;
    digitalWrite(RELE3_PIN, HIGH);
    registarEvento("Bomba Aquecimento: OFF (" + String(tempAQS,1) + "C)");
  }
}

void desligarTudo(){
  controlarRele1(false);
  if (rele2Ligado){ rele2Ligado = false; digitalWrite(RELE2_PIN, HIGH); }
  if (rele3Ligado){ rele3Ligado = false; digitalWrite(RELE3_PIN, HIGH); }
}

// ===============================
// HISTÓRICO DE EVENTOS
// ===============================

const size_t TAMANHO_MAX_LOG = 8192;   // 8KB — chega para centenas de linhas

void registarEvento(String texto){

  String carimbo = rtcOK ? (dataAtual + " " + horaAtual) : ("SEM RTC (" + String(millis()/1000) + "s)");

  String linha = "[" + carimbo + "] " + texto + "\n";

  Serial.print("EVENTO: ");
  Serial.print(linha);

  File f = LittleFS.open("/log.txt", "a");
  if (f){
    f.print(linha);
    f.close();
  }

  // se o ficheiro crescer demasiado, corta o início (mantém só o mais recente)
  f = LittleFS.open("/log.txt", "r");
  if (f && f.size() > TAMANHO_MAX_LOG){

    String conteudo = f.readString();
    f.close();

    conteudo = conteudo.substring(conteudo.length() - TAMANHO_MAX_LOG/2);

    // corta até à próxima linha completa, para não ficar uma linha partida
    int primeiraQuebra = conteudo.indexOf('\n');
    if (primeiraQuebra >= 0) conteudo = conteudo.substring(primeiraQuebra + 1);

    File f2 = LittleFS.open("/log.txt", "w");
    if (f2){
      f2.print(conteudo);
      f2.close();
    }
  } else if (f) {
    f.close();
  }
}


// ===============================
// LOOP
// ===============================

void loop(){

  server.handleClient();

    // verifica o WiFi a cada 30 segundos
if (millis() - ultimaVerificacaoWiFi >= 30000){
  ultimaVerificacaoWiFi = millis();

  bool ligadoAgora = (WiFi.status() == WL_CONNECTED);

  if (!ligadoAgora && wifiEstavaLigado){
    registarEvento("WiFi caiu");           // transição: estava ligado, agora não
  }
  if (ligadoAgora && !wifiEstavaLigado){
    registarEvento("WiFi reconectado");    // transição: estava caído, agora voltou
  }
  wifiEstavaLigado = ligadoAgora;

  if (!modoAP && !ligadoAgora){
    Serial.println("WiFi caiu - a tentar reconectar...");
    WiFi.reconnect();
  }

    // Resincronização periódica em segundo plano, só quando há rede de casa real
  if (!modoAP && ligadoAgora && (millis() - ultimaSincronizacaoNTP >= INTERVALO_RESYNC_NTP)){
    sincronizarRTCcomNTP();
  }    
}

  // se estiver em modo de emergência, tenta voltar à rede de casa a cada 5 min
static unsigned long ultimaTentativaVoltarACasa = 0;

if (modoAP && (millis() - ultimaTentativaVoltarACasa >= 5UL*60UL*1000UL)){  // a cada 5 min
  ultimaTentativaVoltarACasa = millis();
  Serial.println("Em modo emergencia - a tentar voltar a rede de casa...");
  registarEvento("A tentar voltar a rede de casa (reinicio)");
  delay(500);
  ESP.restart();
}

  static bool reiniciadoHoje = false;

if (rtcOK){
  DateTime agora = rtc.now();

  if (agora.hour() == 4 && !reiniciadoHoje){
    reiniciadoHoje = true;
    Serial.println("Reinício diário preventivo...");
    registarEvento("Reinicio diario preventivo");
    delay(1000);
    ESP.restart();
  }

  if (agora.hour() != 4){
    reiniciadoHoje = false;  // reset da flag fora da janela das 4h
  }
}

  static unsigned long ultimo = 0;
  if (millis() - ultimo >= 1000){
    ultimo = millis();
      lerRTC();
    iniciarLeituraTemperaturas();   // pede nova conversão (não bloqueia)
  }

  verificarLeituraTemperaturas();   // verifica se já está pronta (não bloqueia)

  atualizarLCD(); 

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
    if (rele3Ligado){ rele3Ligado = false; 
    digitalWrite(RELE3_PIN, HIGH);  // rele 3 sempre off no verão
    registarEvento("Bomba Aquecimento: OFF (modo Verao)");
    }
  }
}


bool conversaoEmCurso = false;
unsigned long tempoInicioConversao = 0;
const unsigned long TEMPO_CONVERSAO = 750; // ms (12-bit)

void iniciarLeituraTemperaturas(){
  if (!conversaoEmCurso){
    sensores.requestTemperatures();
    tempoInicioConversao = millis();
    conversaoEmCurso = true;
  }
}

void verificarLeituraTemperaturas(){
  if (conversaoEmCurso && (millis() - tempoInicioConversao >= TEMPO_CONVERSAO)){

    float t1 = sensores.getTempCByIndex(0);
    float t2 = sensores.getTempCByIndex(1);

    bool novaSonda1OK = (t1 != DEVICE_DISCONNECTED_C);
    bool novaSonda2OK = (t2 != DEVICE_DISCONNECTED_C);

    if (!novaSonda1OK && sonda1OK) registarEvento("Sonda Caldeira desligada");
    if (novaSonda1OK && !sonda1OK) registarEvento("Sonda Caldeira reconectada");
    if (!novaSonda2OK && sonda2OK) registarEvento("Sonda AQS desligada");
    if (novaSonda2OK && !sonda2OK) registarEvento("Sonda AQS reconectada");

    sonda1OK = (t1 != DEVICE_DISCONNECTED_C);
    sonda2OK = (t2 != DEVICE_DISCONNECTED_C);

    if (sonda1OK) tempCaldeira = t1;
    if (sonda2OK) tempAQS = t2;

    conversaoEmCurso = false;
  }
}

// ===============================
// LCD
// ===============================

void lcdLinha(int linha, String texto){

  while (texto.length() < 16) texto += " ";
  if (texto.length() > 16) texto = texto.substring(0,16);

  lcd.setCursor(0, linha);
  lcd.print(texto);
}

void desenharEcraLCD(){

if (ecraLCD == 0){
  if (modoAP){
    lcdLinha(0, "REDE EMERGENCIA");
    lcdLinha(1, "IP:192.168.4.1");
  } else {
    String marcador = horaSincronizadaAlgumaVez ? "" : "*";
    lcdLinha(0, " Hora:  " + horaAtual + marcador);
    lcdLinha(1, " Modo: " + modoAtual);
  }
}
  else if (ecraLCD == 1){
  lcdLinha(0, sonda1OK ? ("Temp.Cal: " + String(tempCaldeira,1) + (char)223 + "C") : "Temp.Cal: ERRO");
  lcdLinha(1, sonda2OK ? ("Temp.AQS: " + String(tempAQS,1) + (char)223 + "C") : "Temp.AQS: ERRO");
}
  else if (ecraLCD == 2){
    lcdLinha(0, String("  Caldeira:") + (rele1Ligado ? "ON" : "OFF"));
    lcdLinha(1, String(" BC:") + (rele2Ligado ? "ON" : "OFF") + "  BA:" + (rele3Ligado ? "ON" : "OFF"));
  }
}

void atualizarLCD(){

  unsigned long agora = millis();

  // troca de ecrã a cada 5 segundos
  if (agora - ultimaTrocaEcraLCD >= TEMPO_ECRA_LCD){
    ultimaTrocaEcraLCD = agora;
    ecraLCD = (ecraLCD + 1) % 3;
  }

  // refresca os valores no ecrã atual (sem piscar, sem clear())
  if (agora - ultimaAtualizacaoLCD >= INTERVALO_REFRESH_LCD){
    ultimaAtualizacaoLCD = agora;
    desenharEcraLCD();
  }
}



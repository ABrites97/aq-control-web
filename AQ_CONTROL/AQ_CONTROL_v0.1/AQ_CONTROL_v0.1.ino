#include <WiFi.h>
#include <WebServer.h>


// ===============================
// CONFIGURAÇÃO WIFI
// ===============================

const char* ssid = "Brites Silva";
const char* password = "SilvaBrites";


// ===============================
// SERVIDOR WEB
// ===============================

WebServer server(80);


// ===============================
// PÁGINA AQ-CONTROL
// ===============================

const char pagina[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

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


h1{
color:#ff9800;
}


.valor{

font-size:35px;
color:#00e676;

}


button{

padding:15px;
margin:5px;
border-radius:10px;
font-size:16px;

}


</style>

</head>


<body>


<h1> 🚿 AQ-CONTROL 🔥 </h1>


<div class="card">

<h2>Sistema</h2>

<p>Hora RTC</p>

<div class="valor">
--:--:--
</div>


<p>Modo</p>

<h2>INVERNO</h2>


</div>



<div class="card">

<h2>Temperaturas</h2>

<p>🔥 Caldeira</p>

<div class="valor">
-- °C
</div>


<p>🚿 AQS</p>

<div class="valor">
-- °C
</div>


</div>




<div class="card">

<h2>Saídas</h2>

<p>Contacto Caldeira 🟢</p>

<p>Bomba Caldeira 🔴</p>

<p>Bomba Casa 🟢</p>


</div>



<button>ON</button>

<button>INVERNO</button>

<button>VERÃO</button>

<button>OFF</button>



</body>

</html>


)rawliteral";




// ===============================
// SETUP
// ===============================

void setup() {


Serial.begin(115200);


WiFi.begin(ssid,password);


Serial.print("A ligar WiFi");


while(WiFi.status()!=WL_CONNECTED){

delay(500);

Serial.print(".");

}


Serial.println();

Serial.println("WiFi ligado");


Serial.print("IP: ");

Serial.println(WiFi.localIP());



server.on("/", [](){

server.send(200,"text/html; charset=utf-8",pagina);

});


server.begin();


Serial.println("Servidor iniciado");


}



// ===============================
// LOOP
// ===============================

void loop() {

server.handleClient();

}
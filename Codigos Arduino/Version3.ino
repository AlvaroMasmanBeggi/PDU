#include <SPI.h>
#include <Ethernet.h>
// Configuración de red // Dirección MAC e IP para el Arduino
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 0, 0, 178);
EthernetServer server(80);  // Inicializa el servidor web en el puerto 80
struct Rele {
  int pin;
  float medicion = 0;
  bool On = true;
  String corriente, Potencia;
};
String Cadena = "";
Rele rele[4];
float voltage, ki = 0.1470588 /*5/34*/, kv = 68.86270491 /* 55 * 611 / (8 * 51)*/;
int i, numero_muestrasAC = 6000, Equipos = 4, contador;
/*Sincronismo mediante la variable Equipos, está me define que dato envio a labview:
0 I1,
1 I2
2 I3
3 I4
4 Tensión
*/

char comentario[21] = "";          // Para guardar el último comentario enviado (hasta 20 caracteres)
void Ok(EthernetClient &client) {  // Envía la cabecera HTTP para respuestas "200 OK"
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/plain"));
  client.println(F("Connection: close"));
  client.println();
}
void setup() {
  contador = Equipos;
  Serial.begin(115200);
  //	pinMode(A0, INPUT);  //Mido corrientes
  //	pinMode(A1, INPUT);  //Mido tensiones
  for (int i = 0; i < Equipos; i++) {  // Inicializa los pines de los LEDs como salida y apaga todos
    rele[i].pin = (Equipos + 1) - i;   //No usar 0 1, UART; 10 11 12 13 Ethernet;
    pinMode(rele[i].pin, OUTPUT);
    digitalWrite(rele[i].pin, LOW);
  }
  Ethernet.begin(mac, ip);  // Inicia la red
  server.begin();           //Inicia el servidor
  Serial.print("Servidor iniciado en: ");
  Serial.println(Ethernet.localIP());
}
void loop() {
  lectura_tensionAC();          //Habla
  while (Serial.available()) {  //Escucha
    char c = Serial.read();
    if (c == '\n') {
      procesarCadena(Cadena);
      Cadena = "";
    } else {
      Cadena += c;
    }
  }
  Web();  // Atiende peticiones web
  for (i = 0; i < Equipos; i++) {
    digitalWrite(rele[i].pin, rele[i].On);
  }
}
//Funciones utilizadas//
/*------------------------------------------
Habla
------------------------------------------*/
void lectura_tensionAC() {
  String Enviar = "";  // Inicializo para enviar al Lab View
  int lectura = 0;         // Destinado a la obtención de los ADC
  float valor_inst = 0;      // Valor requerido
  float value = 0;
  for (i = 0; i < numero_muestrasAC; i++) {
    if (contador < Equipos) {
      lectura = analogRead(A0);
    } else {
      lectura = analogRead(A1);
    }
    valor_inst = lectura * lectura + valor_inst;
    delayMicroseconds(17);  //Periodo de muestreo optimo, para despreciar el tiempo de muestreo necesario
  }
  valor_inst /= numero_muestrasAC;
  value = sqrt(valor_inst);  //Variable global con la información necesaria
  if (contador < Equipos) {  //Sincronismo mediante la variable Equipos, está me define que dato envio a labview.
    value = value * ki;
    rele[contador].corriente = String(value * 5 / 1023, 3);  //Solo si lo solicitan, lo presentamos, si no solo la potencia
    rele[contador].Potencia = String(value * 5 * voltage / 1023, 3);
    rele[contador].medicion = value * 5 * voltage / 1023;
    Serial.println(rele[contador].Potencia);
  } else {
    value = value * kv;
    voltage = value * 5 / 1023;
  }
  contador = (contador + 1) % (Equipos + 1);
  // Para el Multiplexor
  digitalWrite(7, contador % 2 > 0);
  digitalWrite(6, contador % 4 > 1);
  //digitalWrite(8, contador%8>3); Lo use pa ve si estaban bien los estados
}
/*------------------------------------------
Escucha
------------------------------------------*/
void procesarCadena(String texto) {
  int inicio = 0;
  int coma = texto.indexOf(',');
  while (coma != -1) {
    String comando = texto.substring(inicio, coma);
    ejecutar(comando);
    inicio = coma + 1;
    coma = texto.indexOf(',', inicio);
  }
  ejecutar(texto.substring(inicio));  // Último comando si no termina en coma
}
void ejecutar(String comando) {  //Uso Los RE para comandar los cambios, los RA me los deja de forma constante
  if (comando == "RE1") {
    rele[0].On = !rele[0].On;
  }
  if (comando == "RE2") {
    rele[1].On = !rele[1].On;
  }
  if (comando == "RE3") {
    rele[2].On = !rele[2].On;
  }
  if (comando == "RE4") {
    rele[3].On = !rele[3].On;
  }
}
/*------------------------------------------
Web
------------------------------------------*/
void Web() {  // Atiende las peticiones HTTP
  EthernetClient client = server.available();
  if (!client) return;                // Si no hay cliente, salir
  String req = leerPeticion(client);  // lee la petición
  if (req.indexOf("GET / ") >= 0) {   // según la URL solicitada, responde
    enviarPagina(client);             // página completa
  } else if (req.indexOf("/toggle") >= 0) {
    manejarToggle(client, req);  // cambiar estado LED
  } else if (req.indexOf("/estado") >= 0) {
    enviarEstado(client);  // enviar estado de LEDs
  } else if (req.indexOf("/comentario") >= 0) {
    manejarComentario(client, req);  // recibir comentario
  }

  delay(1);       // pequeña pausa
  client.stop();  // cierra conexión
}
String leerPeticion(EthernetClient &client) {  // Lee la petición HTTP completa hasta encontrar el fin de cabecera
  String req = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      req += c;
      if (req.endsWith("\r\n\r\n")) break;  // fin de cabecera HTTP
    }
  }
  return req;
}
void enviarPagina(EthernetClient &client) {  // Genera y envía toda la página web al cliente
  client.println(F("HTTP/1.1 200 OK"));      // Cabecera HTTP para HTML
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println();
  // HTML + CSS + JS embebido
  client.println(F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"));
  client.println(F("<title>Panel 4 LEDs</title>"));
  client.println(F("<style>body{font-family:Arial;text-align:center;background:#eee;}"));
  client.println(F(".panel{display:inline-block;margin:10px;padding:10px;width:220px;border-radius:8px;color:white;}"));
  client.println(F(".button{margin:5px;padding:10px 20px;font-size:16px;color:white;border:none;cursor:pointer;border-radius:5px;transition:0.3s;}"));
  client.println(F(".on{background:green;}.on:hover{background:lime;}.off{background:red;}.off:hover{background:tomato;}"));
  client.println(F(".med{margin-top:10px;padding:5px;background:#000;border-radius:4px;}</style></head><body>"));
  client.println(F("<h1>Panel de Control</h1>"));
  // Paneles de cada LED
  const char *colores[] = { "#e53935", "#1e88e5", "#43a047", "#fb8c00" };  //Hexadecimal 2 cifras paa RGB
  for (int i = 0; i < Equipos; i++) {
    client.print(F("<div class='panel' style='background:"));
    client.print(colores[i]);
    client.println(F(";'>"));
    client.print(F("<h2 id='estado"));
    client.print(i);
    client.print(F("'>LED "));
    client.print(i + 1);
    client.println(F(": -</h2>"));
    client.print(F("<button id='boton"));
    client.print(i);
    client.print(F("' class='button' onclick='toggle("));
    client.print(i);
    client.print(F(")'>-</button>"));
    client.print(F("<div class='med' id='medicion"));
    client.print(i);
    client.println(F("'>Medición: - V</div></div>"));
  }
  // Caja para enviar comentarios
  client.println(F("<h2>Enviar comentario</h2>"));
  client.println(F("<input type='text' id='comentario' maxlength='20'>"));
  client.println(F("<button onclick='enviarComentario()'>Enviar</button>"));
  client.println(F("<p id='resp'></p>"));
  // Script Javascript para AJAX
  client.println(F("<script>"));
  client.println(F("function toggle(i){var x=new XMLHttpRequest();x.open('GET','/toggle'+i,true);x.send();}"));
  client.println(F("function actualizar(){var x=new XMLHttpRequest();x.open('GET','/estado',true);x.onload=function(){if(x.status==200){var d=x.responseText.trim().split(';');for(let i=0;i<d.length;i++){var p=d[i].split(',');document.getElementById('estado'+i).textContent='LED '+(i+1)+': '+(p[0]=='1'?'ENCENDIDO':'APAGADO');document.getElementById('medicion'+i).textContent='Medición: '+p[1]+' V';var b=document.getElementById('boton'+i);if(p[0]=='1'){b.textContent='Apagar';b.className='button off';}else{b.textContent='Encender';b.className='button on';}}}};x.send();}"));
  client.println(F("function enviarComentario(){var c=document.getElementById('comentario').value;var x=new XMLHttpRequest();x.open('GET','/comentario?c='+encodeURIComponent(c),true);x.onload=function(){document.getElementById('resp').textContent='Comentario enviado';};x.send();}"));
  client.println(F("setInterval(actualizar,100);</script></body></html>"));
}
void manejarToggle(EthernetClient &client, String req) {  // Cambia el estado de un LED
  int led = req.charAt(req.indexOf("/toggle") + 7) - '0';
  rele[led].On = !rele[led].On;
  Ok(client);  // responde con cabecera
  client.println(F("OK"));
}
void enviarEstado(EthernetClient &client) {  // Envía el estado actual y medición de todos los LEDs
  Ok(client);
  for (int i = 0; i < Equipos; i++) {
    rele[i].medicion = analogRead(A0 + i) * (5.0 / 1023.0);  // simulación de voltaje
    client.print(rele[i].On ? "1" : "0");
    client.print(",");
    client.print(rele[i].medicion, 2);
    if (i < Equipos - 1) client.print(";");
  }
  client.println();
}
void manejarComentario(EthernetClient &client, String req) {
	int idx = req.indexOf("?c=");
	if (idx >= 0) {		// extrae la subcadena después de ?c=
		String com = req.substring(idx + 3, req.indexOf(" ", idx));
		com.replace("+", " ");  // opcional, si usás espacios
		com.toCharArray(comentario, 21);  // opcional, si querés guardarlo igual
		Serial.print("Comentario recibido: ");
		Serial.println(com);		// Llama directamente a procesarCadena para analizar y ejecutar
		procesarCadena(com);
	}

	Ok(client);  // responde al navegador
	client.println(F("OK"));
}

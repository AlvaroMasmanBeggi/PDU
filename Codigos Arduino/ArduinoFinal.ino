#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>


// Configuración de red // Dirección MAC e IP para el Arduino
//	pinMode(A0, INPUT);  //Mido corrientes
//	pinMode(A1, INPUT);  //Mido tensiones
//  Los reles estan desde el pin 5 al 2
//  Los controles del multiplexor en 6 y 7

// Dirección del módulo I2C (común: 0x27 o 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 23, 53);
EthernetServer server(80);  // Inicializa el servidor web en el puerto 80
struct Rele {
  int pin;
  float corriente, medicion = 0;
  bool On = false;
  String Potencia;
};
String Cadena = "";
Rele rele[4];
float voltage, Imax = 0.5;  // Corriente máxima;
int i, Equipos = 4, contador, T = 0, S, A = 0, to = 0, t;
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
  lcd.init();
  lcd.backlight();
  S = contador = Equipos;
  Serial.begin(115200);
  for (int i = 0; i < Equipos; i++) {  // Inicializa los pines de los LEDs como salida y apaga todos
    rele[i].pin = (Equipos + 1) - i;   //No usar 0 1, UART; 10 11 12 13 Ethernet;
    pinMode(rele[i].pin, OUTPUT);
    digitalWrite(rele[i].pin, rele[i].On);
  }
  Ethernet.begin(mac, ip);  // Inicia la red
  server.begin();           //Inicia el servidor
}
void loop() {
  lectura_tensionAC();  //Habla
  Escuchar();
  Web();         // Atiende peticiones web
  if (S != A) {  //Solo lo escribo cuando A cambia
    if (S < Equipos) {
      mostrarCanal(S);
    } else if (S == Equipos) {
      mostrarTension();
    }
    A = S;
  }
  T = (T + 1) % 6;
  if (T == 1) {
    S = (S + 1) % (Equipos + 1);
  }
  for (i = 0; i < Equipos; i++) {
    digitalWrite(rele[i].pin, rele[i].On);
  }
}
//Funciones utilizadas//
/*------------------------------------------
Habla
------------------------------------------*/
void lectura_tensionAC() {
  int numero_muestrasAC = 6000;  // Destinado a la obtención de los ADC
  float lectura = 0, valor_inst = 0, value, ki = 0.567*0.32  /*5/34*/, kv = 145.1 /* 55 * 611 / (8 * 51)*/;
  String Enviar = "";  // Inicializo para enviar al Lab View
  delay(100);
  if (rele[contador].On == true || contador == Equipos) {
    for (i = 0; i < numero_muestrasAC; i++) {
      lectura = analogRead(A0 + contador / Equipos);
      valor_inst += lectura * lectura;
      delayMicroseconds(17);                         //Periodo de muestreo optimo, para despreciar el tiempo de muestreo necesario
      value = sqrt(valor_inst / numero_muestrasAC);  //Valor digital RMS
    }
    if (contador < Equipos) {  //Sincronismo mediante la variable Equipos, está me define que dato envio a labview.
      rele[contador].medicion = value * 5 * voltage * ki / 1023;
      rele[contador].Potencia = String(rele[contador].medicion, 3);
      rele[contador].corriente = value * 5 * ki / 1023;
      Serial.print("W");
      Serial.print(contador + 1);
      Serial.print("=");
      Serial.print(rele[contador].Potencia);
      Serial.println(" W");
      Serial.print("i");
      Serial.print(contador + 1);
      Serial.print("=");
      Serial.print(rele[contador].corriente);
      Serial.println(" A");
      Serial.println(value* 5/1023);
      /*if (rele[contador].medicion / voltage > 0.5) {
        rele[contador].On = false;
        Serial.print("W");
        Serial.print(contador + 1);
        Serial.println(" Alta corriente");
      }*/
    } else {
      voltage = value * 5 * kv / 1023;
      if (rele[0].On == false && rele[1].On == false && rele[2].On == false && rele[3].On == false) {
        Serial.print("V=");
        Serial.print(voltage);
        Serial.println(" V");
      }
    }
    value=0;
  }
  contador = (contador + 1) % (Equipos + 1);
  // Para el Multiplexor
  digitalWrite(7,LOW); //contador % 2 > 0);
  digitalWrite(6,LOW); // contador % 4 > 1);
  //digitalWrite(8, contador%8>3); Lo use pa ve si estaban bien los estados
  delay(100);
}
/*------------------------------------------
Escucha
------------------------------------------*/
void Escuchar()  //Mira el serial y responde
{
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      procesarCadena(Cadena, 1);
      Cadena = "";
    } else {
      Cadena += c;
    }
  }
}
void procesarCadena(String texto, int w) {
  int inicio = 0;
  int coma = texto.indexOf(',');
  while (coma != -1) {
    String comando = texto.substring(inicio, coma);
    ejecutar(comando, w);
    inicio = coma + 1;
    coma = texto.indexOf(',', inicio);
  }
  ejecutar(texto.substring(inicio), w);  // Último comando si no termina en coma
}
void ejecutar(String comando, int w) {
  if (comando.startsWith("RE")) {
    int num = comando.substring(2).toInt() - 1;  // obtiene la parte después de "RE", corresponden a rele[0], rele[1]…
    if (num > -1 && num < Equipos) {             // validamos rango pa evitar problemas
      rele[num].On = !rele[num].On;
      if (w == 0) {
        Serial.print("RE");
        Serial.print(num + 1);
        Serial.println("=");
      }
    }
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
  client.println(F("<style>body{font-family:Arial;text-align:center;background:#888;}"));
  client.println(F(".panel{display:inline-block;margin:10px;padding:10px;width:220px;border-radius:8px;color:white;}"));
  client.println(F(".button{margin:5px;padding:10px 20px;font-size:16px;color:white;border:none;cursor:pointer;border-radius:5px;transition:0.3s;}"));
  client.println(F(".on{background:green;}.on:hover{background:lime;}.off{background:red;}.off:hover{background:tomato;}"));
  client.println(F(".med{margin-top:10px;padding:5px;background:#000;border-radius:4px;}</style></head><body>"));
  client.println(F("<h1>Panel de Control</h1>"));
  // Paneles de cada LED
  const char *colores[] = { "#e53935", "#1e88e5", "#43a047", "#fb8c00", "#e53935", "#1e88e5", "#43a047", "#fb8c00", "#e53935", "#1e88e5", "#43a047", "#fb8c00", "#e53935", "#1e88e5", "#43a047", "#fb8c00" };  //Hexadecimal 2 cifras paa RGB
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
    client.println(F("'>Medición: - W</div></div>"));
  }
  // Caja para enviar comentarios
  client.println(F("<h2>Enviar comentario</h2>"));
  client.println(F("<input type='text' id='comentario' maxlength='20'>"));
  client.println(F("<button onclick='enviarComentario()'>Enviar</button>"));
  client.println(F("<p id='resp'></p>"));
  // Script Javascript para AJAX
  client.println(F("<script>"));
  client.println(F("function toggle(i){var x=new XMLHttpRequest();x.open('GET','/toggle'+i,true);x.send();}"));
  client.println(F("function actualizar(){var x=new XMLHttpRequest();x.open('GET','/estado',true);x.onload=function(){if(x.status==200){var d=x.responseText.trim().split(';');for(let i=0;i<d.length;i++){var p=d[i].split(',');document.getElementById('estado'+i).textContent='LED '+(i+1)+': '+(p[0]=='1'?'ENCENDIDO':'APAGADO');document.getElementById('medicion'+i).textContent='Medición: '+p[1]+' W';var b=document.getElementById('boton'+i);if(p[0]=='1'){b.textContent='Apagar';b.className='button off';}else{b.textContent='Encender';b.className='button on';}}}};x.send();}"));
  client.println(F("function enviarComentario(){var c=document.getElementById('comentario').value;var x=new XMLHttpRequest();x.open('GET','/comentario?c='+encodeURIComponent(c),true);x.onload=function(){document.getElementById('resp').textContent='Comentario enviado';};x.send();}"));
  client.println(F("setInterval(actualizar,100);</script></body></html>"));
}
void manejarToggle(EthernetClient &client, String req) {  // Cambia el estado de un LED
  int led = req.charAt(req.indexOf("/toggle") + 7) - '0';
  rele[led].On = !rele[led].On;
  Serial.print("RE");
  Serial.print(led + 1);
  Serial.println("= 0");
  Ok(client);  // responde con cabecera
  client.println(F("OK"));
}
void enviarEstado(EthernetClient &client) {  // Envía el estado actual y medición de todos los LEDs
  Ok(client);
  for (int i = 0; i < Equipos; i++) {
    client.print(rele[i].On ? "1" : "0");
    client.print(",");
    client.print(rele[i].medicion, 2);
    if (i < Equipos - 1) client.print(";");
  }
  client.println();
}
void manejarComentario(EthernetClient &client, String req) {
  int idx = req.indexOf("?c=");
  if (idx >= 0) {  // extrae la subcadena después de ?c=
    String com = req.substring(idx + 3, req.indexOf(" ", idx));
    procesarCadena(com, 0);
  }
  Ok(client);  // responde al navegador
  client.println(F("OK"));
}
/*------------------------------------------
Pantalla LCD
------------------------------------------*/
void mostrarCanal(int n) {  // Muestra cada canal (Wn e In)
  lcd.clear();
  if (rele[n].On) {
    lcd.setCursor(0, 0);  // Fila 1 ? Potencia
    lcd.print("W");
    lcd.print(n + 1);
    lcd.print("=");
    lcd.print(rele[n].Potencia);
    lcd.print(" W");
    lcd.setCursor(0, 1);  // Fila 2 ? Corriente y nivel
    lcd.print("I");
    lcd.print(" ");
    lcd.print(rele[n].corriente, 2);
    lcd.print(" A");
    lcd.print(indicadorCorriente(rele[n].corriente));
    lcd.print(" ");
  } else {
    lcd.print("Equipo ");
    lcd.print(n + 1);
    lcd.print(" Apagado");
  }
}
void mostrarTension() {  // Muestra la pantalla de tensión + Estados de lascorrientes
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V=");
  lcd.print(voltage, 2);
  lcd.print(" V");
  lcd.setCursor(0, 1);
  for (int i = 0; i < 4; i++) {
    if (rele[i].On) {
      lcd.print("I");
      lcd.print(i + 1);
      lcd.print(indicadorCorriente(rele[i].corriente));
      if (i < 3) lcd.print(" ");
    }
  }
}
String indicadorCorriente(float valor) {  // Genera los signos "!" según el nivel de corriente
  if (valor / Imax < 0.25) return " ";
  else if (valor / Imax < 0.5) return " !";
  else if (valor / Imax < 0.75) return " !!";
  else return " !!!";
}
#include <SPI.h>
#include <Ethernet.h>
// Configuración de red
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 0, 0, 178);

EthernetServer server(80);
struct Rele {
	int pin;
	float medicion;
	bool On = true;
	String corriente, Potencia;
};
String Cadena = "";
Rele rele[4];
float lectura = 0, valor_inst = 0, value = 0, voltage, ki = 0.1470588 /*5/34*/, kv = 68.86270491 /* 55 * 611 / (8 * 51)*/;
int i, numero_muestrasAC = 6000, Equipos = 4, contador;
/*Sincronismo mediante la variable Equipos, está me define que dato envio a labview:
0 I1,
1 I2
2 I3
3 I4
4 Tensión
*/
//Inicialización del programa//

void setup() {
	contador =Equipos;
	Serial.begin(115200);
//	pinMode(A0, INPUT);  //Mido corrientes
//	pinMode(A1, INPUT);  //Mido tensiones
	for (int i = 0; i < Equipos; i++) {
		rele[i].pin = (Equipos+1) - i; //No usar 0 1, UART; 10 11 12 13 Ethernet;
		pinMode(rele[i].pin, OUTPUT);
		digitalWrite(rele[i].pin, LOW);
	}
	Ethernet.begin(mac, ip);
	server.begin();
	Serial.print("Servidor iniciado en: ");
	Serial.println(Ethernet.localIP());
}

// Programa Principal//

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
	Web();
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
	lectura = 0;         // Destinado a la obtención de los ADC
	valor_inst = 0;      // Valor requerido
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
	if (contador < Equipos) {   //Sincronismo mediante la variable Equipos, está me define que dato envio a labview.
		value = value * ki;
		rele[contador].corriente = String(value * 5 / 1023, 3);  //Solo si lo solicitan, lo presentamos, si no solo la potencia
		rele[contador].Potencia = String(value * 5 * voltage / 1023, 3);
		rele[contador].medicion=value* 5*voltage / 1023;
		Serial.println(rele[contador].Potencia);
	} else {
		value = value * kv;
		voltage = value * 5 / 1023;
	}
	contador=(contador+1)%(Equipos+1);
	// Para el Multiplexor
	digitalWrite(7, contador%2>0);
	digitalWrite(6, contador%4>1);
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
void Web() {
	EthernetClient client = server.available();
	if (client) {
		String req = "";
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				req += c;
				if (c == '\n') {  // Procesar petición
					ProcesarPeticion(req);
					EnviarPag(client);
					break;
				}
			}
		}
		delay(1);
		client.stop();
	}
}
void ProcesarPeticion(String req) {
	for (int i = 0; i < Equipos; i++) {
		String onCmd = "GET /led" + String(i + 1) + "on";
		String offCmd = "GET /led" + String(i + 1) + "off";
		if (req.indexOf(onCmd) >= 0) {  //Permite que la página y los leds esten vinculados
			rele[i].On = !rele[i].On;
		} else if (req.indexOf(offCmd) >= 0) {
			rele[i].On = !rele[i].On;
		}
	}
}
void EnviarPag(EthernetClient client) {
	client.println(F("HTTP/1.1 200 OK"));
	client.println(F("Content-Type: text/html"));
	client.println(F("Connection: close"));
	client.println();
	client.println(F("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Control de 4 LEDs</title>"));  // Codificación utilizada, coloquen esto en un bloc de notas lo guardan como todos los archivos y el nombre entre "" para visualizar la página sin el módulo
	client.println(F("<style>"));
	client.println(F("body { font-family: Arial; text-align: center; margin-top: 20px; }"));  //Tipo de letra
	client.println(F(".panel { display:inline-block; margin:10px; background:white; padding:10px; border-radius:8px; }"));
	client.println(F(".button { display: inline-block; margin: 5px; padding: 10px 20px; font-size: 16px; color: white; border: none; cursor: pointer; transition: 0.3s; border-radius: 5px; }"));  //Clase de pulsador, me da los tamaños y demás
	client.println(F(".on { background-color: green; } .on:hover { background-color: lime; }"));                                                                                                   //Color Encendido
	client.println(F(".off { background-color: red; } .off:hover { background-color: tomato; }"));
	client.println(F(".med { display: inline-block; margin:10px; padding: 10px 20px;padding:5px; background:#FFC107; }"));
	client.println(F("</style></head><body>"));
	client.println(F("<h1>Panel de control</h1>"));  //Titulo
	for (int i = 0; i < Equipos; i++) {
		//Comienzo, frase Led x Encendido, o Apagado
		client.print(F("<h1>LED "));  //Tamaño de letra, 1 mayor que 3
		client.print(i + 1);
		client.print(F(": "));
		client.print(rele[i].On ? "Encendido" : "APAGADO");
		client.println(F("</h1>"));
		client.print(F("<a href='/led"));  //En conjunto a lo inferior me permite realizar
		client.print(i + 1);               //Un funcionamiento lógico de cada pulsador
		if (!rele[i].On) {
			client.print(F("on'><button class='button on'>Encender LED "));  //El primero me manega la lógica, el segundo el color lógico
		} else {
			client.print(F("off'><button class='button off'>Apagar LED "));
		}
		//client.println(F("</button></a>"));  //Cierra el funcionamiento lógico del boton
		client.println(F("</button></a><br>"));  //Cierra el link del boton y produce un salto
		String colorMed = (rele[i].medicion > 3.0) ? "#f44336" : "#FFC107";
		client.print(F("<div class='med' style='background:"));
		client.print(colorMed);
		client.print(F("'>Medición: "));
		client.print(rele[i].medicion, 2);
		client.println(F(" V</div><br>"));
		client.print(F("<input type='text' placeholder='Comando'> <button>Enviar</button>"));
		client.print(F("</div>"));
	}
	client.println(F("</body></html>"));  //Finaliza la página web
}

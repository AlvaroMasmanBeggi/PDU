#include <SPI.h>
#include <Ethernet.h>

// Configuración de red
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 0, 0, 178);

EthernetServer server(80);
struct Rele {
	int pin;
	bool On = true;
	String corriente, Potencia;
};
String Cadena = "";
Rele rele[4];
const int analogPins[] = { A0, A1, A2, A3 };
int i;
void setup() {
	for (int i = 0; i < 4; i++) {
		rele[i].pin = 5 - i;
		pinMode(rele[i].pin, OUTPUT);
		digitalWrite(rele[i].pin, LOW);
	}
	Ethernet.begin(mac, ip);
	server.begin();
	Serial.begin(9600);
	Serial.print("Servidor iniciado en: ");
	Serial.println(Ethernet.localIP());
}

void loop() {
	Web();
	for (i = 0; i < 4; i++) {
		digitalWrite(rele[i].pin, rele[i].On);
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
	for (int i = 0; i < 4; i++) {
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
	int lectura = analogRead(analogPins[i]);
	float medicion = lectura * (5.0 / 1023.0);
	Serial.println(medicion);
	String colorMed = (medicion > 3.0) ? "#f44336" : "#FFC107";
	client.println("HTTP/1.1 200 OK");
	client.println("Content-Type: text/html");
	client.println("Connection: close");
	client.println();
	client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Control de 4 LEDs</title>");  // Codificación utilizada, coloquen esto en un bloc de notas lo guardan como todos los archivos y el nombre entre "" para visualizar la página sin el módulo
	client.println("<style>");
	client.println("body { font-family: Arial; text-align: center; margin-top: 20px; }");                                                                                                       //Tipo de letra
	 client.println(".panel { display:inline-block; margin:10px; background:white; padding:10px; border-radius:8px; }");
  client.println(".button { display: inline-block; margin: 5px; padding: 10px 20px; font-size: 16px; color: white; border: none; cursor: pointer; transition: 0.3s; border-radius: 5px; }");  //Clase de pulsador, me da los tamaños y demás
	client.println(".on { background-color: green; } .on:hover { background-color: lime; }");                                                                                                   //Color Encendido
	client.println(".off { background-color: red; } .off:hover { background-color: tomato; }");                         
	client.println(".med { display: inline-block; margin:10px; padding: 10px 20px;padding:5px; background:#FFC107; }");
	client.println("</style></head><body>");
	client.println("<h1>Panel de control</h1>");  //Titulo
	for (int i = 0; i < 4; i++) {
		//Comienzo, frase Led x Encendido, o Apagado
		client.print("<h1>LED ");  //Tamaño de letra, 1 mayor que 3
		client.print(i + 1);
		client.print(": ");
		client.print(rele[i].On ? "Encendido" : "APAGADO");
		client.println("</h1>");
		client.print("<a href='/led");  //En conjunto a lo inferior me permite realizar
		client.print(i + 1);            //Un funcionamiento lógico de cada pulsador
		if (!rele[i].On) {
			client.print("on'><button class='button on'>Encender LED ");  //El primero me manega la lógica, el segundo el color lógico
		} else {
			client.print("off'><button class='button off'>Apagar LED ");
		}
		//client.println("</button></a>");  //Cierra el funcionamiento lógico del boton
		client.println("</button></a><br><br>");//Cierra el link del boton y produce un salto
		
		
		
		int lectura = analogRead(analogPins[i]);
		float medicion = lectura * (5.0 / 1023.0);
		String colorMed = (medicion > 3.0) ? "#f44336" : "#FFC107";
		client.print("<div class='med' style='background:");
		client.print(colorMed);
		client.print("'>Medición: ");
		client.print(medicion, 2);
		client.println(" V</div><br><br>");
		client.print("<input type='text' placeholder='Comando'> <button>Enviar</button>");
		client.println("</div>");
	}
	client.println("</body></html>");  //Finaliza la página web
}

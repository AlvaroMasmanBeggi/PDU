//Declaraciones de variables
struct Rele {
  int pin;
  bool Encendido = true;
  String corriente, Potencia;
};
String Cadena = "";
Rele rele[4];
bool Tension = true;
float lectura = 0, valor_inst = 0, value = 0, voltage, ki = 0.1470588 /*5/34*/, kv = 68.86270491 /* 55 * 611 / (8 * 51)*/;
int i, numero_muestrasAC = 6000, Estado = 4, contador = 0;
/*Sincronismo mediante la variable Estado, está me define que dato envio a labview:
0 I1,
1 I2
2 I3
3 I4
4 Tensión
*/
//Inicialización del programa//

void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);  //Mido corrientes
  pinMode(A1, INPUT);  //Mido tensiones
  for (i = 0; i < 4; i++) {
    rele[i].pin = 13 - i;
    pinMode(rele[i].pin, OUTPUT);
    digitalWrite(rele[i].pin, LOW);
  }
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
    if (contador < Estado) {
      lectura = analogRead(A0);
    } else {
      lectura = analogRead(A1);
    }
    valor_inst = lectura * lectura + valor_inst;
    delayMicroseconds(17);  //Periodo de muestreo optimo, para despreciar el tiempo de muestreo necesario
  }
  valor_inst /= numero_muestrasAC;
  value = sqrt(valor_inst);  //Variable global con la información necesaria
  if (contador < Estado) {   //Sincronismo mediante la variable Estado, está me define que dato envio a labview.
    value = value * ki;
    rele[contador].corriente = String(value * 5 / 1023, 3);  //Solo si lo solicitan, lo presentamos, si no solo la potencia
    rele[contador].Potencia = String(value * 5 * voltage / 1023, 3);
    Serial.println(rele[contador].Potencia);
    contador++;
  } else {
    value = value * kv;
    voltage = value * 5 / 1023;
    contador = 0;
  }
  switch (contador) {  // Aprovecho el Multiplexor con A0:
    case 0:
      {
        digitalWrite(7, LOW);
        digitalWrite(6, LOW);
      }
      break;
    case 1:
      {
        digitalWrite(7, HIGH);
        digitalWrite(6, LOW);
      }
      break;
    case 2:
      {
        digitalWrite(7, LOW);
        digitalWrite(6, HIGH);
      }
      break;
    case 3:
      {
        digitalWrite(7, HIGH);
        digitalWrite(6, HIGH);
      }
      break;
  }
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
    digitalWrite(rele[0].pin, rele[0].Encendido);
    rele[0].Encendido = !rele[0].Encendido;
  }
  if (comando == "RE2") {
    digitalWrite(rele[1].pin, rele[1].Encendido);
    rele[1].Encendido = !rele[1].Encendido;
  }
  if (comando == "RE3") {
    digitalWrite(rele[2].pin, rele[2].Encendido);
    rele[2].Encendido = !rele[2].Encendido;
  }
  if (comando == "RE4") {
    digitalWrite(rele[3].pin, rele[3].Encendido);
    rele[3].Encendido = !rele[3].Encendido;
  }
}

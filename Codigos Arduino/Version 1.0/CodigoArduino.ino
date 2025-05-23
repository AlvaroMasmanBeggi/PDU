//Declaraciones de variables//

String Cadena = "";
int rele1 = 13;
int rele2 = 12;
bool RE1 = false, RE2 = false;
float ValorA0 = 0, lectura = 0, valor_inst = 0, volt_value = 0;
int numero_muestrasAC = 6000, contador = 0;

//Inicialización del programa//

void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  digitalWrite(rele1, LOW);
  digitalWrite(rele2, LOW);
}

//¨Programa Principal//

void loop() {
  lectura_tensionAC();
  while (Serial.available()) {
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
    digitalWrite(rele1, RE1);
    RE1 = !RE1;
  } else if (comando == "RE2") {
    digitalWrite(rele2, RE2);
    RE2 = !RE2;
  }
}

void lectura_tensionAC() {
  String Volt_Enviar = "";
  lectura = 0;
  valor_inst = 0;
  for (int i = 0; i < numero_muestrasAC; i++) {
    lectura = analogRead(A0);
    valor_inst = lectura * lectura + valor_inst;
    delayMicroseconds(17);
  }
  valor_inst /= numero_muestrasAC;
  volt_value = sqrt(valor_inst);
  Volt_Enviar = String(volt_value* 5 / 1023, 3);
  Serial.println(Volt_Enviar);
}

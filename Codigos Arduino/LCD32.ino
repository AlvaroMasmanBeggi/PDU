#include <LiquidCrystal_I2C.h>

// Dirección del módulo I2C (común: 0x27 o 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);
struct Rele {
  int pin;
  float corriente, medicion = 0;
  bool On = true;
  String Potencia;
};
Rele rele[4];
float V;
int i, Equipos = 4, T = 0, S, A = 0;
float Imax = 3.0;  // Corriente máxima
// ---------------------------------------------------------------
void setup() {
  lcd.init();
  lcd.backlight();
  S = Equipos;
  for (int i = 0; i < 4; i++) {
    rele[i].corriente = 1* i;
    rele[i].medicion = 10 * i;
    rele[i].Potencia = "10";
  }}
  void loop() {
    T = (T + 1) % 32000;  //Manejo el tiempo en función de pasada
    if (S != A) {         //Solo lo escribo cuando A cambia
      if (S < Equipos) {
        mostrarCanal(S);
      } else if (S == Equipos) {
        mostrarTension();
      }
      A = S;
    }
    if (T == 2499) {
      S = (S + 1) % (Equipos+1);
    }
  }
  void mostrarCanal(int n) {  // Muestra cada canal (Wn e In)
    lcd.clear();
    if (rele[n].On) {
      lcd.setCursor(0, 0);  // Fila 1 → Potencia
      lcd.print("W");
      lcd.print(n + 1);
      lcd.print("=");
      lcd.print(rele[n].Potencia);
      lcd.print(" w");
      lcd.setCursor(0, 1);  // Fila 2 → Corriente y nivel
      lcd.print("I");
      lcd.print(" ");
      lcd.print(rele[n].corriente, 2);
      lcd.print("A");
      lcd.print(indicadorCorriente(rele[n].corriente));
      lcd.print(" ");
    } else {
      lcd.print("Apagado");
    }
  }
  void mostrarTension() {  // Muestra la pantalla de tensión + Estados de lascorrientes
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("V=");
    lcd.print(V, 2);
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
    float rel = valor / Imax;
    if (rel < 0.25) return " ";
    else if (rel < 0.5) return "!";
    else if (rel < 0.75) return "!!";
    else return "!!!";
  }

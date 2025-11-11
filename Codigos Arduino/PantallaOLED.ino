// Este código es para probar una pantalla OLED de 0.96" I2C
// con driver SSD1306 usando las librerías de Adafruit.

// Asegúrate de haber instalado las librerías "Adafruit GFX Library" y "Adafruit SSD1306"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuración de la pantalla
#define SCREEN_WIDTH 128 // Ancho en píxeles de la pantalla OLED (ej. 128)
#define SCREEN_HEIGHT 64 // Altura en píxeles de la pantalla OLED (ej. 64 o 32)

// Dirección I2C del módulo OLED (puede ser 0x3C o 0x3D).
// Si el display no funciona con 0x3C, prueba con 0x3D.
#define OLED_RESET -1 // Pin de reset (se usa -1 para pantallas I2C sin pin de reset)
#define SCREEN_ADDRESS 0x3C 

// Crea el objeto de la pantalla
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);

  // Inicializa la comunicación I2C con la dirección especificada.
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("¡Error! La inicialización del SSD1306 falló."));
    for(;;); // No continuar si falla
  }

  // Borra el búfer para iniciar con una pantalla limpia
  display.clearDisplay();

  // --- CONFIGURACIÓN DE TEXTO ---
  display.setTextSize(1);      // Tamaño de texto normal (1x1)
  display.setTextColor(SSD1306_WHITE); // Color del texto (blanco)
  display.setCursor(0, 0);     // Mueve el cursor a la esquina superior izquierda (0,0)
  display.print("  PRUEBA OLED I2C  ");
  
  // Línea separadora
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);

  // --- TEXTO GRANDE Y POSICIONAMIENTO ---
  display.setCursor(0, 15);
  display.setTextSize(2);
  display.print("ARDUINO");

  // --- DIBUJO DE GRÁFICOS (Ejemplo de rectángulo y círculo) ---
  display.drawRect(5, 40, 50, 20, SSD1306_WHITE); // x, y, ancho, alto
  display.fillCircle(90, 50, 10, SSD1306_WHITE);  // x, y, radio

  display.setCursor(70, 50);
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Texto invertido (fondo blanco)
  display.print("OK");

  // Muestra todo el contenido del búfer en la pantalla
  display.display();
}

void loop() {
  // En este ejemplo simple, no hacemos nada en el loop
  // para mantener la pantalla estática después de la prueba inicial.
}
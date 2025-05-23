//Otero-Sznajdleder-Bairros  Grupo 1
TaskHandle_t Tarea1;
TaskHandle_t Tarea2;
#include <U8g2lib.h>
#include "DHT.h"
#include <WiFi.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


void printBMP_OLED(void );
void printBMP_OLED2(void) ;
#define BOTON1 34
#define BOTON2 35
#define LED 25
#define P1 0
#define P2 1
#define RST 20
#define ESPERA1 2
#define ESPERA2 3
#define ESPERA3 21
#define AUMENTAR 4
#define RESTAR 5
int estado = RST;
#define DHTPIN 23
#define DHTTYPE DHT11
#define BOTtoken "7588398058:AAEviInMnNCT7zbqz89vuKEnT-hg-bg8OD4"
#define CHAT_ID "5941222238"
DHT dht(DHTPIN, DHTTYPE);
float temp;
unsigned long millis_valor;
unsigned long millis_actual;
unsigned long millis_valor2;
unsigned long millis_actual2;
int valorU = 26;
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
void CodigoTarea1(void *pvParameters);
void CodigoTarea2(void *pvParameters);
bool alertaEnviada = false;

const char* ssid = "ORT-IoT";
const char* password = "NuevaIOT$25";
const char* ntpServer = "pool.ntp.org";

// LED pins
const int led1 = 25;
const int led2 = 26;

void setup() {
  Serial.begin(115200);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(BOTON1, INPUT_PULLUP);
  pinMode(BOTON2, INPUT_PULLUP);
  Serial.println(F("DHTxx test!"));
  u8g2.begin();
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  String lastText = "";
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  bot.sendMessage(CHAT_ID, "Bot started up", "");


  xTaskCreatePinnedToCore(
    CodigoTarea1,   /* Task function. */
    "Tarea1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Tarea1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(500);

  xTaskCreatePinnedToCore(
    CodigoTarea2,   /* Task function. */
    "Tarea2",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Tarea2,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */
  delay(500);

}

void CodigoTarea1(void *pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    millis_actual = millis();
    if (millis_actual - millis_valor >= 2000) {
      millis_valor = millis_actual;  // actualizar para el próximo ciclo
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      temp = dht.readTemperature();
      if (isnan(temp)) {
        Serial.println("Error al leer del sensor DHT");
        continue;  // sigo al próximo ciclo sin hacer nada más
      }

      Serial.print("Temperatura actual: ");
      Serial.println(temp);

      if (temp > valorU && alertaEnviada == false) {
        String mensaje = "⚠️ ¡Alerta! Temperatura mayor al valor umbral: " + String(valorU) + ", Temperatura: " + String(temp) + " °C";
        bot.sendMessage(CHAT_ID, mensaje, "");
        digitalWrite(led1, HIGH);
        alertaEnviada = true;

      } else if (temp <= valorU && alertaEnviada == true) {
        bot.sendMessage(CHAT_ID, "✅ Temperatura menor al valor umbral: " + String(valorU) + ", Temperatura: " + String(temp) + " °C", "");
        digitalWrite(led1, LOW);
        alertaEnviada = false;
      }

    }

  }
}



void CodigoTarea2( void * pvParameters ) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) { //si o si tiene que ir un while o un for para que nunca salga de la tarea
    millis_actual2 = millis();
    if (millis_actual2 - millis_valor2 >= 2000) {
      temp = dht.readTemperature();
      if (isnan(temp)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
    }
    switch (estado) {
      case RST:
        {
          millis_valor2 = millis();
          estado = P1;
        }
        break;
      case P1:
        {
          printBMP_OLED();
          if (digitalRead(BOTON1) == LOW) {
            millis_valor2 = millis();
            estado = ESPERA1;
          }
        }
        break;
      case ESPERA1:
        {
          if (digitalRead(BOTON1) == HIGH && digitalRead(BOTON2) == LOW) {
            estado = ESPERA3;
          }
          if (millis_actual2 - millis_valor2 >= 5000) {
            millis_valor2 = millis();
            estado = P1;
          }

        }
        break;
      case ESPERA3:
        {
          if (digitalRead(BOTON2) == HIGH && digitalRead(BOTON1) == LOW ) {
            estado = P2;
          }
          if (millis_actual2 - millis_valor2 >= 5000) {
            estado = P1;
          }

        }
        break;
      case P2:
        {
          if (digitalRead(BOTON1) == HIGH) {
            printBMP_OLED2();
            if (digitalRead(BOTON1) == LOW) {
              estado = AUMENTAR;
            }
            if (digitalRead(BOTON2) == LOW) {
              estado = RESTAR;
            }
            if (digitalRead(BOTON1) == LOW && digitalRead(BOTON2) == LOW) {
              estado = ESPERA2;
            }
          }
        }
        break;
      case ESPERA2:
        {
          if (digitalRead(BOTON1) == HIGH && digitalRead(BOTON2) == HIGH) {
            estado = P1;
          }
        }
        break;

      case AUMENTAR:
        {
          if (digitalRead(BOTON2) == LOW) {
            estado = ESPERA2;
          }
          if (digitalRead(BOTON1) == HIGH) {
            valorU = valorU + 1;
            estado = P2;
          }

        }
        break;

      case RESTAR:
        {
          if (digitalRead(BOTON1) == LOW) {
            estado = ESPERA2;
          }
          if (digitalRead(BOTON2) == HIGH) {
            valorU = valorU - 1;
            estado = P2;
          }

        }
        break;
    }
  }
}




void loop() {

}


void printBMP_OLED(void) {
  char stringU[5];
  char stringtemp[6];
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_11b_tr); // choose a suitable font
  sprintf (stringtemp, "%.2f" , temp); ///convierto el valor float a string
  sprintf (stringU, "%d" , valorU); ///convierto el valor float a string
  u8g2.drawStr(0, 35, "T. Actual:");
  u8g2.drawStr(60, 35, stringtemp);
  u8g2.drawStr(90, 35, "°C");
  u8g2.drawStr(0, 50, "V. Umbral:");
  u8g2.drawStr(60, 50, stringU);
  u8g2.drawStr(75, 50, "°C");
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void printBMP_OLED2(void) {
  char stringU[5];
  u8g2.clearBuffer();          // clear the internal memory
  sprintf (stringU, "%d" , valorU);
  u8g2.setFont(u8g2_font_t0_11b_tr); // choose a suitable font
  u8g2.drawStr(0, 50, "V. Umbral:");
  u8g2.drawStr(60, 50, stringU);
  u8g2.drawStr(75, 50, "°C");
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;


    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Usuario no autorizado");
      continue;
    }
    if (text != "") {
      if (temp > valorU ) {
        String mensaje = "Temperatura: " + String(temp) + " °C";
        bot.sendMessage(CHAT_ID, mensaje, "");
      }
      if (temp <= valorU ) {
        bot.sendMessage(CHAT_ID, "Temperatura: " + String(temp) + " °C", "");
      }
    }
  }
}

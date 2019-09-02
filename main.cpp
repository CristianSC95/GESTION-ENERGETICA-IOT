// Librerías para medir consumo
#include <Wire.h>
#include <Adafruit_ADS1015.h>
// Librerias para WiFi y MQTT
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Datos de la red---------------------------
const char* ssid = "MiRED";  // WRTred
const char* password = "electronica"; // electronica
const char* mqtt_server = "192.168.31.130";  // 192.168.1.130

// Cliente WiFi en MQTT-----------------------
const char* mqtt_user = "hassbian-mqtt";
const char* mqtt_pswd = "electronica";
String mqttID="SCT-013-030";
const char* topic="casa/demanda";
char msg[10];
WiFiClient espClient;
PubSubClient client(espClient);

// Instancia del ADS--------------------------
Adafruit_ADS1115 ads;

// Factor corriente/tensión de la sonda-------
const float FACTOR = 30; //30A/1V

// Resolución usada para la sonda-------------
const float multiplier = 0.0625F;

int tiempo_muestreo = 1500;

//--------------------------------------------
//                FUNCIONES
//--------------------------------------------
// Conexión WiFi------------------------------
void setup_wifi() {

  delay(10);
  // Texto que indica que está conectando
  Serial.println();
  Serial.print("Conectando con ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  //Espera a conexión
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Texto que indica que se ha establecido conexión
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Función de conexión a broker MQTT----------
void reconnect() {
  // Espera hasta conexión con broker MQTT
  while (!client.connected()) {
    Serial.print("Esperando conexión MQTT...");
    // Creación de ID aleatorio
    String clientId = mqttID + " :";
    clientId += String(random(0xffff), HEX);
    // Intento de conexión
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pswd)) {
      Serial.println("conectado");
      // Conectado, publicar un anuncio...
      client.publish(topic, "Midiendo consumo vivienda");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintento en 5 segundos");
      // Espera 5 segundos antes del reintento
      delay(5000);
    }
  }
}

// Función de impresión de una medida---------
void printMeasure(String prefix, float value, String postfix)
{
 Serial.print(prefix);
 Serial.print(value, 3);
 Serial.println(postfix);
}

// Función de medición de la corriente en Irms
float getCorriente(int titer)
{
 float voltage;
 float corriente;
 float sum = 0;
 long tiempo = millis();
 int counter = 0;

 while (millis() - tiempo < titer)
 {
   voltage = ads.readADC_Differential_0_1() * multiplier;
   corriente = voltage * FACTOR;
   corriente /= 1000.0;

   sum += sq(corriente);
   counter = counter + 1;
  }

 corriente = sqrt(sum / counter);
 return(corriente);
}

//--------------------------------------------
//            PROGRAMA PRINCIPAL
//--------------------------------------------
// Inicialización-----------------------------
void setup()
{
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  setup_wifi();  // Conexión a la red WiFi
  client.setServer(mqtt_server, 1883);// establece el servidor MQTT
  // ads.setGain(GAIN_TWOTHIRDS);  +/- 6.144V  1 bit = 0.1875mV (default)
  // ads.setGain(GAIN_ONE);        +/- 4.096V  1 bit = 0.125mV
  // ads.setGain(GAIN_TWO);        +/- 2.048V  1 bit = 0.0625mV
  // ads.setGain(GAIN_FOUR);       +/- 1.024V  1 bit = 0.03125mV
  // ads.setGain(GAIN_EIGHT);      +/- 0.512V  1 bit = 0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    +/- 0.256V  1 bit = 0.0078125mV
  ads.setGain(GAIN_TWO); // ±2.048V  1 bit = 0.0625mV
  ads.begin();
}

// Bucle infinito
void loop()
{
 digitalWrite(LED_BUILTIN,HIGH);

 // Parte encargada de medir la potencia
 Serial.print("Tiempo de interación: ");
 Serial.println(tiempo_muestreo);
 float currentRMS = getCorriente(tiempo_muestreo);
 float power = 230.0 * currentRMS;

 printMeasure("Irms: ", currentRMS, "A ,");
 printMeasure("Potencia: ", power, "W");

 // Parte encargada del envío por MQTT
 if (!client.connected()) {
   reconnect();    //Conexión como cliente a MQTT
 }
 client.loop();

 // Formato de publicación del mensaje
 snprintf (msg, 10, "%f", power);
 Serial.print("Mensaje publicado: ");
 Serial.println(msg);
 client.publish(topic, msg);

 digitalWrite(LED_BUILTIN,LOW);

 delay(1000);
}

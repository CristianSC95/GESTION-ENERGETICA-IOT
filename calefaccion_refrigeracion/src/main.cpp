//==================================================================//
//                                                                  //
//                                                                  //
//                                                                  //
//                                                                  //
//==================================================================//


//LIBRERIAS----------------------------------------------------------
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Datos de la red
const char* ssid = "MiRED";  // WRTred
const char* password = "electronica"; // electronica
const char* mqtt_server = "192.168.31.130";  // 192.168.1.130

// Sonda DS18B20
const int pinSonda = 5;
OneWire oneWireSonda(pinSonda);
DallasTemperature sensorDS18B20(&oneWireSonda);
float temperatura;
int t_min = 20;
int t_nom = 23;
int t_max = 26;
int estado_cal = 0;
int estado_ref = 0;
int t_act = 10000;
int tiempo = 0;

// Cliente WiFi en MQTT
const char* mqtt_user = "hassbian-mqtt";
const char* mqtt_pswd = "electronica";
String mqttID="cal_ref";
WiFiClient espClient;
PubSubClient client(espClient);
const char* topic="casa/cal_ref";
const char* topic_temperatura="casa/salon/temperatura";
char msg[10];
int autorizacion = 0;
int rele_cal = 12;
int rele_ref = 13;

//--------------------------------------------
//                FUNCIONES
//--------------------------------------------
// Conexión WiFi------------------------------
void setup_wifi() {
  delay(10);
  // Texto que indica que está conectando
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  //Espera a conexión
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Texto que indica que se ha establecido conexió
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Función de recepción de menajes del broker MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  switch ((char)payload[0]) {
    case '0':
        // No autorizado a funcionar si se recibe un 0
        autorizacion = 0;
        break;
    case '1':
        // Autorizado a funcionar si se recibe un 1
        autorizacion = 1;
        break;
    case '2':
        // No autorizado a funcionar si se recibe un 2
        // Activa calefacción durante 5 seg.
        autorizacion = 0;
        digitalWrite(rele_cal,HIGH);
        delay(5000);
        digitalWrite(rele_cal,LOW);
        break;
  }
}

// Función de conexión a broker MQTT----------
void reconnect() {
  // Espera hasta conexión con broker MQTT
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Creación de ID aleatorio
    String clientId = mqttID + " :";
    clientId += String(random(0xffff), HEX);
    // Intento de conexión
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pswd)) {
      Serial.println("connected");
      // Conectado, suscribirse
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Espera 5 segundos antes del reintento
      delay(5000);
    }
  }
}

void setup() {
  pinMode(rele_cal, OUTPUT);
  pinMode(rele_ref, OUTPUT);
  Serial.begin(115200);
  sensorDS18B20.begin();
  setup_wifi();                     // Conexión a la red WiFi
  client.setServer(mqtt_server, 1883);// establece el servidor MQTT
  client.setCallback(callback);     // Suscripción a un topic
}

void loop() {
  if (!client.connected()) {
    reconnect();    //Conexión como cliente MQTT
  }
  client.loop();
  // Lectura temperatura sonda
  Serial.println("Midiendo tempertaturas.");
  sensorDS18B20.requestTemperatures();
  temperatura = sensorDS18B20.getTempCByIndex(0);
  Serial.print("Temperatura sensor 0: ");
  Serial.print(temperatura);
  Serial.println(" ºC");
  Serial.print("Autorizacion: ");
  Serial.println(autorizacion);

  // Publicación de temperatura cada 10 seg.
  if(millis()-tiempo>=t_act){
    snprintf (msg, 10, "%f", temperatura);
    client.publish(topic_temperatura,msg);
    tiempo=millis();
    Serial.println("Temperatura publicada por MQTT");
  }

  // Control de calefacción y refrigeración según Tª
  if(temperatura>26 && estado_ref==0 && autorizacion==1){
      estado_ref = 1;
      digitalWrite(rele_ref,HIGH);
  }
  else if(temperatura<23 && estado_ref==1){
      estado_ref = 0;
      digitalWrite(rele_ref,LOW);
  }
  else if(temperatura<20 && estado_cal==0 && autorizacion==1){
      estado_cal = 1;
      digitalWrite(rele_cal,HIGH);
  }
  else if(temperatura>23 && estado_cal==1){
      estado_cal = 0;
      digitalWrite(rele_cal,LOW);
  }
  else if(autorizacion==0 &&(estado_cal==1 || estado_ref==1)){
      digitalWrite(rele_cal,LOW);
      digitalWrite(rele_ref,LOW);
      estado_ref = 0;
      estado_cal = 0;
  }

  delay(1000);
}

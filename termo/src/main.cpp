// Librerías para WiFi y para MQTT
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Datos de la red
const char* ssid = "MiRED";  // WRTred
const char* password = "electronica"; // electronica
const char* mqtt_server = "192.168.31.130";  // 192.168.1.130

// Cliente WiFi en MQTT
const char* mqtt_user = "hassbian-mqtt";
const char* mqtt_pswd = "electronica";
String mqttID="termo_ACS";
char msg[50];
WiFiClient espClient;
PubSubClient client(espClient);

const char* topic="casa/termo";
int estado = 0;

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

  // Texto que indica que se ha establecido conexión
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

  // Apaga el termo si se recibe un 0 y está activo
  if ((char)payload[0] == '0' && estado == 1) {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    estado = 0;
  }
  // Enciende el termo si se recibe un 1 y está desactivo
  else if((char)payload[0] == '1' && estado == 0){
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    estado = 1;
  }
  // Enciende el termo durante 5 seg. si se recibe un 2
  else if((char)payload[0] == '2'){
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);
    estado = 0;
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
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();                     // Conexión a la red WiFi
  client.setServer(mqtt_server, 1883);// establece el servidor MQTT
  client.setCallback(callback);     // Suscripción a un topic
}

void loop() {

  if (!client.connected()) {
    reconnect();    //Conexión como cliente MQTT
  }
  client.loop();
}

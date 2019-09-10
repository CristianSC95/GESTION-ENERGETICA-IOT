#!/usr/local/bin/python
# coding: latin-1

import sys
import ssl

#librería paho para mqtt
import paho.mqtt.client as mqtt
#librería time para hacer controles de tiempo
import time

id_cliente = "productor"
broker = "127.0.0.1"
puerto = 1883
topic = "casa/produccion"
#Producción aproximada en cada hora en punto
produccion = [0, 0, 0, 0, 0, 0, 0, 0, 300, 1000, 1800, 2500, 3000, 3200, 3000, 2500, 1700, 1000, 250, 0, 0, 0, 0, 0]
 

def on_connect(client, userdata, flags, rc):
	if rc == 0:
		print('Conexión OK (%s)' % client._client_id)
	else:
		print('Conexión fallida, código de error ',rc)

def on_disconnect(client, userdata, flags, rc=0):
	print('Código de desconexión '+str(rc))

client = mqtt.Client(client_id=id_cliente)

client.on_connect = on_connect
client.on_disconnect = on_disconnect

print('Conectando al broker ',broker)

#identificación en el broker mqtt
client.username_pw_set(username="mqtt-user",password="mqtt-password")

client.connect(host=broker, port=puerto)

client.loop_start()

while True:
	for potencia in produccion:
        #Publica el valor de producción
		client.publish(topic,potencia)
		time.sleep(7.5)


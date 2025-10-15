#include <Arduino.h>
#include <WiFi.h> 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const int ledChannel =0;
const int ledChanne2 =1;

// Pines para los servos
const int servoPin = 18; // Primer servo
// Canales PWM
const int servoChannel = 2; // Canal exclusivo para el servo
const int servoFrequency = 50; // Frecuencia de 50 Hz para servos
const int servoResolution = 16; // Resolución de 16 bits

const int frequency = 5000;
const int resolution = 8;

int ultimo_estado = LOW;
volatile bool cambio_pendiente = false; // Bandera para indicar cambio de dirección
bool sentido = true;
const byte motorIzquierdoA = 14;
const byte motorIzquierdoB = 27;

const byte enableIzquierdoA = 12;


const byte motorDerechoA = 26;
const byte motorDerechoB = 25;

const byte enableDerechoB = 13;

const byte trigg = 4;
const byte echo =  19;

int velocidad= 0;

byte velocidad_inicial = 35;
byte velocidad_maxima = 255;
int angle = 0;


Servo mi_servo;
Servo mi_servo2;
const byte ang_max = 161; 
const byte ang_min = 0;

#define SCREEN_WIDTH 128 // Ancho del display en píxeles
#define SCREEN_HEIGHT 64  // Alto del display en píxeles

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "Totalplay-39A6";
//Montse Movis
//BUAP_Estudiantes
//Totalplay-39A6
const char* password = "39A6D7838DkTTB22";
//f85ac21de4
//39A6D7838DkTTB22
//montse1234
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_pass = "";
WiFiClient espClient;
PubSubClient client(espClient);

void moverAtras(byte vel);
void moverAdelante(byte vel);
void controlMotor(byte dirA, byte dirB, byte enablePin, bool sentido, byte vel);
void girarDerecho(byte vel);
void apagarMotor(byte dirA, byte dirB, byte enablePin);
void girarIzquierdo(byte vel);
void detener(byte vel);
bool esDireccionOpuesta(String actual, String nueva);
void writeServoAngle(int angle);
int mide_distancia();
void gatillo();

String ultimaDireccion = "adelante"; // Variable para almacenar la última dirección
void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Mensaje recibido bajo el tópico -> ");
    Serial.println(topic);
    Serial.print("Mensaje recibido: ");
    Serial.println(message);

  // Verificar distancia antes de permitir avanzar
    int distancia = mide_distancia(); // Mide la distancia al recibir un comando
    if (distancia <= 20 && message == "adelante") {
        // Publicar alerta en el tópico `david`
        client.publish("david", "Obstáculo detectado, movimiento bloqueado");
        Serial.println("Obstáculo detectado, movimiento hacia adelante bloqueado");

        // Detener el motor
        apagarMotor(motorDerechoA, motorDerechoB, ledChanne2); // Detener motor derecho
        apagarMotor(motorIzquierdoA, motorIzquierdoB, ledChannel); // Detener motor izquierdo

        return; // Salir sin ejecutar el comando de avance
    }

      if (message.startsWith("servo_")) {
        int angle = message.substring(6).toInt(); // Extraer el ángulo después de "servo_"
        if (angle >= 0 && angle <= 180) {
                writeServoAngle(angle); 
            Serial.print("Servo movido a: ");
            Serial.println(angle);
        } else {
            Serial.println("Ángulo fuera de rango para el servo");
        }
        return; // Salir porque ya se manejó el comando
    }
    else if (message.startsWith("motor_")) {
     String command = message.substring(6); // Extrae el comando después de "motor_"

    if (command == "stop") {
        ultimaDireccion = "stop";
        detener(30);
        Serial.println("El carro se detuvo");
    }

    if (esDireccionOpuesta(ultimaDireccion, command)) {
    detener(0);  // Detener todos los motores
    delay(200);  // Pausa breve antes de cambiar dirección
    }
    if (command == "adelante") {
        ultimaDireccion = "adelante";
        moverAdelante(velocidad);
        Serial.println("Moviendo hacia adelante");
    } else if (command == "atras") {
        ultimaDireccion = "atras";
        moverAtras(velocidad);
        Serial.println("Moviendo hacia atrás");
    } else if (command == "derecha") {
        ultimaDireccion = "derecha";
        girarDerecho(velocidad);
        Serial.println("Girando a la derecha");
    }
    else if (command == "izquierda") {
        ultimaDireccion = "izquierda";
        girarDerecho(velocidad);
        Serial.println("Girando a la izquierda");
    }  else {
        int value = command.toInt();
        if (value >= 0 && value <= 255) {
            velocidad = value;
            Serial.print("Velocidad ajustada a: ");
            Serial.println(velocidad);

            if (ultimaDireccion == "adelante") {
                moverAdelante(velocidad);
            } else if (ultimaDireccion == "atras") {
                moverAtras(velocidad);
            } else if (ultimaDireccion == "derecha") {
                girarDerecho(velocidad);
            }
             else if (ultimaDireccion == "izquierda") {
                girarIzquierdo(velocidad);
            }
        } else {
            Serial.println("Mensaje desconocido o fuera de rango");
        }
    }
}
}

void reconnect() {
    Serial.println("Intentando Conexion MQTT");

    String clientId = "puntaje";
    clientId = clientId + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
        Serial.println("Conexion a MQTT exitosa!!");
        client.publish("david", "primer mensaje");
        client.publish("servo/david", "primer mensaje");
        client.subscribe("david");
        client.subscribe("servo/david");
    } else {
        Serial.println("Fallo la conexion");
        Serial.print(client.state());
        Serial.print("Se intentara de nuevo en 5 segundos");
        delay(5000);
    }
}

void setup_wifi() {
    Serial.println("");
    Serial.print("Conectando a ->");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("");
    Serial.println("Conexion exitosa");
    Serial.print("Mi ip es ->");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    delay(5000);
    
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

   // Configuración del PWM para el servo
    ledcSetup(servoChannel, servoFrequency, servoResolution);
    ledcAttachPin(servoPin, servoChannel);

    // Mueve el servo al centro
    writeServoAngle(90); 
    Serial.println("Servo inicializado");

  pinMode(motorDerechoA,OUTPUT);
  pinMode(motorDerechoB,OUTPUT);
  pinMode(motorIzquierdoA,OUTPUT);
  pinMode(motorIzquierdoB,OUTPUT);

    ledcSetup(ledChannel, frequency, resolution);  // Canal 0 para Motor 1
    ledcSetup(ledChanne2, frequency, resolution);  // Canal 1 para Motor 2

ledcAttachPin(enableIzquierdoA, ledChannel);
ledcAttachPin(enableDerechoB, ledChanne2);


}

void loop() {
 if (!client.connected()) {
        reconnect();
    }
    client.loop();
}


void moverAtras(byte vel) {
    controlMotor(motorIzquierdoA, motorIzquierdoB, ledChannel, false, vel);
    controlMotor(motorDerechoA, motorDerechoB, ledChanne2, false, vel);
}
void moverAdelante(byte vel) {
    controlMotor(motorIzquierdoA, motorIzquierdoB, ledChannel, true, vel);
    controlMotor(motorDerechoA, motorDerechoB, ledChanne2, true, vel);
}
void controlMotor(byte dirA, byte dirB, byte enablePin, bool sentido, byte vel) {
    if (sentido) {
        digitalWrite(dirA, HIGH);
        digitalWrite(dirB, LOW);
    } else {
        digitalWrite(dirA, LOW);
        digitalWrite(dirB, HIGH);
    }
    ledcWrite(enablePin, vel); // Control de velocidad mediante PWM
}

void girarDerecho(byte vel) {
    // Encender motores A y D
    controlMotor(motorIzquierdoA, motorIzquierdoB, ledChannel, true, vel);  // Motor A (izquierda)
    // Apagar los demás motores
    apagarMotor(motorDerechoA, motorDerechoB, ledChanne2); // Motor B
}

void girarIzquierdo(byte vel) {
    // Encender motores B y C
    controlMotor(motorDerechoA, motorDerechoB, ledChanne2, true, vel);  // Motor B 

    // Apagar los demás motores
    apagarMotor(motorIzquierdoA, motorIzquierdoB, ledChannel); // Motor A
}
void detener(byte vel) {
    // Apagar los demás motores
    apagarMotor(motorDerechoA, motorDerechoB, ledChanne2); // Motor B

    apagarMotor(motorIzquierdoA, motorIzquierdoB, ledChannel); // Motor A

}

void apagarMotor(byte dirA, byte dirB, byte enablePin) {
    digitalWrite(dirA, LOW);  // Desactivar dirección
    digitalWrite(dirB, LOW);  // Desactivar dirección opuesta
    ledcWrite(enablePin, 0);  // Detener PWM en el pin enable
}

bool esDireccionOpuesta(String actual, String nueva) {
     if ((actual == "adelante" && nueva == "atras") ||
        (actual == "atras" && nueva == "adelante") ||
        (actual == "izquierda" && nueva == "derecha") ||
        (actual == "derecha" && nueva == "izquierda")) {
        return true;
    }
    return false;
}


void writeServoAngle(int angle) {
    // Convierte el ángulo (0-180) al pulso PWM en µs
    int dutyCycle = map(angle, 0, 180, 1000 * pow(2, servoResolution) / 20000, 2000 * pow(2, servoResolution) / 20000);
    ledcWrite(servoChannel, dutyCycle);
}

int mide_distancia(){
  gatillo();
  long tiempo = pulseIn(echo, HIGH);
  int distancia = tiempo /58;
  delay(200);
  return distancia;
}
void gatillo(){
  digitalWrite(trigg,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigg,LOW);
}

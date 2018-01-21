//Node ID definition
#define MY_NODE_ID 1
//Enable message signing
#define MY_SIGNING_SOFT
#define MY_SIGNING_REQUEST_SIGNATURES
#define MY_SIGNING_SOFT_RANDOMSEED_PIN 8

//NRF24 module power and channel settings
#define MY_RF24_PA_LEVEL RF24_PA_MAX
#define MY_RF24_CHANNEL 83

//Node WIPS lockout
#define MY_NODE_LOCK_FEATURE
#define MY_NODE_UNLOCK_PIN 3

#include <Arduino.h>
#include <math.h>
#include <Secrets.h>
#include <MySensors.h>
#include <DHT.h>
#include <NewPing.h>

//Temperature sensor config
#define DHTPIN 7
#define DHTTYPE DHT22

//Heating control relay pin
#define RELAY_PIN 2

//Tank metering ultrasonic sensor config
#define TANK_TRIGGER_PIN 4
#define TANK_ECHO_PIN 6
#define TANK_MAX_DISTANCE 200
#define TANK_EMPTY_DISTANCE 170
#define TANK_FULL_DISTANCE 20

//MySensors Child sensor IDs
#define SENSOR_TEMP_ID 0
#define SENSOR_HUM_ID 1
#define SENSOR_CAPACITY_ID 2
#define SENSOR_HEATER_ID 3

bool heater_status = false;
float oil_percent = 0;
float ext_temp = 0;
float ext_hum = 0;


MyMessage temp_msg(SENSOR_TEMP_ID, V_TEMP);
MyMessage hum_msg(SENSOR_HUM_ID, V_HUM);
MyMessage oil_msg(SENSOR_CAPACITY_ID, V_VOLUME);
MyMessage heater_msg(SENSOR_HEATER_ID, V_STATUS);

DHT dht(DHTPIN, DHTTYPE);
NewPing tank(TANK_TRIGGER_PIN, TANK_ECHO_PIN, TANK_MAX_DISTANCE);



void receive(const MyMessage &message) {
        if ( ( message.sensor == SENSOR_HEATER_ID ) && ( message.type == V_STATUS ) && ( !message.isAck() ) ) {
              heater_status = message.getBool();
              if ( heater_status ) {
                      digitalWrite(RELAY_PIN, HIGH);
              } else {
                      digitalWrite(RELAY_PIN, LOW);
              }
              saveState(0, heater_status);
          }
}

float getTankPercent() {
        float tank_distance = 0.0;
        float percent = 0.0;
        float spent = 0.0;
        float remaining = 0.0;
        tank_distance = tank.ping_cm();
        spent = tank_distance - TANK_FULL_DISTANCE;
        remaining = TANK_EMPTY_DISTANCE - spent;
        percent = remaining / (TANK_EMPTY_DISTANCE - TANK_FULL_DISTANCE) * 100;
        return percent;
}

void sendChanges(float new_temp, float new_hum, float new_oil) {
        if ( new_temp != ext_temp ) {
                ext_temp = new_temp;
                temp_msg.set(new_temp, 2);
        }
        if ( new_hum != ext_hum ) {
                ext_hum = new_hum;
                hum_msg.set(new_hum, 2);
        }
        if ( new_oil != oil_percent ) {
                oil_percent = new_oil;
                oil_msg.set(oil_percent, 2);
        }
        send(temp_msg);
        send(hum_msg);
        send(oil_msg);

}


void presentation() {
        present(0, S_TEMP, "Temperatura exterior Caldera");
        present(1, S_HUM, "Humedad exterior Caldera");
        present(2, S_WATER, "Volumen tanque Gasoil");
        present(3, S_BINARY, "Salida calefaccion");
        sendSketchInfo("Caldera", "V1.0");
}
void setup() {

        // put your setup code here, to run once:
        pinMode(RELAY_PIN, OUTPUT);
        dht.begin();

        //Load relay saved state
        heater_status = loadState(0);
        if ( heater_status ) {
                digitalWrite(RELAY_PIN, HIGH);
        } else {
                digitalWrite(RELAY_PIN, LOW);
        }
}

void loop() {
        float tank_percent = 0.0;
        float new_temp = 0.0;
        float new_hum = 0.0;
        new_temp = dht.readTemperature();
        new_hum = dht.readHumidity();
        if ( isnan(new_temp) || isnan(new_hum) ) {
                return;
        }
        tank_percent = getTankPercent();
        sendChanges(ext_temp, ext_hum, tank_percent);

        sendHeartbeat();

}

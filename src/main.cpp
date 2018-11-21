//Node ID definition
#define MY_NODE_ID 1

//Enable debug mode
#define MY_DEBUG

//NRF24 module power and channel settings
#define MY_RADIO_NRF24
#define MY_RF24_PA_LEVEL RF24_PA_LOW
#define MY_RF24_CHANNEL 83

//Node WIPS lockout
//#define MY_NODE_LOCK_FEATURE
//#define MY_NODE_UNLOCK_PIN 3

#include <Arduino.h>
#include <math.h>
#include <Secrets.h>
#include <MySensors.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <NewPing.h>
#include <avr/wdt.h>

//Temperature sensor config
#define DHTPIN 7
#define DHTTYPE DHT22

//Sensor data send delay in ms
#define WAIT_TIME 300000
//Heating control relay pin
#define RELAY_PIN 2
#define RELAY_ON 1
#define RELAY_OFF 0

//Tank metering ultrasonic sensor config
#define TANK_TRIGGER_PIN 4
#define TANK_ECHO_PIN 6
#define TANK_MAX_DISTANCE 155
#define TANK_EMPTY_DISTANCE 148
#define TANK_FULL_DISTANCE 5
#define TANK_AREA 1.05

//MySensors Child sensor IDs
#define SENSOR_TEMP_ID 0
#define SENSOR_HUM_ID 1
#define SENSOR_CAPACITY_ID 2
#define SENSOR_HEATER_ID 3

bool heater_status = false;
float oil_capacity = 0.0;
float ext_temp = 0;
float ext_hum = 0;


MyMessage temp_msg(SENSOR_TEMP_ID, V_TEMP);
MyMessage hum_msg(SENSOR_HUM_ID, V_HUM);
MyMessage oil_msg(SENSOR_CAPACITY_ID, V_VOLUME);
MyMessage heater_msg(SENSOR_HEATER_ID, V_STATUS);

DHT dht(DHTPIN, DHTTYPE);
NewPing tank(TANK_TRIGGER_PIN, TANK_ECHO_PIN, TANK_MAX_DISTANCE);

float getTankCapacity();
void sendChanges(float new_temp, float new_hum, float new_oil);

void receive(const MyMessage &message) {
        if ( ( message.sensor == SENSOR_HEATER_ID ) && ( message.type == V_STATUS ) && ( !message.isAck() ) ) {
              heater_status = message.getBool();
              if ( heater_status ) {
                      digitalWrite(RELAY_PIN, HIGH);
              } else {
                      digitalWrite(RELAY_PIN, LOW);
              }
              wait(500);
              heater_msg.set(heater_status? RELAY_ON : RELAY_OFF);
              send(heater_msg, true);
          }
}

float getTankCapacity() {
        float tank_distance = 0.0;
        float spent = 0.0;
        float remaining_cm = 0.0;
        float capacity = 0.0;
        tank_distance = tank.ping_cm();
        remaining_cm = TANK_EMPTY_DISTANCE - (tank_distance - TANK_FULL_DISTANCE);
        capacity = TANK_AREA * (remaining_cm / 100);
        return capacity;
}

void sendChanges(float new_temp, float new_hum, float new_oil) {
        if ( !isnan(new_temp) && (new_temp != ext_temp) ) {
                ext_temp = new_temp;
                temp_msg.set(new_temp, 2);
        }
        if ( !isnan(new_hum) && (new_hum != ext_hum) ) {
                ext_hum = new_hum;
                hum_msg.set(new_hum, 2);
        }
        if ( new_oil != oil_capacity ) {
                oil_capacity = new_oil;
                oil_msg.set(oil_capacity, 2);
        }
        send(temp_msg, true);
        wait(500);
        send(hum_msg, true);
        wait(500);
        send(oil_msg, true);
        wait(500);
        send(heater_msg, true);
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
        wdt_enable(WDTO_8S);
}

void loop() {
        float new_capacity = 0.0;
        float new_temp = 0.0;
        float new_hum = 0.0;
        new_temp = dht.readTemperature();
        new_hum = dht.readHumidity();
        new_capacity = getTankCapacity();
        sendChanges(new_temp, new_hum, new_capacity);
        wait(WAIT_TIME);
      //  sendHeartbeat();

}

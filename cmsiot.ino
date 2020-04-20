//Include Zone
#include "DHT.h"
#include "sps30.h"
#include "Adafruit_CCS811.h"
#include <Wire.h>
#include <MAX44009.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SocketIoClient.h>

#define cms_iot_key "0edebf5c02b29"

/* SPS30 CONNECTION
  RED 5V
  Purple GND
  Blue no connect
  Green pin 17 RX2
  Yellow pin 16 TX2

  DHT
  VCC 5V
  DATA PIN 2

  CCS881
  VCC 3.3 V
  SCl -----SCL1
  SDA -----SDA1

*/
char host[] = "34.87.72.130"; // Ip Address 
int port = 3001; // Socket.IO Port Address 
/////////////////////////////////////
////// ESP32 Socket.IO Client //////
///////////////////////////////////

SocketIoClient webSocket;
WiFiClient client;


// DEFINE ZONE
#define SP30_COMMS SERIALPORT1
#define TX_PIN 13 //D1
#define RX_PIN 14 //D2
#define DEBUG 0

#define DHTPIN 17     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

//PIN DEFINE ZONE
int pir = 16; //D7
MAX44009 light;
Adafruit_CCS811 ccs;

void Errorloop(char *mess, uint8_t r);
void ErrtoMess(char *mess, uint8_t r);
int tempp;
float humi;
const char* ssid = "sevirach_2.4G";
const char* password =  "58993844";


SPS30 sps30;

void setup()
{
  Serial.begin(115200);
  delay(4000);   //Delay needed before calling the WiFi.begin

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");


  Serial.begin(115200);
  pinMode(pir, INPUT);
  Serial.println("CCS811 test");

  if (!ccs.begin()) {
    Serial.println("Failed to start sensor! Please check your wiring.");
    while (1);
  }

  // Wait for the sensor to be ready
  while (!ccs.available());
  Serial.println();
  Serial.println(F("Trying to connect"));

  // set driver debug level
  sps30.EnableDebugging(DEBUG);

  // set pins to use for softserial and Serial1 on ESP32
  if (TX_PIN != 0 && RX_PIN != 0) sps30.SetSerialPin(RX_PIN, TX_PIN);

  // Begin communication channel;
  if (sps30.begin(SP30_COMMS) == false) {
    Errorloop("could not initialize communication channel.", 0);
  }

  // check for SPS30 connection
  if (sps30.probe() == false) {
    Errorloop("could not probe / connect with SPS30.", 0);
  }
  else
    Serial.println(F("Detected SPS30."));

  // reset SPS30 connection
  if (sps30.reset() == false) {
    Errorloop("could not reset.", 0);
  }

  dht.begin();
  webSocket.begin(host, port);
}
void checkLEDState() {
  cabon();
  dustbuster10();
  dustbuster25();
  lighter();
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  /*===============================================================*/

  long state = digitalRead(pir);
  int state_server_pir = 0 ;
  if (state == HIGH) {

    Serial.println("Motion detected!");
    state_server_pir = 1;
  }
  else {

    Serial.println("Motion absent!");
    state_server_pir = 0;
  }
  /*===================================================================*/
  // CHECK POINT EIEI :D
  //int cabon = cabon();


//  Serial.print ("CARBON TEST : ");
//  Serial.println (cabon());
//  Serial.print ("HUMI TEST ");
//  Serial.print (h);
//  Serial.print("%");
//  Serial.println("");
//  Serial.print ("TEMP TEST ");
//  Serial.print (t);
//  Serial.print(" C");
//  Serial.println("");
//  Serial.print ("PM10: ");
//  Serial.print (dustbuster10());
//  Serial.println("");
//  Serial.print ("PM25: ");
//  Serial.print (dustbuster25());
//  Serial.println ("");
//  Serial.print ("LIGHT  : " );
//  Serial.println(lighter());
//  Serial.println("===============================");


  String str = "{\"key\":\"" + String(cms_iot_key) + "\",\"humid\":\"" + String(h) + "\",\"temp\":\"" + String(t) + "\",\"pm10\":\"" + String(dustbuster10()) + "\",\"pm25\":\"" + String(dustbuster25()) + "\",\"lux\":\"" + String(lighter()) + "\",\"carbon\":\"" + String(cabon()) + "\",\"pir\":\"" + String(state_server_pir) + "\"}";

  char msg[str.length() + 1] ;
  str.toCharArray(msg, str.length() + 1);
  Serial.println(msg);
  webSocket.emit("iot", msg);
}
void loop()
{
   webSocket.loop();
       checkLEDState();
   

}

float lighter() {
  float lux = light.get_lux();
  return (lux);
}

int cabon () {
  if (ccs.available()) {
    if (!ccs.readData()) {
      return (ccs.geteCO2());
    }
    else {
      Serial.println("ERROR!");
      while (1);
    }
  }

}

float dustbuster10() {

  uint8_t ret, error_cnt = 0;
  struct sps_values val;
  do {

    ret = sps30.GetValues(&val);
  } while (ret != ERR_OK);
  float duster = val.MassPM1;

  return (duster);


}
float dustbuster25() {

  uint8_t ret, error_cnt = 0;
  struct sps_values val;
  do {

    ret = sps30.GetValues(&val);
  } while (ret != ERR_OK);
  float duster = val.MassPM2;
  return (duster);


}
void Errorloop(char *mess, uint8_t r)
{
  if (r) ErrtoMess(mess, r);
  else Serial.println(mess);
  Serial.println(F("Program on hold"));
  for (;;) delay(100000);
}
void ErrtoMess(char *mess, uint8_t r)
{
  char buf[80];

  Serial.print(mess);

  sps30.GetErrDescription(r, buf, 80);
  Serial.println(buf);
}
/*int carbon () {
    if(ccs.available()){
    if(!ccs.readData()){
      Serial.print("CO2: ");
      Serial.println;
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }
  }
  }*/

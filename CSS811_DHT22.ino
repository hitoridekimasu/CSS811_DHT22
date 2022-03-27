#include <ESP8266WiFi.h>
#include <Wire.h>
#include <InfluxDbClient.h>
#include "Adafruit_CCS811.h"
#include "DHT.h" // Libreria para manejar sensor de temperatura

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321 <-----
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

const int DHTPin = 14;     // (Serigrafia D3)
double CO2;
double TVOC;
double TEMP;
double HR;


Adafruit_CCS811 ccs;
DHT dht(DHTPin, DHTTYPE);

// -------------- Inicialización / Parametros WIFI -------------------
#define WIFI_SSID "wohntextilien"
#define WIFI_PASSWORD "*"
// -------------------------------------------------------------------

// -------------- Sincronizacion NTP ---------------------------------
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
// Servidores NTP para sincronización.
// Para la sincronización mas rápida https://www.pool.ntp.org/zone/
#define NTP_SERVER1  "1.es.pool.ntp.org"
#define NTP_SERVER2  "1.europe.pool.ntp.org"
//--------------------------------------------------------------------

// -------------- Incializacion / Parametros INFLUX ------------------
#define INFLUXDB_URL "http://ataoyo-gcp.ddns.net:8086"
#define INFLUXDB_DB_NAME "temphum"
#define INFLUXDB_USER ""
#define INFLUXDB_PASS ""
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 1  // Numero de puntos que se escriben de una vez
#define WRITE_BUFFER_SIZE 50 //Número máximo de puntos en el buffer
                             
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
//--------------------------------------------------------------------

void config_WIFI(void)
{


 /************************************************************************/
 /* Inicialización WIFI*/
 /************************************************************************/

 Serial.println("-----------------------------------------------------");
 Serial.print("Inicializando WIFI");
 Serial.println("-----------------------------------------------------");

 WiFi.begin(WIFI_SSID, WIFI_PASSWORD);      // Conectar WIFI

 Serial.println("*****************************************************");
 Serial.print("WIFI Inicialización conectando a: ");
 Serial.print(WIFI_SSID); Serial.println(" ...");

  int i = 0;

  // Esperando la conexión **********************************************
 
  while (WiFi.status() != WL_CONNECTED) { 
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }


 Serial.println("Conexion establecida!");
 Serial.print(" Direccion IP:  ");
 Serial.println(WiFi.localIP());  // Dirección IP asignada al sensor
 Serial.println("*****************************************************");

}
void config_TIME(void)
 {
   /************************************************************************/
  /* Sincronizacion  NTP*/
  /************************************************************************/
  Serial.println("Sincronización NTP.");
  Serial.println("");
  timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
  Serial.println("*****************************************************");

 }

 void config_INFLUX(void)
 {
  /************************************************************************/
  /* Inicializacion parametros Influx*/
  /************************************************************************/
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASS);
  client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));
  Serial.println("Conexión InfluxDB realizada");
  Serial.println("*****************************************************");

 }

 void InFlux_Write(void)
{

  time_t tnow;

  Point pointDevice("ambientales");

  // Establecemos tiempo para registro actual ------------
  tnow=time(nullptr);
  pointDevice.setTime(tnow);
  // -----------------------------------------------------

  // Medidas conexion WIFI -------------------------------
  pointDevice.clearFields(); //Limpiamos registros para seguir añadiendo
  pointDevice.setTime(tnow);
  pointDevice.addField("CO2", CO2);
  pointDevice.addField("TVOC", TVOC);
  pointDevice.addField("TEMP", TEMP);
  pointDevice.addField("HR", HR);

  // -----------------------------------------------------
  client.writePoint(pointDevice); //Registramos en dB
}

void setup() {
  Serial.begin(115200);

  config_WIFI();     // Incialización WIFI (En Setup porque solo se hace una vez)
  config_TIME();     // Sincronizacion NTP
  config_INFLUX();   // Conexión con la base dedatos

  Serial.println("CCS811 test");

  dht.begin();

  if(!ccs.begin()){
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    while(1);
  }


  // Wait for the sensor to be ready
  while(!ccs.available());
}

void loop() {
  if(ccs.available()){
    if(!ccs.readData()){
      Serial.print("CO2: ");
      CO2 = ccs.geteCO2();
      Serial.print(CO2);
      TVOC = ccs.getTVOC();
      Serial.print("ppm \t TVOC: ");
      Serial.println(TVOC);
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }

  HR = dht.readHumidity();
  TEMP = dht.readTemperature();

  if (isnan(HR) || isnan(TEMP)) {
      Serial.println("Fallo de Sensor!");
      return;
  }


  Serial.print("Humedad: ");
  Serial.print(HR);
  Serial.print(" %\t");
  Serial.print("Temperatura: ");
  Serial.print(TEMP);
  Serial.println(" *C ");
  InFlux_Write();
  delay(5000);
}

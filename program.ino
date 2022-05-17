// Author: Kevin Luwemba Mugumya
// Fetching DHT22 sensor data (temperature and humidity) via MQTT using ESP 32
// The MQTT broker server: mqtt.jyings.com is built on top of EMQX, configured and self-hosted on a Linode server instance.   
// The broker server handles all the sensor traffic and the telemetry.
// Sensor data payload is published in JSON format i.e {"Humidity":49.00,"Temperature_C":31.70,"Temperature_F":89.06,"HeatIndex_C":33.60,"HeatIndex_F":92.48}

#include <DHT.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// wifi auth settings
#define WIFI_NAME "wifi-access-point-name"
#define WIFI_PASSWORD "wifi-password"

// Define DHT data pin and sensor type
#define DHTPIN 4
#define DHTTYPE DHT22

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// MQTT Topics
#define topic "sensors/temp-humidity-sensor/tts5"


// variables for creating and connecting to the MQTT research broker (server)
const char* mqttServer = "your-broker.com";
const int mqttPort = 8883;
const char* mqttUser = "your-broker-auth-name";
const char* mqttPassword = "your-broker-auth-password";

// server root CA
// SHA1 fingerprint is broken now! Don't use it
const char* mqtt_root_ca = \
      "-----BEGIN CERTIFICATE-----\n" \
      "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
      "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
      "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
      "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
      "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
      "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
      "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
      "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
      "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
      "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
      "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
      "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
      "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
      "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
      "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
      "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFfgGnY+EgPbk6ZGQ\n" \
      "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
      "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVdff6Lv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
      "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
      "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
      "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+ssSM1N95R1NbdWhscdCb+ZAJzVc\n" \
      "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
      "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
      "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
      "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
      "-----END CERTIFICATE-----\n";

WiFiClientSecure net;

PubSubClient client(net);

unsigned long lastSend;

void setup() 
{
  // initialize serial for debugging and wait for port to open:
  Serial.begin(9600);       
  delay(10);
  
  InitWiFi();
  
  reconnect();
  Serial.print('\n');
  
  Serial.println("Reading and sending sensor data");
  delay(1000);
  Serial.println("RH\t\tTemp (C)\tTemp (F)\tHeat Index (C)\tHeat Index (F)");
  dht.begin();
  lastSend = 0; 
}
  
void loop() 
{
  if ( !client.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > 30000 ) { // Update and send only after 30 seconds
    getAndSendTemperatureAndHumidityData();
    lastSend = millis();
  }
  
  client.loop();
}

//-----------------Reading Sensor data--------------------
void getAndSendTemperatureAndHumidityData()
{
  
  // Reading temperature or humidity takes about 250 milliseconds!
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature_C = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float temperature_F = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature_C)) {
    Serial.println("Failed to read from DHT sensor");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float heatIndex_F = dht.computeHeatIndex(temperature_F, humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  float heatIndex_C = dht.computeHeatIndex(temperature_C, humidity, false);
  
  Serial.print(humidity); Serial.print(" %\t\t");
  Serial.print(temperature_C); Serial.print(" *C\t");
  Serial.print(temperature_F); Serial.print(" *F\t");
  Serial.print(heatIndex_C); Serial.print(" *C\t");
  Serial.print(heatIndex_F); Serial.println(" *F");

  String hum = String(humidity);
  String temp_C = String(temperature_C);
  String temp_F = String(temperature_F);
  String hIndex_C = String(heatIndex_C);
  String hIndex_F = String(heatIndex_F);

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"Humidity\":"; 
  payload += hum; 
  payload += ",";
  payload += "\"Temperature_C\":"; 
  payload += temp_C;
  payload += ",";
  payload += "\"Temperature_F\":"; 
  payload += temp_F;
  payload += ",";
  payload += "\"HeatIndex_C\":"; 
  payload += hIndex_C;
  payload += ",";
  payload += "\"HeatIndex_F\":"; 
  payload += hIndex_F;
  payload += "}";

  // Send payload
  char attributes[payload.length() + 1];
  payload.toCharArray( attributes, (payload.length() + 1));
  client.publish( topic, attributes );
//  Serial.println( attributes );
}

//-----------------Function connecting ESP module to Wifi-----------------
void InitWiFi()
{
  // attempt to connect to wifi
  Serial.print("Connecting to SSID:  ");
  Serial.print(WIFI_NAME); Serial.println(" ...");
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);

  // visual feedback signal while waiting for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // once connected
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}


//-------------------Connection to secure research MQTT broker-------------------
void reconnect() {
  // first set the server RootCA cert
  net.setCACert(mqtt_root_ca);

  // Set the MQTT server details for client to connect to
  client.setServer(mqttServer, mqttPort);

  // Callback function to execute when an MQTT message is received by subscribed client.
  //client.setCallback(callback);
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print('\n');
    Serial.println("Connecting to MQTT broker node ...");
    // Attempt to connect (clientId, username, password)
    
    if ( client.connect("ESP32-DHT22-Client", mqttUser, mqttPassword) ) {
      Serial.println( "[CONNECTED]" );
     } else {
      Serial.print( "Failed with state code: " );
      Serial.print(client.state());
      Serial.println( " : retrying in 5 seconds" );
      
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}
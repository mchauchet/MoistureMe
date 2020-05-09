#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

Preferences preferences;

// Create AsyncWebServer object on port 80
AsyncWebServer webServer(80);

const uint8_t WIFI_TRESH = 60;
const String WIFI_SSID_DEFAULT = "MoistureMe";
const String WIFI_PASS_DEFAULT = "MoistureMe";
const String WIFI_HOSTNAME_DEFAULT = "moistureme-";

const uint8_t MAX_POTS = 8;
uint8_t numPots = 0;
uint8_t potSensor[MAX_POTS];
uint8_t potRelay[MAX_POTS];

uint8_t pumpRelay = 0;
uint8_t pumpSensorTrig = 0;
uint8_t pumpSensorEcho = 0;

const int thrsh_low = 2400;
const int thrsh_high = 3000;

void connectWifi(String wifiMode, String wifiSsid, String wifiPass) {
  Serial.println("WiFi Mode: "+wifiMode);
  Serial.println("WiFi SSID: "+wifiSsid);
  Serial.println("WiFi Password: "+wifiPass);
  
  if(wifiMode.equals("STA")) {
      WiFi.disconnect(true);
      WiFi.enableSTA(true);
      WiFi.mode(WIFI_STA);
      
      Serial.print("Connecting to ");
      Serial.println(wifiSsid);
     
      short wifiTimeout = 0;
      while (WiFi.status() != WL_CONNECTED && wifiTimeout < WIFI_TRESH) {
          WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
          delay(1000);
          Serial.print(".");
          wifiTimeout++;
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println();
      }
    } else if(wifiMode.equals("AP") || WiFi.status() != WL_CONNECTED) {
      Serial.println("Configuring access point...");
      WiFi.softAP(wifiSsid.c_str(), wifiPass.c_str());
    }
}

String relayState(int pinRelay) {
  int state = digitalRead(pinRelay);
  if(state == HIGH)
    return "OFF";
  else
    return "ON";
}


String getState() {
  String ret = "";

  ret+="{";
  //wifi state
  ret+="\"wifi\":{";
  // Mode
  //ret+=("mode: " + WiFi.mode + ",");
   // MODE
  // Connected
  // SSID
  // RSSI
  ret+="},";
  //All flower pot sensor and relay states
  ret+="\"pots\":{\"pot\":[";
  for(int i = 0; i< numPots; i++) {
    ret+="{";
    ret+=("\"id\": "+String(i)+",");
    ret+=("\"sensor\": "+String(analogRead(potSensor[i]))+",");
    ret+=("\"relay\": \""+String(relayState(potRelay[i]))+"\"");
    ret+="},";
  }
  ret+="]},";
  //pump sensor and relay state
  ret+="\"pump\":{";
    ret+=("\"relay\": \""+String(relayState(pumpRelay))+"\"");
  ret+="}}";
  return ret;
}


void setup()
{    
    // Setup serial port for debugging purpose
    Serial.begin(115200);

    // Initialize SPIFFS
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    /**
     * Setup WiFi connection
     *  - Check if there are WiFi preferences configured
     *  - If: Try connecting to configured WiFi
     *     - If connection fails, fail back and setup AP with default preferences
     *  - If not: Setup AP with default preferences
     */
    // Load preferences from memory
    Serial.println("Loading Wifi preferences from memory ...");
    
    preferences.begin("WiFi", false);
      String wifiHostname = preferences.getString("Hostname", "");
      String wifiMode = preferences.getString("Mode", "");
      String wifiSsid = preferences.getString("Ssid", "");
      String wifiPass = preferences.getString("Password", "");
    preferences.end();

    if(wifiHostname.equals("")) {
      Serial.println("No valid hostname found, using default hostname");
      Serial.println();
      wifiHostname = WiFi.macAddress();
      wifiHostname.replace(":","");
      wifiHostname = WIFI_HOSTNAME_DEFAULT + wifiHostname;
    }
    Serial.print("Set Hostname: "); Serial.println(wifiHostname);
    WiFi.setHostname(wifiHostname.c_str());
    Serial.println();
    
    if (wifiMode.equals("")
        || wifiSsid.equals("") 
        || wifiPass.equals("")) {
      Serial.println("No valid WiFi preferences found, starting in AP mode with default settings");
      wifiMode = "AP"; 
      wifiSsid = WIFI_SSID_DEFAULT;
      wifiPass = WIFI_PASS_DEFAULT;
    } else {
        Serial.println("Valid preferences found");
    }

    wifiMode = "STA"; 
    wifiSsid = "Pololan";
    wifiPass = "pojeha2359";
    
    connectWifi(wifiMode, wifiSsid, wifiPass);
    Serial.println();
    delay(1000);


    /**
     * Load flower pot configuration
     *  - Load number of flower pots
     *  - Read GPIO settings
     */
    // Load preferences from memory
    Serial.println("Loading flower pot preferences from memory ...");
    preferences.begin("FlowerPots", false);
      numPots = preferences.getInt("numPots");
      preferences.getBytes("PotSensor", potSensor, MAX_POTS);
      preferences.getBytes("PotRelay", potRelay, MAX_POTS);
      pumpRelay = preferences.getInt("PumpRelay");
      pumpSensorTrig = preferences.getInt("PumpSensorTrig");
      pumpSensorEcho = preferences.getInt("PumpSensorEcho");
    preferences.end();

    // TODO: Remove when preferences can be set from web page
    numPots = 2;
    potSensor[0] = 36;
    potSensor[1] = 39;
    potSensor[2] = 34;
    potRelay[0] = 19;
    potRelay[1] = 18;
    potRelay[2] = 17;
    pumpRelay = 16;

    for(int i = 0; i < numPots; i++) {
      pinMode(potRelay[i], OUTPUT);
      digitalWrite(potRelay[i], HIGH);
    }
    pinMode(pumpRelay, OUTPUT);
    digitalWrite(pumpRelay, HIGH);

    Serial.println("*===================================");
    Serial.print("* Total pots configured: ");
    Serial.println(numPots);
    Serial.println("*-----------------------------------");
    for(int i = 0; i < numPots; i++) {
      Serial.print("* Pot "); Serial.println(i);
      Serial.print("*   Sensor Pin: "); Serial.println(potSensor[i]);
      Serial.print("*   Relay Pin: "); Serial.println(potRelay[i]);
    }
    Serial.println("*-----------------------------------");
    Serial.println("* Pump ");
    Serial.print("*   Relay Pin: "); Serial.println(pumpRelay);
    Serial.print("*   Sensor Trig: "); Serial.println(pumpSensorTrig);
    Serial.print("*   Sensor Echo: "); Serial.println(pumpSensorEcho);
    Serial.println("*===================================");
    Serial.println();
    
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html");
    });
    webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/settings.html");
    });
    //webServer.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request){
    //  request->send(SPIFFS, "/calibrate.html");
    //});
    webServer.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", getState().c_str());
    });
    // Start server
    webServer.begin();
    
    Serial.println("Setup done");
}

void loop()
{
  for(int i = 0; i < numPots; i++) {
    //Serial.print("Sensor "); Serial.print(i);
    //Serial.print(" GPIO"); Serial.print(potSensor[i]); Serial.print(": ");
    //Serial.println(analogRead(potSensor[i]));
    if(analogRead(potSensor[i]) > thrsh_high) {
      Serial.print("Start watering pot "); Serial.println(i+1);
      digitalWrite(potRelay[i], LOW);
      delay(200);
      digitalWrite(pumpRelay, LOW);
      
      while(analogRead(potSensor[i]) > thrsh_low) { 
        //Serial.println(analogRead(potSensor[i]));
        delay(1000);
      }
      Serial.print("End watering pot "); Serial.println(i+1);
      digitalWrite(pumpRelay, HIGH);
      delay(200);
      digitalWrite(potRelay[i], HIGH);
    }
    //delay(1000);
  }
}

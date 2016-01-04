#include "DHT.h"
#include <SoftwareSerial.h>
#define SSID "change_me"
#define PASS "change_me"
#define GATEWAY_ADDRESS "icent.nut.cc" //iotGateway address /IP address  http://icent.nut.cc:3000/
#define GATEWAY_PORT 3000
#define chipId "101"
#define initReqData "POST /dhts HTTP/1.1\r\nHost:"
#define DHTPIN 4 // the uno digital pin I'm connected to
#define DHTTYPE DHT22 // DHT 22  

unsigned long previousMillis = 0; // will store last time DHT was acccessed 
const long readInterval = 600000;   // read temperature,humidity and send readings to iotgateway every 10 minutes 

SoftwareSerial wifiPort(2, 3); //Connect ESP Tx to Uno Pin 2, ESP Rx to Uno Pin 3
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(9600);
    wifiPort.begin(9600); //  ESP baudrate
   
    //initalize the wifi module 
    prepareWifiModule();

    //Connect ESP module to home/office wifi 
    connectToWiFi();
}

void prepareWifiModule() {
    sendCommand("AT\r\n");
    sendCommand("AT+RST\r\n");
    sendCommand("AT+CWMODE=3\r\n");
    sendCommand("AT+CIPMUX=1\r\n");
    delay(2000);
}

void connectToWiFi() {
    // Serial.println("Connecting to the internet...");
    String cmd = "AT+CWJAP=\"";
    cmd += SSID;
    cmd += "\",\"";
    cmd += PASS;
    cmd += "\"";
    cmd += "\r\n";
    sendCommand(cmd);
    delay(2000);
}

void loop() {
    unsigned long currentMillis = millis();
    
    if(currentMillis - previousMillis > readInterval) {
    //save the next readInterval.
    previousMillis = currentMillis;
    
    // Read temperature as Celsius (the default)
    float hum = dht.readHumidity();
    
    // Read temperature as Celsius
    float tem = dht.readTemperature();
    
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float temF = dht.readTemperature(true);
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(tem)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    
    float hic = dht.computeHeatIndex(tem, hum, false);
    
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(tem);
    Serial.print(" *C\t ");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print('\n');
    
    sendTelemetry(tem, hum, hic);
    
    } 
}

void sendTelemetry(float t, float h, float hi) {
    char buff[10];
    String temperature_string;
    String humidity_string;
    String heatIndex_string;
    
    //convert floats to strings
    temperature_string = dtostrf(t, 2, 2, buff);
    humidity_string = dtostrf(h, 2, 2, buff);
    heatIndex_string = dtostrf(hi, 2, 2, buff);

   //Generate telemetry in json format 
    String telemetry = "{";
    telemetry += "\"chipid\":\"";
    telemetry += chipId;
    telemetry +=  "\",\"location\":\"GBRG6\",\"description\":\"B4Z1\",\"temperature\":";
    telemetry += temperature_string;
    telemetry += "\",\"humidity\":\"";
    telemetry += humidity_string;
    telemetry += "\",\"heat_index\":\"";
    telemetry += heatIndex_string;
    telemetry += "\"}";

    // Or parameterize telemetery in post request..
    //Note that the dht[...] is a ruby on rails thing.
    
//    String telemetry = "dht[chipid]=";
//    telemetry += chipId;
//    telemetry += "&dht[location]=GBRG6&dht[description]=B4Z1&dht[temperature]=";
//    telemetry += temperature_string;
//    telemetry += "&dht[humidity]=";
//    telemetry += humidity_string;
//    telemetry += "&dht[heat_index]=";
//    telemetry += heatIndex_string;

    telemetry.trim(); //remove trailing whitespaces.. 

    delay(1000); //let the buffer load 

    String requestHeader = GATEWAY_ADDRESS;
    requestHeader += "\r\n";
  //requestHeader += "Content-Type:application/x-www-form-urlencoded\r\nContent-Length:";  //if using post forms instead of JSON. 
    requestHeader += "Content-Type:application/json\r\nContent-Length:";
    requestHeader += telemetry.length();
    requestHeader += " \r\n\r\n";

    delay(1000); //let the buffer load 

    String http_request = initReqData;
    http_request += requestHeader;
    http_request += telemetry;
    http_request += "\r\n\r\n";

    delay(1000);
   
    //connect to gateway
    String cipStart = "AT+CIPSTART=4,\"TCP\",\"";
    cipStart += GATEWAY_ADDRESS;
    cipStart += "\",";
    cipStart += GATEWAY_PORT;
    cipStart += "\r\n";
    sendCommand(cipStart);
    
    delay(1000);
    
    //Serial.println(wifiPort.readString());
    if (Serial.find("ERROR") || Serial.find("Error")  || Serial.find("Unlink") || Serial.find("link is not")) {
        Serial.println("Could not connect to IOTgateway, retrying module initialization...");
        prepareWifiModule();
    }

    String cipSend = "AT+CIPSEND=4";
    cipSend += ",";
    cipSend += http_request.length();
    cipSend += "\r\n";
    
    sendCommand(cipSend);
    delay(1000);
    
    sendCommand(http_request);
    delay(3000);

    //close the TCP connection 
    sendCommand("AT+CIPCLOSE=4\r\n");
}

String sendCommand(String command) {
    String response = "";
    const int timeout = 1000;
    boolean debug = true;

    wifiPort.print(command); // send the read character to the esp8266

    long int time = millis();

    while ((time + timeout) > millis()) {
        while (wifiPort.available()) {
            // The esp has data so display its output to the serial window 
            char c = wifiPort.read(); // read the next character.
            response += c;
        }
    }

    if (debug) {
        Serial.print(response);
    }

    return response;
}

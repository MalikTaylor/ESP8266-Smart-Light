//#include <WiFi.h>
//#include <WiFiClient.h>
//#include <WiFiServer.h>
//#include <WiFiUdp.h>

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
//#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
//#include<FirbaseArduino.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "BT-Guest"
#define WIFI_PASSWORD "HouLovesYou!"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDK2SUCuixfZ82hJJpKKoQKRVlsxwJZWU8"

// Insert Authorized Username and Corresponding Password
#define USER_EMAIL "malik.omar.taylor@gmail.com"
#define USER_PASSWORD "Smores#1"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://esp-webapp-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

String userID;

// Variables to save database paths
String listenerPath = "board1/outputs/digital";
String sensor_db_path = "board1/inputs";

// Declare outputs
const int output1 = 5; //GPIO D1
const int reed_sensor = 4; //GPIO D2
int currentState = 0;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 4000;

//so far a timer delat of 4000 is perfect

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Callback function that runs on database changes
void streamCallback(FirebaseStream data){
  Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data); //see addons/RTDBHelper.h
  Serial.println();

  // Get the path that triggered the function
  String streamPath = String(data.dataPath());
  Serial.println(streamPath);

  // if the data returned is an integer, there was a change on the GPIO state on the following path /{gpio_number}
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_integer){
    String gpio = streamPath.substring(1);
    int state = data.intData();
    Serial.print("GPIO: ");
    Serial.println(gpio);
    Serial.print("STATE: ");
    Serial.println(state);
    digitalWrite(gpio.toInt(), state);
  }

  /* When it first runs, it is triggered on the root (/) path and returns a JSON with all keys
  and values of that path. So, we can get all values from the database and updated the GPIO states*/
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json){
    FirebaseJson json = data.to<FirebaseJson>();

    // To iterate all values in Json object
    size_t count = json.iteratorBegin();
    Serial.println("\n---------");
    for (size_t i = 0; i < count; i++){
        FirebaseJson::IteratorValue value = json.valueAt(i);
        int gpio = value.key.toInt();
        int state = value.value.toInt();
        Serial.print("STATE: ");
        Serial.println(state);
        Serial.print("GPIO:");
        Serial.println(gpio);
        digitalWrite(gpio, state);
        Serial.printf("Name: %s, Value: %s, Type: %s\n", value.key.c_str(), value.value.c_str(), value.type == FirebaseJson::JSON_OBJECT ? "object" : "array");
    }
    Serial.println();
    json.iteratorEnd(); // required for free the used memory in iteration (node data collection)
  }
  
  //This is the size of stream payload received (current and max value)
  //Max payload size is the payload size under the stream path since the stream connected
  //and read once and will not update until stream reconnection takes place.
  //This max value will be zero as no payload received in case of ESP8266 which
  //BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
}

void streamTimeoutCallback(bool timeout){
  if (timeout)
    Serial.println("stream timeout, resuming...\n");
  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void sendState(String path, int state){
  if(Firebase.RTDB.setInt(&stream, path.c_str(), state)){
    Serial.print("Writing state: ");
    Serial.print (state);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + stream.dataPath());
    Serial.println("TYPE: " + stream.dataType());
  }else{
    Serial.println("FAILED");
    Serial.println("REASON: " + stream.errorReason());
  }
}

void setup(){
  Serial.begin(115200);
  initWiFi();

  // Initialize Outputs
  pinMode(output1, OUTPUT);
  pinMode(reed_sensor, INPUT);
  
  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  Serial.println("Getting user ID");

  while((auth.token.uid) == ""){
    Serial.print(".");
    delay(1000);
  }

  userID = auth.token.uid.c_str();
  Serial.print("User ID: ");
  Serial.print(userID);

  // Update database path
  String databasePath = "/UsersData/" + userID;

  // Streaming (whenever data changes on a path)
  // Begin stream on a database path --> board1/outputs/digital
  if (!Firebase.RTDB.beginStream(&stream, listenerPath.c_str()))
    Serial.printf("stream begin error, %s\n\n", stream.errorReason().c_str());

  // Assign a calback function to run when it detects changes on the database
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  delay(2000);
}

void loop(){
  if (Firebase.isTokenExpired()){
//    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    currentState = digitalRead(reed_sensor);
    sendState(sensor_db_path, currentState);
  }
}


/* Personal Note about the project:
 *  The program works succesfully on basic networks (WPA), however the program cannot connect to wifi networks of time WAP2-Ent which often
 *  require a user name and a password. Some of these networks also require access to a login portal after connecting, and thus far I have not 
 *  figured out a way to connect the ESP8266 to these more intricate networks
 */

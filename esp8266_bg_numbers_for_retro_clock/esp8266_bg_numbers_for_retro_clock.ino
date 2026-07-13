#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>  // <-- Add this line
#include <ESP8266HTTPClient.h> // Use #include <ESP8266HTTPClient.h> if you are using an ESP8266

HTTPClient http;

const uint8_t tpicDigitMap[11] = {
    0b01110111, // 0
    0b00100100, // 1
    0b00111011, // 2
    0b00111110, // 3
    0b01101100, // 4
    0b01011110, // 5
    0b01011111, // 6
    0b00110100, // 7
    0b01111111, // 8
    0b01111110,  // 9
    0b00000000  // 10 = blank

};

// On ESP8266 NodeMCU, We care about GPIO, not what's labelled on the board (ie. D4)
#define DATA_PIN    13
#define CLOCK_PIN   14
#define LATCH_PIN   15

//--------------------------------
const char* ssid     = "UB1";
const char* password = "xxxxxxxxxx";              // CHANGE
//--------------------------------
char          buf[128];
unsigned int  bg;
unsigned int  direction;
char          msg[100]= {0};
unsigned long runTime;

// The URL of your local server script
const char* serverUrl = "https://www.dextender.com/cgi-bin/getLatestBG.cgi?";  // CHANGE
  
// Variable to store the server's response
String serverResponse = ""; 

//=======================================================================
// Prototypes
//=======================================================================
void makeWebCall();
void lightEmUp(int reading);



//-------------------------------------------------------------------------------------------------------  
  void setup() {
    Serial.begin(115200);
    delay(1000);
      
    // --- Connect to Wi-Fi ---
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("Wi-Fi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // --- Make the Web Call Once ---
    pinMode(DATA_PIN,  OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);

    digitalWrite(DATA_PIN,  LOW);
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(LATCH_PIN, LOW);  

    // --- Make the Web Call Once ---
    makeWebCall(); 
    
    testWrapper(1);
}

//-------------------------------------------------------------------------------------------------------
void loop()
{
   
    // Keeping loop empty for this example, 
    // but you can use the 'serverResponse' variable here!
    
    int httpResponseCode = http.GET(); 
    if (httpResponseCode == HTTP_CODE_OK) {
        serverResponse = http.getString();

        if (serverResponse.length() > 0) {

            serverResponse.toCharArray(buf, sizeof(buf)); 
            if (sscanf(buf, "%d|%d|%lu|%99s", &bg, &direction, &runTime, msg) == 4) {            
    
                if(strcmp(msg,"null") != 0) {  // message should always be null if everything is running smoothly
                    // Serial.print("Errror received from server");
                    testWrapper(2);
                    runTime=300;
                }
                else {
                    // Serial.print("BG: "); Serial.println(bg);   
                    // Serial.print("Direction:"); Serial.println(direction);

                    if (runTime > 300) runTime = 300;
                    // Serial.print("runTime:"); Serial.println(runTime);

                    // Light them up
                    if ((bg >  40) && (bg < 400)) lightEmUp(bg);   
                }    
            }
            else {
                testWrapper(2);
                runTime=300;
            }
        }    
    }      
    delay(runTime*1000);
}


//-------------------------------------------------------------------------------------------------------
// --- Function to Handle the HTTP GET Request ---
//-------------------------------------------------------------------------------------------------------
void makeWebCall() {
    if (WiFi.status() == WL_CONNECTED) {
    
      // 2. Use WiFiClientSecure instead of WiFiClient
      WiFiClientSecure client; 
    
      // 3. CRITICAL: Tell the client to skip SSL certificate validation.
      // Without this, the ESP8266 will reject the connection because it doesn't 
      // have the server's root certificate installed in its limited memory.
      client.setInsecure(); 

      // HTTPClient http;

      Serial.print("Connecting securely to server: ");
      Serial.println(serverUrl);
    
      // Initialize using the secure client
      http.begin(client, serverUrl); 
    
      int httpResponseCode = http.GET(); 
    
      if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
    
          serverResponse = http.getString();
      
          Serial.println("--- Data Stored ---");
          Serial.println(serverResponse); 
          Serial.println("-------------------");
      } 
      else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
      }
    
      http.end(); 
    } 
    else {
        Serial.println("WiFi Disconnected");
    }
}


//-------------------------------------------------------------------------------------------------------
//  Light up the Digits
//-------------------------------------------------------------------------------------------------------

void lightEmUp(int reading)
{
    int number = constrain(reading, 0, 999);

    byte hundreds = number / 100;
    byte tens     = (number / 10) % 10;
    byte ones     = number % 10;

    if (hundreds==0) hundreds=10;

    displayDigits(hundreds, tens, ones);
}


void displayDigits(byte left, byte middle, byte right)
{
    digitalWrite(LATCH_PIN, LOW);

    // Rightmost digit shifted first
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, tpicDigitMap[right]);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, tpicDigitMap[middle]);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, tpicDigitMap[left]);

    digitalWrite(LATCH_PIN, HIGH);
}

void testWrapper(unsigned int looper) {
   for (int x=0; x< looper; x++) {
       for (int i=0;i<8;i++)  {
           testPattern(1 << i);
           delay(1500);
       }
   }   
}


void testPattern(byte value) {
    digitalWrite(LATCH_PIN, LOW);

    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, value);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, value);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, value);

    digitalWrite(LATCH_PIN, HIGH);
}

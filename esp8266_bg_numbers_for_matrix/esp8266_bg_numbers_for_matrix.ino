  #include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>  // <-- Add this line
#include <ESP8266HTTPClient.h> // Use #include <ESP8266HTTPClient.h> if you are using an ESP8266

HTTPClient http;


// On ESP8266 NodeMCU, GPIO 2 maps to the physical pin labeled "D4"
#define PIN     5
#define WIDTH  32
#define HEIGHT  8

//--------------------------------
const char* ssid     = "UB1";
const char* password = "2324070289";
//--------------------------------
char buf[128];
unsigned int  bg;
unsigned int  direction;
unsigned long runTime;
char          msg[100]= {0};
  
// The URL of your local server script
const char* serverUrl = "https://www.dextender.com/cgi-bin/getLatestBG.cgi?code=392455&uid=100002"; 
  
// Variable to store the server's response
String serverResponse = ""; 

//=======================================================================
// Initialize matrix with zigzag row layout
//=======================================================================
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT, PIN,
NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG, // Changed ROWS to COLUMNS
NEO_GRB         + NEO_KHZ800);

String scrollText = "Hi Marnie !";
int16_t x = matrix.width();
int16_t y = 0;

// Variables to dynamically calculate text width

int16_t TX1, TY1; 
uint16_t textWidth, textHeight;
uint16_t textColor;


//=======================================================================
// Prototypes
//=======================================================================
void makeWebCall();
void lightEmUp(int reading);
void drawUpArrow(int16_t x, uint16_t color);
void drawDownArrow(int16_t x, uint16_t color);
void draw45UpArrow(int16_t x, uint16_t color);
void draw45DownArrow(int16_t x, uint16_t color);
void drawNoArrow(int16_t x);


//-------------------------------------------------------------------------------------------------------  
void setup() {
    
    Serial.begin(115200);
    
    matrix.begin();
    // Pre-calculate the exact pixel width of your text
    matrix.getTextBounds(scrollText, 0, 0, &TX1, &TX1, &textWidth, &textHeight);
    matrix.setTextWrap(false);
    matrix.fillScreen(0);
    matrix.show();
    
    
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
    makeWebCall();     
}

void loop() {

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
                    lightEmUp(0,0);
                    runTime=300;
                }
                else {
                    // Serial.print("BG: "); Serial.println(bg);   
                    // Serial.print("Direction:"); Serial.println(direction);

                    if (runTime > 300) runTime = 300;
                    // Serial.print("runTime:"); Serial.println(runTime);

                    // Light them up
                    if ((bg >  40) && (bg < 400)) lightEmUp(bg, direction);   
                }    
            }
            else {
                lightEmUp(0,0);
                runTime=300;
            }
        }    
    }    
  
    delay(runTime*1000);
}


void lightEmUp(int reading, int direction) {
  
    String strReading=String(reading);
    unsigned int red=0;
    unsigned int green=0;
    unsigned int blue=0;
    
    matrix.fillScreen(0);                    // Clear the screen
    if(strReading.length() == 2)  matrix.setCursor(10, 0); 
    else                          matrix.setCursor(7,0);

    
    if ((reading <= 80) || (reading >= 160)) {                 // outside of range
        if(reading <= 80) {                                    // less than 80
            if (reading == 0 ) {
                red=120; green=120; blue=0;
            }
            else {
                if (reading <= 70) {                               // less equal to 70
                    if (reading <= 60) {                           // danger ! all red.
                        red=180;   green=0;  blue=0;
                    }
                    else {                                         // between 60 and 70, start yellow to red                        
                        red=120; green= 120-((70-reading)*12); blue=0;
                    }
                }
                else {                                            // between 70 and 80, green to yellow
                    red=(80-reading)*12; green=120; blue=0;
                }
            }   
        }
        else {                                                // greater or equal to 160
             if (reading >= 180) {                            // purple
                red=160; green=0; blue=160;    
             }
             else {
                if (reading <= 170 ) {                        // Going to green to blue
                    red=0; green=100; blue=100;                    
                }
                else {                                        // Going blue to purple
                    red=60; green=0; blue=120;
                }
             }
        }
    }   
    else {
      red=0; green=120; blue=0;
    }

    Serial.print("Colors - RED: "); 
    Serial.print(red);
    Serial.print(" GREEN: ");
    Serial.print(green);
    Serial.print(" BLUE: ");
    Serial.println(blue);
        
    textColor = matrix.Color(red, green, blue);
    matrix.setTextColor(textColor);
    matrix.setBrightness(10); // Keep it low to avoid using power    
    switch(direction) {
      case 45: draw45UpArrow(26, textColor);
               break;
      case 90: 
      case 91:
               drawUpArrow(26, textColor);
               break;
      case 270: 
      case 271: 
               drawDownArrow(26, textColor);
               break;
      case 315:
               draw45DownArrow(26, textColor);
               break;  
       default:
               drawFlat(26);
    }
    if (reading > 0 ) matrix.print(String(reading));
    matrix.show();
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

void drawUpArrow(int16_t x, uint16_t color) {
  matrix.drawLine(x+3,  1, x+3, 7, color); // Stem
  matrix.drawPixel(x+2, 2, color);        // Left tip
  matrix.drawPixel(x+1, 3, color);        // Left tip

  matrix.drawPixel(x+4, 2, color);        // Right tip
  matrix.drawPixel(x+5, 3, color);        // Right tip  
}

void drawDownArrow(int16_t x, uint16_t color) {
  // Center stem is at x + 2
  matrix.drawLine(x+3,  1, x+3, 7, color); // Stem
  
  matrix.drawPixel(x+2, 6, color);          // Left arrow tip
  matrix.drawPixel(x+1, 5, color);          // Left arrow tip  
  matrix.drawPixel(x+4, 6, color);          // Right arrow tip  
  matrix.drawPixel(x+6, 5, color);          // Right outer tip
}

void draw45UpArrow(int16_t x, uint16_t color) {
  // Diagonal stem from bottom-left to top-right
  matrix.drawLine(x, 6, x + 5, 1, color); 
  
  // Arrowhead tips branching off the top-right corner (x+5, 1)
  matrix.drawLine(x + 2, 1, x + 4, 1, color); // Horizontal barb
  matrix.drawLine(x + 5, 2, x + 5, 4, color); // Vertical barb
}

void draw45DownArrow(int16_t x, uint16_t color) {
  // Diagonal stem from top-left to bottom-right
  matrix.drawLine(x, 1, x + 5, 6, color); 
  
  // Arrowhead tips branching off the bottom-right corner (x+5, 6)
  matrix.drawLine(x + 2, 6, x + 4, 6, color); // Horizontal barb
  matrix.drawLine(x + 5, 3, x + 5, 5, color); // Vertical barb
}

void drawFlat(int16_t x) {
  matrix.drawLine(x,  3, x+5, 3, matrix.Color(120, 120, 120));   
}

void drawError() {
  matrix.drawLine(9,  3, 10,  3, matrix.Color(120, 120, 120));   
  matrix.drawLine(12, 3, 13,  3, matrix.Color(120, 120, 120));   
  matrix.drawLine(15, 3, 16,  3, matrix.Color(120, 120, 120));   
  matrix.drawLine(18, 3, 19,  3, matrix.Color(120, 120, 120));   
  matrix.drawLine(21, 3, 22,  3, matrix.Color(120, 120, 120));   
}

/*** PROJECT SETUP SECTION ***/
#define SMSRec "6947977104" // IMPORTANT: SET THE SMS RECIPIENT NUMBER HERE
//#define SIMPin ""           // IMPORTANT: IF THE SIM REQUIRES A PIN, ENTER HERE ELSE ""

/** COSMOTE **/
//#define APN    "internet"   // For GPRS, use your provider APN. For Cosmote, that's "internet"
//#define APN_USER ""         // If your APN requires a username, enter here else ""
//#define APN_PASS ""         // If your APN requires a password, enter here else ""

/** VODAFONE **/
#define APN "webonly.vodafone.gr"
#define APN_USER ""         // If your APN requires a username, enter here else ""
#define APN_PASS ""         // If your APN requires a password, enter here else ""

/*** END PROJECT SETUP ***/

/*** Define Pin assignment ***/
#define FONA_RX     2   // FONA Serial In
#define FONA_TX     3   // FONA Serial Out
#define FONA_RST    4   // FONA Command bus

#define TFT_DC      9   // TFT 
#define TFT_CS      10  // TFT Cable select
//#define SPI_1     11  // Pin used internally by HW SPI, do not use for other
//#define SPI_2     12  // Pin used internally by HW SPI, do not use for other
//#define SPI_3     13  // Pin used internally by HW SPI, do not use for other

#define TS_YP       A1  // Touch sensor Y+ must be an analog pin, use "An" notation!
#define TS_XP       A2  // Touch sensor X+ can be a digital pin
#define TS_YM       A3  // Touch sensor Y- can be a digital pin
#define TS_XM       A4  // Touch sensor X- must be an analog pin, use "An" notation!

bool netinit = false;
char msg[90];

/*** Include requirements for FONA, and initialization ***/
#include "Adafruit_FONA.h"

#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Expose a global object for FONA
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

/*** Include requirements for TFT and touch sensor ***/
#include <Adafruit_GFX.h>       // Core graphics library
#include <Adafruit_ILI9341.h>   // Out TFT Monitor driver
#include <SPI.h>                // SPI support
#include "TouchScreen.h"        // Adafruit Touchscreen library

// Pass pins, and the resistance between X+ and X- in Ω for calibration as measured for this board.
TouchScreen ts = TouchScreen(TS_XP, TS_YP, TS_XM, TS_YM, 287);

// This is calibration data for the raw touch data to the screen coordinates
// Touch screen edges to find actual values to calibrate.
#define TS_MINY 50
#define TS_MAXY 850

#define TS_MINX 80
#define TS_MAXX 910

// The display also uses hardware SPI (pins 11-13) plus the ones defined below.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void printUI() {
  tft.fillScreen(ILI9341_WHITE);
  // Draw black grid around buttons
  tft.drawLine(107,   0, 107, 239, ILI9341_BLACK);
  tft.drawLine(215,   0, 214, 239, ILI9341_BLACK);
  tft.drawLine(0,   120, 319, 120, ILI9341_BLACK);
  
  tft.fillRect(0,     0, 106, 119, ILI9341_BLUE);  
  tft.fillRect(108,   0, 106, 119, ILI9341_GREEN);
  tft.fillRect(216,   0, 106, 119, ILI9341_BLUE);
  tft.setCursor(35,70);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(6);
  tft.setTextWrap(false);
  tft.print("1  2  3");

  tft.fillRect(0,   121, 106, 120, ILI9341_GREEN);
  tft.fillRect(108, 121, 106, 120, ILI9341_BLUE);
  tft.fillRect(216, 121, 106, 120, ILI9341_GREEN);
  tft.setCursor(35,140);
  tft.print("4  5  6");    
}

void printSend() {
  tft.fillRect(15,15,290,210, ILI9341_MAGENTA);
  tft.setCursor(30,80);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.setTextWrap(false);
  tft.print("SENDING...");
}

uint8_t checkButton(uint8_t &btn) {
  TSPoint p = ts.getPoint();
  if(p.z < 800 || p.z > 2000) { return 0; }

  // Invert axes due to screen rotation
  btn = p.x;
  p.x = p.y;
  p.y = btn;
  btn = 0;
  sprintf(msg, "-------------------> TS! X: %u, Y: %u, Z:%u", p.x, p.y, p.z);
  Serial.println(msg);

  // Scale from ~90>870 (touch sensor) to tft.width using the calibration #'s
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, 240); 
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, 320);

  sprintf(msg, "<------------------- TFT: X: %u, Y: %u, Z:%u", p.x, p.y, p.z);
  Serial.println(msg);

  // Get which button we have horizontally
  if(p.x<108) {
    btn = 1;
  } else if (p.x<216) {
    btn = 2;
  } else { 
    btn = 3;
  }

  if(p.y<120) {
    // We are on the bottom row, increase to reflect
    btn +=3; // Should be 4,5,6
  }

  return btn;
  
}


void setup() {
  // put your setup code here, to run once:
  while (!Serial); // Wait for FONA initialization
  Serial.begin(4800);

  Serial.println("PlayPort serial debug");
  Serial.println("Initializing....(May take 3 seconds)");
  bool netinit = false;

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);
  
  tft.setCursor(30,80);
  tft.setTextColor(ILI9341_RED, ILI9341_WHITE);
  tft.setTextSize(4);
  tft.setTextWrap(false);
  tft.print("PLEASE WAIT");

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println("FATAL ERROR: COULD NOT FIND FONA MODULE.");
    while (1);
  }
  Serial.println("FONA is alive");

//  tft.drawLine(0,0,319,0,ILI9341_RED);
//  tft.drawLine(0,0,0,239,ILI9341_RED);
//  tft.drawLine(319,0,319,239,ILI9341_RED);
//  tft.drawLine(0,239,319,239,ILI9341_RED);

}

void loop() {
  uint8_t btn=0;
  // Do every 5sec until successful
  if(netinit==false && millis()%5000==0) {
    if(fona.getNetworkStatus()==1) {
      // Turn on GPRS data
      fona.setGPRSNetworkSettings(F(APN), F(APN_USER), F(APN_PASS));
      delay(2000);
      
      // Turn on GPS
      if (!fona.enableGPS(true)) {
        Serial.println("GPS-ON: FAIL");
        tft.fillScreen(ILI9341_RED);
        while(1);
      } else {
        Serial.println("GPS-ON: SUCCESS");
      }

      // Turn on GPRS
      if (!fona.enableGPRS(true)) {
        Serial.println("GPRS-ON: FAIL");
        tft.fillScreen(ILI9341_RED);
        while(1);
      } else {
        Serial.println("GPRS-ON: SUCCESS");
        fona.enableNetworkTimeSync(true);
      }
      netinit = true;
      tft.fillScreen(ILI9341_WHITE);
      printUI();              
    }
  }

  if(netinit) {
    checkButton(btn);
    
    if(btn>0) {
      printSend();
      Serial.println("SUCCESS!");
      Serial.print("Raw button is ");
      Serial.println(btn);
      
      uint16_t ret;
      
      Serial.println("Checking GPS");
      msg[0] = '\0';
      
      if(fona.GPSstatus()>1) {
        fona.getGPS(0,msg,120);  
      } else {
        Serial.println("GPS has no lock, moving on to GPRS");
        if(!fona.getGSMLoc(ret, msg, 150)){          
          Serial.println("failed to get gprs loc");
        }
        if(ret==0) {
          Serial.println("SUCCESS getting loc from GPRS");          
        } else {
          Serial.print("FAILED to get GPRS loc with error code ");
          Serial.println(ret);
        }
        Serial.println(msg);
      }
      //sprintf(msg, "%u button at %s", btn, msg);
      Serial.println("Sending text:");
      //sprintf(msg, "%s at button %u",msg, btn);
      switch(btn) {
        case 1:
          sprintf(msg, "%s | SEA TRASH",msg);
        break;
        case 2:
          sprintf(msg, "%s | SIDEWALK TRASH",msg);
        break;
        case 3:
          sprintf(msg, "%s | BIN FULL",msg);
        break;
        case 4:
          sprintf(msg, "%s | BROKEN TOYS",msg);
        break;
        case 5:
          sprintf(msg, "%s | STRAY",msg);
        break;
        case 6:
          sprintf(msg, "%s | FECES",msg);
        break;
        default:
        sprintf(msg, "%s UNKNOWN FUNCTION",msg, btn);
      }
      Serial.println(msg);
      fona.sendSMS(SMSRec, msg);
      delay(2000);
      printUI();
    }
  }  
}

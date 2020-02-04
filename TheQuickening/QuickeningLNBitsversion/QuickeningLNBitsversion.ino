/**
 *  PIN MAP for ESP32 NODEMCU-32S, other ESP32 dev boards will vary
 *  Keypad (12-32
 *  1.8 128/160 TFT PIN MAP: [VCC - 5V, GND - GND, CS - GPIO5, Reset - GPIO16, AO (DC) - GPI17, SDA (MOSI) - GPIO23, SCK - GPIO18, LED - 3.3V]
 */

#include "Quickening.c"
#include <Keypad.h>
#include <string.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <TFT_eSPI.h> 
#include "qrcode.h"

TFT_eSPI tft = TFT_eSPI(); 

//Wifi details
char wifiSSID[] = "YOUR-WIFI-SSID";
char wifiPASS[] = "YOUR-PASS";

String pubkey;
String totcapacity;
const char* payment_request;
bool certcheck = false;

//LNBITS DETAILS
int httpsPort = 443;
const char* lnbitshost = "lnbits.com"; // Change to you LNBits host
String invoicekey = "YOUR-LNBITS-INVOICE-API"; 


String choice;
String payhash;
String on_currency = "BTCGBP"; //currency can be changed here ie BTCUSD BTCGBP etc
String on_sub_currency = on_currency.substring(3);
String memo = "Memo "; //memo suffix, followed by the price then a random number

  String key_val;
  String cntr = "0";
  String inputs;
  int keysdec;
  int keyssdec;
  float temp;  
  String fiat;
  float satoshis;
  String nosats;
  float conversion;
  String postid;
  String data_id;
  String data_lightning_invoice_payreq = "";
  String data_status;
  bool settle = false;
  String payreq;
  String hash;
  String virtkey;
  
//Set keypad
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[rows] = {12, 14, 27, 26}; //connect to the row pinouts of the keypad
byte colPins[cols] = {25, 33, 32}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
int checker = 0;
char maxdig[20];

void setup() {
  tft.begin();
  Serial.begin(115200);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  
  tft.drawXBitmap(0, 0, topmap, 160, 66, TFT_WHITE, TFT_BLACK);
  tft.drawXBitmap(0, 66, middlemap, 160, 28, TFT_BLACK, TFT_BLACK);
  tft.drawXBitmap(0, 94, bottommap, 160, 34, TFT_WHITE, TFT_BLACK);
  
  //connect to local wifi            
  WiFi.begin(wifiSSID, wifiPASS);   
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if(i >= 5){
     tft.fillScreen(TFT_BLACK);
     tft.setCursor(55, 20);
     tft.setTextSize(1);
     tft.setTextColor(TFT_RED);
     tft.println("WIFI NOT CONNECTED");
     ESP.restart();
    }
    delay(1000);
    i++;
  }
  tft.drawXBitmap(0, 0, topmap, 160, 66, TFT_WHITE, TFT_BLACK);
  tft.drawXBitmap(0, 94, bottommap, 160, 34, TFT_WHITE, TFT_BLACK);
  for (int i=0;i<=50;i++){
  tft.drawXBitmap(0, 66, middlemap, 160, 28, TFT_WHITE, TFT_BLACK);
  delay(10);
  tft.drawXBitmap(0, 66, middlemap, 160, 28, TFT_BLUE, TFT_BLACK);
  delay(10);
  }
  page_nodecheck();
  on_rates();

}

void loop() {
  inputs = "";
  page_input();
  displaysats(); 
  bool cntr = false;
  
  while (cntr != true){
    
   char key = keypad.getKey();
   if (key != NO_KEY){
     virtkey = String(key);
       
       if (virtkey == "#"){
        page_processing();
        addinvoice(nosats);
        
        showAddress(payreq);
      
        checkpayment();
        int counta = 0;
         while (settle != true){
           counta++;
           virtkey = String(keypad.getKey());
         
           if (virtkey == "*"){
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(52, 40);
            tft.setTextSize(1);
            tft.setTextColor(TFT_RED);
            tft.println("CANCELLED");
            delay(1000);
            settle = true;
            cntr = true;
           }
           else{
            checkpayment();
             if (settle == true){
              tft.fillScreen(TFT_BLACK);
              tft.setCursor(52, 40);
              tft.setTextSize(1);
              tft.setTextColor(TFT_GREEN);
              tft.println("COMPLETE");
              delay(1000);
              cntr = true;
             }
           }
           if(counta >30){
            settle = true;
            cntr = true;
           }
         }
       }
      
      else if (virtkey == "*"){
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_WHITE);
        key_val = "";
        inputs = "";  
        nosats = "";
        virtkey = "";
        cntr = "2";
      }
      displaysats();    
    }
  }
}

//display functions

void page_input()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(27, 10);
  tft.println("THE QUICKENING POS");
  tft.setTextSize(1);
  tft.setCursor(0, 35);
  tft.println("AMOUNT THEN #");
  tft.println("");
  tft.println(on_sub_currency + ": ");
  tft.println("");
  tft.println("SATS: ");
  tft.println("");
  tft.println("");
  tft.setTextSize(1);
  tft.setCursor(34, 110);
  tft.println("TO RESET PRESS *");
}

void page_processing()
{ 
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(49, 40);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.println("PROCESSING");
 
}

void page_nodecheck()
{ 
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(49, 40);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.println("INITIALISING");
 
}

void displaysats(){
  inputs += virtkey;
  float temp = float(inputs.toInt()) / 100;
  fiat = String(temp);
  satoshis = temp/conversion;
  int intsats = (int) round(satoshis*100000000.0);
  Serial.println(intsats);
  
  nosats = String(intsats);
  tft.setTextSize(1);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(26, 51);
  tft.println(fiat);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(31, 67);
  tft.println(nosats);
  delay(100);
  virtkey = "";

}


//OPENNODE REQUESTS

void on_rates(){
  WiFiClientSecure client;
  if (!client.connect("api.opennode.co", httpsPort)) {
    return;
  }

  String url = "/v1/rates";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.opennode.co\r\n" +
               "User-Agent: ESP32\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {

      break;
    }
  }
  String line = client.readStringUntil('\n');
    const size_t capacity = 169*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(168) + 3800;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);
    conversion = doc["data"][on_currency][on_currency.substring(3)]; 

}


void addinvoice(String nosats){

 WiFiClientSecure client;
  
  if (!client.connect(lnbitshost, httpsPort)) {
    return;
  }
  
  String topost = "{  \"value\" : \"" + nosats +"\", \"memo\" :\""+ memo + String(random(1,1000)) + "\"}";
  String url = "/v1/invoices";
  client.print(String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitshost + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Grpc-Metadata-macaroon:"+ invoicekey +"\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");



  while (client.connected()) {
    String line = client.readStringUntil('\n');
   Serial.println(line);
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readString();


  Serial.println(line);
  const size_t capacity = JSON_OBJECT_SIZE(2) + 700;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, line);
  const char* pay_req = doc["pay_req"]; 
  const char* payment_hash = doc["payment_hash"]; 
  payreq = pay_req;
  Serial.println(payreq);
  payhash = payment_hash;
  Serial.println(payhash);
}


void checkpayment(){
  
 WiFiClientSecure client;
  
  if (!client.connect(lnbitshost, httpsPort)) {
    return;
  }
  String url = "/v1/invoice/";
  client.print(String("GET ") + url + payhash +" HTTP/1.1\r\n" +
                "Host: " + lnbitshost + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "Grpc-Metadata-macaroon:"+ invoicekey +"\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");


  String line = client.readStringUntil('\n');
  Serial.println(line);

  if (line == ""){
    settle = false;
  }
  else{
    settle = true;
  }
}

void showAddress(String XXX){
  tft.fillScreen(TFT_WHITE);
  XXX.toUpperCase();
 const char* addr = XXX.c_str();
 Serial.println(addr);
  int qrSize = 12;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = String(addr).length();
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+1;
      break;
    }
  }
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize - 3, ECC_LOW, addr);
  Serial.println(qrSize );
 
  float scale = 2;

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if(qrcode_getModule(&qrcode, x, y)){       
        tft.drawRect(15+2+scale*x, 2+scale*y, scale, scale, TFT_BLACK);
      }
      else{
        tft.drawRect(15+2+scale*x, 2+scale*y, scale, scale, TFT_WHITE);
      }
    }
  }
}

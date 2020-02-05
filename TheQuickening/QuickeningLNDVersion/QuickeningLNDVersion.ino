/**
 *  TheQuickening uses a non-fullnode, local Zap desktop wallet install and serveo.net (run in terminal *ssh -R mort.serveo.net:3010:localhost:8180 serveo.net*...Useful Zap guide https://docs.zaphq.io/docs-desktop-neutrino-connect
 *  Macaroons will need to be converted to hex strings, in terminal run "xxd -plain readonly.macaroon > readmac.txt"...in Linux Zap's readonly.macaroon can be found in `$XDG_CONFIG_HOME/Zap/lnd/bitcoin/mainnet/wallet-1` (or under`~/.config`)
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
char wifiSSID[] = "YOUR-WIFI";
char wifiPASS[] = "YOUR-PASS";

//BLITZ DETAILS
const char*  server = "zappedd.ngrok.io"; //Using serveo to tunnel URL means no TLS cert is needed. In terminal run "ssh -R SOME-NAME.serveo.net:3010:localhost:8180 serveo.net"  (change SOME-NAME)
const int httpsPort = 443;
const int lndport = 443;
String pubkey;
String totcapacity;
const char* payment_request;

bool certcheck = false;
String readmacaroon = "0201036c6e64028a01030a105dc5e03a7e5444d4ebbdbb513d1fc40f1201301a0f0a07616464726573731204726561641a0c0a04696e666f1204726561641a100a08696e766f696365731204726561641a0f0a076d6573736167651204726561641a100a086f6666636861696e1204726561641a0f0a076f6e636861696e1204726561641a0d0a05706565727312047265616400000620b964213f708bc349dc1de651b424817651858fcfffa38345f6c566053ee22cf5";
String invoicemacaroon = "0201036c6e640247030a105ec5e03a7e5444d4ebbdbb513d1fc40f1201301a160a0761646472657373120472656164120577726974651a170a08696e766f69636573120472656164120577726974650000062062630006bd8cc1f7ce6be81d90dd2c1b754ecf7d7d0d794fe2c36fec5680876e";
//#include "TLSCert.h" //Un-comment if you need to include a TLS Cert, also uncomment line 279, 303, 360, 403

String choice;

String on_currency = "BTCEUR"; //currency can be changed here ie BTCUSD BTCGBP etc
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
  nodecheck();
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
        reqinvoice(nosats);
        gethash(payreq);
        showAddress(payreq);
        checkpayment(hash);
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
            checkpayment(hash);
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
           if(counta >15){
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


// LND Requests

void nodecheck(){
  bool checker = false;
  while(!checker){
  WiFiClientSecure client;
 
    //client.setCACert(tlscert); 
    
  if (!client.connect(server, lndport)){

    tft.fillScreen(TFT_BLACK);
     tft.setCursor(36, 20);
     tft.setTextSize(1);
     tft.setTextColor(TFT_RED);
     tft.println("NO NODE DETECTED");
     delay(1000);
  }
  else{
    checker = true;
  }
  }
  
}

void reqinvoice(String value){

  
  
   WiFiClientSecure client;
  
  //client.setCACert(tlscert); 
  
  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
      return;   
  }
 
   String topost = "{\"value\": \""+ value 
                   +"\", \"memo\": \""+ memo + String(random(1,1000)) 
                   +"\", \"expiry\": \"1000\","+
                   +"\"private\": true}";

  
  Serial.println(topost);
       client.print(String("POST ")+ "https://" + server +":"+ String(lndport) +"/v1/invoices HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "User-Agent: ESP322\r\n" +
                 "Grpc-Metadata-macaroon:" + invoicemacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close\r\n" +
                 "Content-Length: " + topost.length() + "\r\n" +
                 "\r\n" + 
                 topost + "\n");


     String line = client.readStringUntil('\n');
    Serial.println(line);
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
       
        break;
      }
    }
    
    String content = client.readStringUntil('\n');
    Serial.println(content);

    client.stop();
    
    const size_t capacity = JSON_OBJECT_SIZE(3) + 520;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    const char* r_hash = doc["r_hash"];
    hash = r_hash;
    payment_request = doc["payment_request"]; 
    payreq = payment_request;
    Serial.println(payreq);
 
}

void gethash(String xxx){
  
   WiFiClientSecure client;
   
  //client.setCACert(tlscert); 
  
  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
       return;
  }
   

       client.println(String("GET ") + "https://" + server +":"+ String(lndport) + "/v1/payreq/"+ xxx +" HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "Grpc-Metadata-macaroon:" + readmacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close");
                 
       client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
       
        break;
      }
    }
    

    String content = client.readStringUntil('\n');

    client.stop();

    const size_t capacity = JSON_OBJECT_SIZE(7) + 270;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    const char* payment_hash = doc["payment_hash"]; 
    hash = payment_hash;
}


void checkpayment(String xxx){
  
   WiFiClientSecure client;
  
  //client.setCACert(tlscert); 

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
       return;
  }

       client.println(String("GET ") + "https://" + server +":"+ String(lndport) + "/v1/invoice/"+ xxx +" HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "Grpc-Metadata-macaroon:" + readmacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close");
                 
       client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {

        break;
      }
    }
    

    String content = client.readStringUntil('\n');

    client.stop();
    
    const size_t capacity = JSON_OBJECT_SIZE(9) + 460;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    settle = doc["settled"]; 
    Serial.println(settle);
  
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
  qrcode_initText(&qrcode, qrcodeData, qrSize-1, ECC_LOW, addr);
  Serial.println(qrSize -1);
 
  float scale = 2;

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if(qrcode_getModule(&qrcode, x, y)){       
        tft.drawRect(15+3+scale*x, 3+scale*y, scale, scale, TFT_BLACK);
      }
      else{
        tft.drawRect(15+3+scale*x, 3+scale*y, scale, scale, TFT_WHITE);
      }
    }
  }
}

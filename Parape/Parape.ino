/*                      --------------- SOFTWARE PARAPE ---------------
    AUTHOR: GABRIEL J SCHILLER
            ANSELMO NETO
    Software destined to the PARAPE projec. The project cover the analyse, processing and the action to
    the perfect step-walk. The main reason of this project was to be used on a kid that itself walking
    was weakened, showing to him when he walk wrong.
    Software destined to load on a ESP-32.
    Source code and more information: https://github.com/bubaluza/Parape */

// --------------- LIBRARIES INCLUDED ---------------
#include "WiFi.h"                                                   // High level library to ESP-32 WIFI
#include "esp_wifi.h"                                              // Low level library to ESP-32 WIFI
#include "FS.h"                                                     // File System
#include "SPIFFS.h"                                                 // SPIFFS
// --------------- OBJECT INSTANCE ---------------
WiFiServer server (80);                                             //Instance a WifiServer object 'server' on port 80
// --------------- TIMER POINTERS ---------------
hw_timer_t * timer = NULL;                                          // Timer pointer 1
hw_timer_t * timer2 = NULL;                                         // Timer pointer 2
// --------------- SSID/PASSWORD ESP32 ---------------
const char *ssid = "PARAPE";                                        // Name SSID
const char *password = "parape123";                                 // password to WiFi
// --------------- SPIFFS ---------------
#define FORMAT_SPIFFS_IF_FAILED true
// --------------- INPUT PIN ---------------
#define PinValueSensorHeel 34                                       //Read Pin of Heel Sensor
#define PinValueSensorCenter 39                                     //Read Pin of Center Sensor
#define PinValueSensorTip 36                                        //Read Pin of Tip Sensor
#define PinAccelX  35                                               //Read Pin of accelerometer X
#define PinAccelY  32                                               //Read Pin of accelerometer Y
#define PinAccelZ  33                                              //Read Pin of accelerometer Z
#define PinButton 23
// --------------- OUTPUT PIN ---------------
#define PinMotor 4                                                  //Motor Pin
#define PinLED 2                                                    // LED Pin
// --------------- STRENGTH LIMIT SENSOR ---------------
#define StrLimit 1500                                               //Ac-Dc Limit
// --------------- CONFIG DEFAULT ---------------
#define defaultPwm 50                                               //Default configuration of Motor PWM intensity
#define defaultJoke 2                                               //Default configuration of Playtime
// --------------- TIME DEFINE ---------------
#define TimeMotorMs 3000                                            //Vibration Time
#define TimeLockedUs 60000000                                       //Locked Up time Playtime
#define TimeCheckUs 30000000                                        //Check Time Playtime
// ---------------- VAR CONFIG LOADED -------------------
short int pwmLoad;                                                  //Use/Load PWM Intensity
short int activeJokeLoad;                                           //Use/Load Playtime
int wrongStepsLoad;                                                 //Use/load wrong Steps
// ---------------- SENSOR VAR VALUE ----------------
int ValueSensorHeel = 0;                                            //Heel Sensor Value
int ValueSensorCenter = 0;                                          //Center Sensor Value
int ValueSensorTip = 0;                                             //Tip Sensor  Value
float ValueAccelX = 0.0;                                                //accelerometer X Value
float ValueAccelY = 0.0;                                                //accelerometer Y Value
float ValueAccelZ = 0.0;                                                //accelerometer Z Value
// ---------------- AUX VAR ----------------
bool exitL = 0;                                                      //flag to exit if steps-sequence are right
bool flag = 0;                                                      //flag to Activate Motor if steps-sequence are Wrong
bool lockedSystem = 0;                                              //flag to Locked up System
bool interruption = 0;                                              //flag to Interruption System
bool cleaning = 0;                                                  //flag to clean flags
bool readB = 0;                                                      //Buttom Digital Read
bool wifiStatus = 0;                                                //Configuration Mode Status
int ActivadeMotorCheck = 0;                                         //Lower Limit Playtime
byte i=0;
int mean=20;

// ---------------- FUNCTIONS DECLARATION ----------------
void getSinal() ;                                                   //Read Sensors
void pulseMotor();                                                  //Vibrate Motor
void checkMotor() ;                                                 //Check activate number Motor
void startTimer();                                                  //Start Timer 1
void stopTimer();                                                   //Stop Timer 1
void interrupionBlink();                                            //Playtime LED blink
void startTimer2();                                                 //Start Timer 2
void stopTimer2();                                                  //Stop Timer 2
void htmlx();                                                       //configuration page
void appendFile(fs::FS &fs, const char * path, String  message);    //Change File content
void deleteFile(fs::FS &fs, const char * path);                     //Delete File
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);     //List Director (only to check)
void readFile(fs::FS &fs, const char * path);                       //Read File content to Serial(only to check)
String readFileTo(fs::FS &fs, const char * path);                   //Read File content
bool isStopped();                                                   //Check if the person is standing
// ---------------- STARTUP SETUP ----------------
void setup() {
  delay(2000);
  pinMode(PinLED, OUTPUT);
  digitalWrite (PinLED, HIGH);
  Serial.begin(115200);
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {                     //Mount SPIFFS
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  if (readFileTo(SPIFFS, "/pwm.txt") == "error") {                  //Check if dont exist file/Content or Open Error
    deleteFile(SPIFFS, "/pwm.txt");
    appendFile(SPIFFS, "/pwm.txt", (String)defaultPwm);             //Put Default PWM file
  }
  pwmLoad = (readFileTo(SPIFFS, "/pwm.txt")).toInt();               //Read PWM file

  if (readFileTo(SPIFFS, "/playtime.txt") == "error") {             //Check if dont exist file/Content or Open Error
    deleteFile(SPIFFS, "/playtime.txt");
    appendFile(SPIFFS, "/playtime.txt", (String)defaultJoke);       //Put Default Playtime file
  }
  activeJokeLoad = (readFileTo(SPIFFS, "/playtime.txt")).toInt();   //Read PWM file

  if (readFileTo(SPIFFS, "/steps.txt") == "error") {                //Check if dont exist file/Content or Open Error
    deleteFile(SPIFFS, "/steps.txt");
    appendFile(SPIFFS, "/steps.txt", "0");                          //Put '0' on Step File
  }
  wrongStepsLoad = (readFileTo(SPIFFS, "/steps.txt")).toInt();      //Read Steps File
  ledcAttachPin(PinMotor, 2);                                       //put PinMotor on PWM channel 2
  ledcSetup(2, 500, 10);                                            //set PWM channel 2: 500 hz, 10 bits resolution

  WiFi.softAP(ssid, password);                                      //Start Wifi on AP Mode
  esp_wifi_stop();                                                  //Stop WiFi
  delay(5000);
  Serial.println("ESP32 started and openned Serial");
  pinMode(PinMotor, OUTPUT);
  pinMode(PinButton, INPUT_PULLDOWN);
  digitalWrite (PinMotor, LOW); 
  server.begin();                                                   //Start a WiFi server
  startTimer();
  digitalWrite (PinLED, LOW);
}


//---------------- MAIN LOOP ----------------
void loop() {
  if (lockedSystem) {
    Serial.print("locked system for");
    Serial.print(TimeLockedUs / 1000000);
    Serial.println("seconds.");

    startTimer2();
    delayMicroseconds(TimeLockedUs);
    lockedSystem = 0;
    startTimer();
    ActivadeMotorCheck = 0;
  }

  if (cleaning) {
    ActivadeMotorCheck = 0;
    cleaning = 0;
  }

  readB = digitalRead(PinButton);
  if (readB == 1) {
    while (readB == 1)                                               //wait Release buttom
      readB = digitalRead(PinButton);
    if (wifiStatus == 0) {                                          //if wifi are OFF
      esp_wifi_start();                                             //Start Wifi
      digitalWrite(PinLED, wifiStatus = 1);
    }
    else if (wifiStatus == 1) {                                     //if wifi are ON
      esp_wifi_stop();                                              //Stop Wifi
      digitalWrite(PinLED, wifiStatus = 0);
    }
    delay(6000);
  }
  if (wifiStatus) {                                                 //configuration Mode listening
    htmlx();
  }
  else {                                                            //evaluate Heel -> Tip Sensor
    getSinal();                                                 
    if ( (!isStopped()) && (ValueSensorHeel > StrLimit ) ) {
      while (ValueSensorHeel > StrLimit || flag != 1) {
        getSinal();
        if (ValueSensorTip > StrLimit) {
          exitL = 1;
          while (ValueSensorTip > StrLimit)
            getSinal();                                      
          break;
        }
        if (exitL == 1)
          break;
        if (ValueSensorHeel <= StrLimit)
          flag = 1;
      }
    }
    else if ((!isStopped()) && ValueSensorTip > StrLimit) {         //Evaluate Tip -> Heel Sensor
      while (ValueSensorTip > StrLimit || flag != 1) {
        getSinal();
        if (ValueSensorHeel > StrLimit) {
          exitL = 1;
          while (ValueSensorHeel > StrLimit)
            getSinal();                                
          break;
        }
        if (exitL == 1)
          break;
        if (ValueSensorTip <= StrLimit)
          flag = 1;
      }
    }
    else if ((!isStopped()) && ValueSensorCenter > StrLimit ) {      //Evaluate Center -> Tip Sensor (Stair Walking)
      while (ValueSensorCenter > StrLimit || flag != 1) {
        getSinal();
        if (ValueSensorTip > StrLimit) {
          exitL = 1;
          while (ValueSensorTip > StrLimit)
            getSinal();                                 
          break;
        }    
        if (exitL == 1)
          break;
        if (ValueSensorCenter <= StrLimit)
          flag = 1;
      }
    }
    
    if (flag == 1) {                                                //if Walked wrong Pulse Motor
      ActivadeMotorCheck++;
      wrongStepsLoad++;
      deleteFile(SPIFFS, "/steps.txt");
      appendFile(SPIFFS, "/steps.txt", (String)wrongStepsLoad);
      pulseMotor();
      flag = 0;
    }
    exitL = 0;
  }
}

// --------------- FUNCTIONS ---------------
void getSinal() {
  ValueSensorHeel = analogRead(PinValueSensorHeel);
  ValueSensorCenter = analogRead(PinValueSensorCenter);
  ValueSensorTip = analogRead(PinValueSensorTip);
}

void pulseMotor() {
  ledcWrite(2, map(pwmLoad, 0, 100, 350 , 1022));
  delay(TimeMotorMs);
  ledcWrite(2, 0);
}

void checkMotor() {
  cleaning = 1;
  if ((ActivadeMotorCheck >= activeJokeLoad) && !wifiStatus) {
    lockedSystem = 1;
    stopTimer();
  }
}

void startTimer() {
  timer = timerBegin(3, 80, true);
  timerAttachInterrupt(timer, &checkMotor, true);
  timerAlarmWrite(timer, TimeCheckUs, true);
  timerAlarmEnable(timer);
}

void stopTimer() {
  timerEnd(timer);
  timer = NULL;
}

void interrupionBlink() {
  digitalWrite(PinLED, interruption = !interruption);
  if ( lockedSystem == 0) {
    digitalWrite(PinLED, interruption = 0);
    stopTimer2();
  }
}

void startTimer2() {
  timer2 = timerBegin(2, 80, true);
  timerAttachInterrupt(timer2, &interrupionBlink, true);
  timerAlarmWrite(timer2, 500000, true);
  timerAlarmEnable(timer2);
}

void stopTimer2() {
  timerEnd(timer2);
  timer2 = NULL;
}

bool isStopped() {
    i=0;
  ValueAccelX = 0.0;
  ValueAccelY = 0.0;
  ValueAccelZ = 0.0;
  for(i;i<mean;i++){
   ValueAccelX += ((analogRead(PinAccelX))*3.3)/4095; //read from PinAccelX
   ValueAccelY += ((analogRead(PinAccelY))*3.3)/4095; //read from PinAccelY
   ValueAccelZ += ((analogRead(PinAccelZ))*3.3)/4095; //read from PinAccelZ
   delay(1);
  }
  ValueAccelX = ValueAccelX/mean;
  ValueAccelY = ValueAccelY/mean;
  ValueAccelZ = ValueAccelZ/mean;
  
  float mod=sqrt(   0.22*(pow(ValueAccelX,2)) + 2*(pow(ValueAccelY,2)) + 0.1*(pow(ValueAccelZ,2)) );

    if(  mod>=2.2 && mod <= 2.4 )
        return true;
          else{

            return false; 
          }
            
          
}

void htmlx() {
  WiFiClient client = server.available();                           // Listen for incoming clients
  String payload;
  String vibration;
  String playtime;
  String sleep;
  String line = "";
  if (client) {                                                     // If a new client connects,
    Serial.println("New Client");
    String request = "";
    while (client.connected()) {                                    // loop while the client's connected
      if (client.available()) {                                     // if there's bytes to read from the client,
        char c = client.read();                                     // read a byte
        request += c;
        if (c == '\n') {
          if (line.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (request.indexOf("GET /clear") >= 0) {
              wrongStepsLoad = 0;
              deleteFile(SPIFFS, "/steps.txt");
              appendFile(SPIFFS, "/steps.txt", "0");

              client.println("{\"message\": \"Passos Zerados\"}");
              client.println();
              break;
            } else if (request.indexOf("GET /reset") >= 0) {
              pwmLoad = defaultPwm;
              activeJokeLoad = defaultJoke;
              deleteFile(SPIFFS, "/pwm.txt");
              appendFile(SPIFFS, "/pwm.txt", (String)pwmLoad);
              deleteFile(SPIFFS, "/playtime.txt");
              appendFile(SPIFFS, "/playtime.txt", (String)activeJokeLoad);

              client.println("{\"message\": \"Dados Redefinidos\", \"data\": {\"vibration\": 50, \"playtime\": 2}}");
              client.println();
              break;
            } else if (request.indexOf("GET /save") >= 0) {
              int index = 0;
              String line = request.substring(request.indexOf("GET /") + 5);
              String information = "";
              while (line[index] != '\n') {
                information += line[index];
                index++;
              }
              payload = information.substring(0, information.indexOf(" HTTP/1.1"));
              vibration = payload.substring(payload.indexOf("vibration=") + 10, payload.indexOf('&'));
              String  playtime1 =  payload.substring(payload.indexOf("playtime=") + 9);
              playtime = playtime1.substring(0, playtime1.indexOf('&'));
              pwmLoad = vibration.toInt();
              activeJokeLoad = playtime.toInt();

              if (pwmLoad > 100)
                pwmLoad = 100;
              else if (pwmLoad < 0)
                pwmLoad = 0;

              if (activeJokeLoad < 1)
                activeJokeLoad = 1;

              deleteFile(SPIFFS, "/pwm.txt");
              appendFile(SPIFFS, "/pwm.txt", (String)pwmLoad);
              deleteFile(SPIFFS, "/playtime.txt");
              appendFile(SPIFFS, "/playtime.txt", (String)activeJokeLoad);

              Serial.println(readFileTo(SPIFFS, "/pwm.txt"));
              Serial.println(readFileTo(SPIFFS, "/playtime.txt"));

              client.println("{\"message\": \"Dados Salvos\"}");
              client.println();
              break;
            }

            String html = "<!doctype html>";
            html += "<html lang='pt-BR'>";
            html += "<head>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>";
            html += "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>";
            html += "<title>PARAPE - Configuração de Dispositivo</title>";
            html += "<style rel='stylesheet' type='text/css'> :root{--blue: #007bff; --indigo: #6610f2; --purple: #6f42c1; --pink: #e83e8c; --red: #dc3545; --orange: #fd7e14; --yellow: #ffc107; --green: #28a745; --teal: #20c997; --cyan: #17a2b8; --white: #fff; --gray: #6c757d; --gray-dark: #343a40; --primary: #007bff; --secondary: #6c757d; --success: #28a745; --info: #17a2b8; --warning: #ffc107; --danger: #dc3545; --light: #f8f9fa; --dark: #343a40; --breakpoint-xs: 0; --breakpoint-sm: 576px; --breakpoint-md: 768px; --breakpoint-lg: 992px; --breakpoint-xl: 1200px; --font-family-sans-serif: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif, 'Apple Color Emoji', 'Segoe UI Emoji', 'Segoe UI Symbol'; --font-family-monospace: SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace}*, ::after, ::before{box-sizing: border-box}html{font-family: sans-serif; line-height: 1.15; -webkit-text-size-adjust: 100%; -ms-text-size-adjust: 100%; -ms-overflow-style: scrollbar; -webkit-tap-highlight-color: transparent}body{margin: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif, 'Apple Color Emoji', 'Segoe UI Emoji', 'Segoe UI Symbol'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; text-align: left; background-color: #fff}hr{box-sizing: content-box; height: 0; overflow: visible}h2, h3{margin-top: 0; margin-bottom: .5rem}p{margin-top: 0; margin-bottom: 1rem}img{vertical-align: middle; border-style: none}label{display: inline-block; margin-bottom: .5rem}button{border-radius: 0}button:focus{outline: 1px dotted;}button, input{margin: 0; font-family: inherit; font-size: inherit; line-height: inherit}button, input{overflow: visible}button{text-transform: none}button, html [type=button]{-webkit-appearance: button}[type=button]::-moz-focus-inner, button::-moz-focus-inner{padding: 0; border-style: none}[type=number]::-webkit-inner-spin-button, [type=number]::-webkit-outer-spin-button{height: auto}::-webkit-file-upload-button{font: inherit; -webkit-appearance: button}h2, h3{margin-bottom: .5rem; font-family: inherit; font-weight: 500; line-height: 1.2; color: inherit}h2{font-size: 2rem}h3{font-size: 1.75rem}hr{margin-top: 1rem; margin-bottom: 1rem; border: 0; border-top: 1px solid rgba(0, 0, 0, .1)}.container{width: 100%; padding-right: 15px; padding-left: 15px; margin-right: auto; margin-left: auto}@media (min-width: 576px){.container{max-width: 540px}}@media (min-width: 768px){.container{max-width: 720px}}@media (min-width: 992px){.container{max-width: 960px}}@media (min-width: 1200px){.container{max-width: 1140px}}.row{display: -webkit-box; display: -ms-flexbox; display: flex; -ms-flex-wrap: wrap; flex-wrap: wrap; margin-right: -15px; margin-left: -15px}.col, .col-lg-3, .col-lg-6, .col-md-3, .col-md-6, .col-md-8, .col-sm-12, .col-sm-6{position: relative; width: 100%; min-height: 1px; padding-right: 15px; padding-left: 15px}.col{-ms-flex-preferred-size: 0; flex-basis: 0; -webkit-box-flex: 1; -ms-flex-positive: 1; flex-grow: 1; max-width: 100%}@media (min-width: 576px){.col-sm-6{-webkit-box-flex: 0; -ms-flex: 0 0 50%; flex: 0 0 50%; max-width: 50%}.col-sm-12{-webkit-box-flex: 0; -ms-flex: 0 0 100%; flex: 0 0 100%; max-width: 100%}}@media (min-width: 768px){.col-md-3{-webkit-box-flex: 0; -ms-flex: 0 0 25%; flex: 0 0 25%; max-width: 25%}.col-md-6{-webkit-box-flex: 0; -ms-flex: 0 0 50%; flex: 0 0 50%; max-width: 50%}.col-md-8{-webkit-box-flex: 0; -ms-flex: 0 0 66.666667%; flex: 0 0 66.666667%; max-width: 66.666667%}}@media (min-width: 992px){.col-lg-3{-webkit-box-flex: 0; -ms-flex: 0 0 25%; flex: 0 0 25%; max-width: 25%}.col-lg-6{-webkit-box-flex: 0; -ms-flex: 0 0 50%; flex: 0 0 50%; max-width: 50%}}.form-control{display: block; width: 100%; padding: .375rem .75rem; font-size: 1rem; line-height: 1.5; color: #495057; background-color: #fff; background-clip: padding-box; border: 1px solid #ced4da; border-radius: .25rem; transition: border-color .15s ease-in-out, box-shadow .15s ease-in-out}.form-control::-ms-expand{background-color: transparent; border: 0}.form-control:focus{color: #495057; background-color: #fff; border-color: #80bdff; outline: 0; box-shadow: 0 0 0 .2rem rgba(0, 123, 255, .25)}.form-control::-webkit-input-placeholder{color: #6c757d; opacity: 1}.form-control::-moz-placeholder{color: #6c757d; opacity: 1}.form-control:-ms-input-placeholder{color: #6c757d; opacity: 1}.form-control::-ms-input-placeholder{color: #6c757d; opacity: 1}.form-control:disabled{background-color: #e9ecef; opacity: 1}.form-control-range{display: block; width: 100%}.btn{display: inline-block; font-weight: 400; text-align: center; white-space: nowrap; vertical-align: middle; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; border: 1px solid transparent; padding: .375rem .75rem; font-size: 1rem; line-height: 1.5; border-radius: .25rem; transition: color .15s ease-in-out, background-color .15s ease-in-out, border-color .15s ease-in-out, box-shadow .15s ease-in-out}.btn:focus, .btn:hover{text-decoration: none}.btn:focus{outline: 0; box-shadow: 0 0 0 .2rem rgba(0, 123, 255, .25)}.btn:disabled{opacity: .65}.btn:not(:disabled):not(.disabled){cursor: pointer}.btn:not(:disabled):not(.disabled).active, .btn:not(:disabled):not(.disabled):active{background-image: none}.btn-info{color: #fff; background-color: #17a2b8; border-color: #17a2b8}.btn-info:hover{color: #fff; background-color: #138496; border-color: #117a8b}.btn-info:focus{box-shadow: 0 0 0 .2rem rgba(23, 162, 184, .5)}.btn-info:disabled{color: #fff; background-color: #17a2b8; border-color: #17a2b8}.btn-info:not(:disabled):not(.disabled).active, .btn-info:not(:disabled):not(.disabled):active{color: #fff; background-color: #117a8b; border-color: #10707f}.btn-info:not(:disabled):not(.disabled).active:focus, .btn-info:not(:disabled):not(.disabled):active:focus{box-shadow: 0 0 0 .2rem rgba(23, 162, 184, .5)}.btn-danger{color: #fff; background-color: #dc3545; border-color: #dc3545}.btn-danger:hover{color: #fff; background-color: #c82333; border-color: #bd2130}.btn-danger:focus{box-shadow: 0 0 0 .2rem rgba(220, 53, 69, .5)}.btn-danger:disabled{color: #fff; background-color: #dc3545; border-color: #dc3545}.btn-danger:not(:disabled):not(.disabled).active, .btn-danger:not(:disabled):not(.disabled):active{color: #fff; background-color: #bd2130; border-color: #b21f2d}.btn-danger:not(:disabled):not(.disabled).active:focus, .btn-danger:not(:disabled):not(.disabled):active:focus{box-shadow: 0 0 0 .2rem rgba(220, 53, 69, .5)}.btn-link{font-weight: 400; color: #007bff; background-color: transparent}.btn-link:hover{color: #0056b3; text-decoration: underline; background-color: transparent; border-color: transparent}.btn-link:focus{text-decoration: underline; border-color: transparent; box-shadow: none}.btn-link:disabled{color: #6c757d}.btn-sm{padding: .25rem .5rem; font-size: .875rem; line-height: 1.5; border-radius: .2rem}@supports ((-webkit-transform-style:preserve-3d) or (transform-style:preserve-3d)){}@supports ((-webkit-transform-style:preserve-3d) or (transform-style:preserve-3d)){}@supports ((-webkit-transform-style:preserve-3d) or (transform-style:preserve-3d)){}@supports ((position:-webkit-sticky) or (position:sticky)){}.text-center{text-align: center !important}.text-danger{color: #dc3545 !important}@media print{*, ::after, ::before{text-shadow: none !important; box-shadow: none !important}img{page-break-inside: avoid}h2, h3, p{orphans: 3; widows: 3}h2, h3{page-break-after: avoid}@page{size: a3}body{min-width: 992px !important}.container{min-width: 992px !important}}input[type=range]{-webkit-appearance: none; width: 100%; background: 0 0}input[type=range]::-webkit-slider-thumb{-webkit-appearance: none; border: 1px solid #000; height: 30px; width: 16px; border-radius: 3px; background: #fff; cursor: pointer; margin-top: -10px; box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d}input[type=range]:focus{outline: 0}input[type=range]::-moz-range-thumb{box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d; border: 1px solid #000; height: 36px; width: 16px; border-radius: 3px; background: #fff; cursor: pointer}input[type=range]::-ms-thumb{box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d; border: 1px solid #000; height: 36px; width: 16px; border-radius: 3px; background: #fff; cursor: pointer}input[type=range]::-webkit-slider-runnable-track{width: 100%; height: 8.4px; cursor: pointer; box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d; background: linear-gradient(to left, #dc3545 25%, #138496 100%); border-radius: 1.3px; border: .2px solid #010101}input[type=range]:focus::-webkit-slider-runnable-track{background: linear-gradient(to left, #dc3545 25%, #138496 100%)}input[type=range]::-moz-range-track{width: 100%; height: 8.4px; cursor: pointer; box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d; background: #3071a9; border-radius: 1.3px; border: .2px solid #010101}input[type=range]::-ms-track{width: 100%; height: 8.4px; cursor: pointer; background: 0 0; border-color: transparent; border-width: 16px 0; color: transparent}input[type=range]::-ms-fill-lower{background: #2a6495; border: .2px solid #010101; border-radius: 2.6px; box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d}input[type=range]:focus::-ms-fill-lower{background: #3071a9}input[type=range]::-ms-fill-upper{background: #3071a9; border: .2px solid #010101; border-radius: 2.6px; box-shadow: 1px 1px 1px #000, 0 0 1px #0d0d0d}input[type=range]:focus::-ms-fill-upper{background: #367ebd}.col-lg-3, .col-md-6, .col-sm-12, .col-xs-12{padding-top: 20px}.tooltip{position: relative; display: inline-block; border-bottom: 1px dotted black;}.tooltip .tooltip-text{visibility: hidden; width: 280px; background-color: #555; color: #fff; text-align: center; border-radius: 6px; padding: 5px; position: absolute; z-index: 1; bottom: 125%; left: 50%; margin-left: -60px; opacity: 0; transition: opacity 0.3s;}.tooltip .tooltip-text::after{content: ''; position: absolute; top: 100%; left: 50%; margin-left: -5px; border-width: 5px; border-style: solid; border-color: #555 transparent transparent transparent;}.tooltip:hover .tooltip-text{visibility: visible; opacity: 1;}  .alert {position: relative;padding: .75rem 1.25rem;margin-bottom: 1rem;border: 1px solid transparent;border-radius: .25rem}.alert-success {color: #155724;background-color: #d4edda;border-color: #c3e6cb}</style>";
            html += "</head>";
            html += "<body>";
            html += "<div class='container' style='padding-top: 20px;'>";
            html += "<div class='row'>";
            html += "<div class='col-xs-12 col-md-12 col-sm-12 col-lg-12 text-center'>";
            html += "<h2 style='font-size: 100px'>PARAPE</h2> <h3>Configuração de Dispositivo</h3>";
            html += "</div>";
            html += "</div>";
            html += "</div>";
            html += "<div class='container' style='padding-top: 40px'>";
            html += "<div class='row' id='alert' style='visibility: hidden'>";
            html += "<div class='col'>";
            html += "<div class='alert alert-success' role='alert' id='messages'></div>";
            html += "</div>";
            html += "</div>";
            html += "<form method='get'>";
            html += "<div class='row'>";
            html += "<div class='col-xs-12 col-sm-12 col-md-6 col-lg-6'>";
            html += "<label for='vibration' class='tooltip'> Vibração ";
            html += "<span style='line-height: 18px; text-align: center; vertical-align: middle; border: 0.5px solid #000000; border-radius: 50%; height: 20px;  width: 20px;  display: inline-block; font-size: 18px'> ? </span>";
            html += "<span class='tooltip-text'>Vibração do Dispositivo</span>";
            html += "</label>";
            html += "<input id=vibration name=vibration type='range' value=':vibration' class='form-control-range' min='1' max='100'>";
            html += "</div>";
            html += "<div class='col-xs-12 col-sm-12 col-md-6 col-lg-6'>";
            html += "<label for='playtime' class='tooltip'> Modo brincadeira";
            html += "<span style='line-height: 18px; text-align: center; vertical-align: middle; border: 0.5px solid #000000; border-radius: 50%; height: 20px;  width: 20px;  display: inline-block; font-size: 18px'> ? </span>";
            html += "<span class='tooltip-text'>Número mínimo de passos errados pro dispositivo travar por 1 minuto</span> </label>";
            html += "<input class='form-control' id='playtime' value=':playtime' type='number' min='1' name='playtime'>";
            html += "</div>";
            html += "</div>";
            html += "</form>";
            html += "<hr>";
            html += "<div class='row'>";
            html += "<div class='col-xs-6 col-sm-6 col-md-3 col-lg-3'>";
            html += "<p> Total de Passos: <span id='passCounter'>:counter</span> <button class='btn btn-link text-danger' id='clear' type='button'> Zerar</button> </p>";
            html += "</div>";
            html += "</div>";
            html += "<hr>";
            html += "<button type='button' class='btn btn-info btn-sm' id='save'> Salvar</button>";
            html += "<button class='btn btn-danger btn-sm' id='reset' type='button'> Restaurar</button>";
            html += "</div>";
            html += "<script type='text/javascript'>(function(){'use strict';let form=document.getElementsByTagName('form')[0];let vibrationInput=document.getElementById('vibration');let playtimeInput=document.getElementById('playtime');let clearButton=document.getElementById('clear');let saveButton=document.getElementById('save');let resetButton=document.getElementById('reset');let alert=document.getElementById('alert');let messages=document.getElementById('messages');let counter = document.getElementById('passCounter');form.onsubmit=function(event){event.preventDefault()};clearButton.onclick=function(){let client=new HttpClient();client.get(`/clear`,function(response){let json=JSON.parse(response);alert.style.visibility='visible';messages.innerHTML=json.message;counter.innerHTML = 0;setTimeout(function(){alert.style.visibility='hidden';messages.innerHTML=''},3000)})};saveButton.onclick=function(){let client=new HttpClient();client.get(`/save?vibration=${vibrationInput.value}&playtime=${playtimeInput.value}`,function(response){let json=JSON.parse(response);alert.style.visibility='visible';messages.innerHTML=json.message;setTimeout(function(){alert.style.visibility='hidden';messages.innerHTML=''},3000)})};resetButton.onclick=function(){let client=new HttpClient();client.get(`/reset`,function(response){let json=JSON.parse(response);alert.style.visibility='visible';messages.innerHTML=json.message;vibrationInput.value=json.data.vibration;playtimeInput.value=json.data.playtime;setTimeout(function(){alert.style.visibility='hidden';messages.innerHTML=''},3000)})};let HttpClient=function(){this.get=function(aUrl,aCallback){let anHttpRequest=new XMLHttpRequest();anHttpRequest.onreadystatechange=function(){if(anHttpRequest.readyState===4&&anHttpRequest.status===200){aCallback(anHttpRequest.responseText)}};anHttpRequest.open('GET',aUrl,!0);anHttpRequest.send(null)}}}())</script>";
            html += "</body>";
            html += "</html>";

            html.replace(":vibration", (String)pwmLoad);
            html.replace(":playtime", (String)activeJokeLoad);
            html.replace(":counter", (String)wrongStepsLoad);

            client.println(html);
            client.println();
            break;
          } else {
            line = "";
          }
        } else if (c != '\r') {
          line += c;
        }
      }
    }
    request = "";

    client.stop();                                              // Clear the header variable
    Serial.println("Client disconnected.");                     // Close the connection
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}


String readFileTo(fs::FS &fs, const char * path) {
  String returnString = "";
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return "error";
  }
  else {
    while (file.available()) {
      returnString = returnString + (char)file.read();
    }
    return returnString;
  }
}

void appendFile(fs::FS &fs, const char * path, String  message) {
  Serial.printf("Appending to file: %s  - ", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s -", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

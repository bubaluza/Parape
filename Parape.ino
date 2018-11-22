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
#include <esp_wifi.h>                                               // Low level library to ESP-32 WIFI
#include "FS.h"                                                     // File System
#include "SPIFFS.h"                                                 // SPIFFS
// --------------- OBJECT INSTANCE ---------------
WiFiServer servidor(80);                                            //Instance a WifiServer object 'servidor' on port 80
// --------------- TIMER POINTERS ---------------
hw_timer_t * timer = NULL;
hw_timer_t * timer2 = NULL;
// --------------- SSID/PASSWORD ESP32 ---------------
const char *ssid = "PARAPE";
const char *password = "parape123";
// --------------- SPIFFS ---------------
#define FORMAT_SPIFFS_IF_FAILED true
// --------------- INPUT PIN ---------------
#define PinValueSensorCalcanhar 34
#define PinValueSensorMeio 39
#define PinValueSensorPonta 36
#define PinAcelerX  33
#define PinAcelerY  32
#define PinAcelerZ  35
#define botao 23
// --------------- OUTPUT PIN ---------------
#define PinMotor 4
#define PinLED 2
// --------------- STRENGTH LIMIT SENSOR ---------------
#define forca 1500
// --------------- CONFIG DEFAULT ---------------
#define defaultPwm 50
#define defaultJoke 2
// --------------- TIME DEFINE ---------------
#define tempoMotorMs 3000
#define tempoTravadoUs 60000000
#define tempoCheckUs 30000000
// ---------------- VAR CONFIG LOADED -------------------
short int pwmLoad;
short int ativJokeLoad;
int numPassosErradoLoad;
// ---------------- SENSOR VAR VALUE ----------------
int ValueSensorCalcanhar = 0;
int ValueSensorMeio = 0;
int ValueSensorPonta = 0;
int ValueAcelerX = 0;
int ValueAcelerY = 0;
int ValueAcelerZ = 0;
// ---------------- AUX VAR ----------------
bool sair = 0;
bool flag = 0; //int
bool sistemaTravado = 0;
bool interruption = 0;
bool limpeza = 0;
bool leitura = 0;
bool wifiStatus = 0;
int numMotorAtivadoCheck = 0;
// ---------------- FUNCTIONS DECLARATION ----------------
void getSinal() ;
void vibrar();
void checkMotor() ;
void startTimer();
void stopTimer();
void interrupionBlink();
void startTimer2();
void stopTimer2();
void htmlx();
void appendFile(fs::FS &fs, const char * path, String  mensagem);
void deleteFile(fs::FS &fs, const char * path);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void readFile(fs::FS &fs, const char * path);
String readFileTo(fs::FS &fs, const char * path);
bool estaParado();
// ---------------- STARTUP SETUP ----------------
void setup() {
  delay(2000);
  pinMode(PinLED, OUTPUT);
  digitalWrite (PinLED, HIGH);
  Serial.begin(115200);
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  if (readFileTo(SPIFFS, "/pwm.txt") == "erro") {
    deleteFile(SPIFFS, "/pwm.txt");
    appendFile(SPIFFS, "/pwm.txt", (String)defaultPwm);
  }
  pwmLoad = (readFileTo(SPIFFS, "/pwm.txt")).toInt();

  if (readFileTo(SPIFFS, "/playtime.txt") == "erro") {
    deleteFile(SPIFFS, "/playtime.txt");
    appendFile(SPIFFS, "/playtime.txt", (String)defaultJoke);
  }
  ativJokeLoad = (readFileTo(SPIFFS, "/playtime.txt")).toInt();

  if (readFileTo(SPIFFS, "/passos.txt") == "erro") {
    deleteFile(SPIFFS, "/passos.txt");
    appendFile(SPIFFS, "/passos.txt", "0");
  }
  numPassosErradoLoad = (readFileTo(SPIFFS, "/passos.txt")).toInt();
  ledcAttachPin(PinMotor, 2);
  ledcSetup(2, 500, 10);

  WiFi.softAP(ssid, password);
  esp_wifi_stop();
  delay(5000);
  Serial.println("ESP32 started and openned Serial");
  pinMode(PinMotor, OUTPUT);
  pinMode(botao, INPUT_PULLDOWN);
  digitalWrite (PinMotor, LOW);
  servidor.begin();
  startTimer();
  digitalWrite (PinLED, LOW);
}

//---------------- MAIN LOOP ----------------
void loop() {
  if (sistemaTravado) {
    Serial.print("sistema travado por ");
    Serial.print(tempoTravadoUs / 1000000);
    Serial.println("segundos.");

    startTimer2();
    delayMicroseconds(tempoTravadoUs);
    sistemaTravado = 0;
    startTimer();
    numMotorAtivadoCheck = 0;
  }

  if (limpeza) {
    numMotorAtivadoCheck = 0;
    limpeza = 0;
  }

  leitura = digitalRead(botao);
  if (leitura == 1) {
    while (leitura == 1)
      leitura = digitalRead(botao);
    if (wifiStatus == 0) {
      esp_wifi_start();
      digitalWrite(PinLED, wifiStatus = 1);
    }
    else if (wifiStatus == 1) {
      esp_wifi_stop();
      digitalWrite(PinLED, wifiStatus = 0);
    }
    delay(6000);
  }
  if (wifiStatus) {
    htmlx();
  }
  else {
    getSinal();                                                 
    if (ValueSensorCalcanhar > forca && (!estaParado())) {      
      while (ValueSensorCalcanhar > forca || flag != 1) {      
        getSinal();
        if (ValueSensorPonta > forca) {                       
          sair = 1;                                           
          while (ValueSensorPonta > forca)                    
            getSinal();                                      
          break;
        }
        if (sair == 1)
          break;
        if (ValueSensorCalcanhar <= forca)
          flag = 1;
      }
    }
    else if (ValueSensorPonta > forca && (!estaParado())) {
      while (ValueSensorPonta > forca || flag != 1) {      
        getSinal();
        if (ValueSensorCalcanhar > forca) {                  
          sair = 1;                                    
          while (ValueSensorCalcanhar > forca)            
            getSinal();                                
          break;
        }
        if (sair == 1)
          break;
        if (ValueSensorPonta <= forca)
          flag = 1;
      }
    }
    else if (ValueSensorMeio > forca && (!estaParado())) {
      while (ValueSensorMeio > forca || flag != 1) {
        getSinal();
        if (ValueSensorPonta > forca) {                   
          sair = 1;                                     
          while (ValueSensorPonta > forca)           
            getSinal();                                 
          break;
        }    
        if (sair == 1)
          break;
        if (ValueSensorMeio <= forca)
          flag = 1;
      }
    }
    
    if (flag == 1) {
      numMotorAtivadoCheck++;
      numPassosErradoLoad++;
      deleteFile(SPIFFS, "/passos.txt");
      appendFile(SPIFFS, "/passos.txt", (String)numPassosErradoLoad);
      vibrar();
      flag = 0;
    }
    sair = 0;
  }
}

// --------------- FUNCTIONS ---------------
void getSinal() {
  ValueSensorCalcanhar = analogRead(PinValueSensorCalcanhar);
  ValueSensorMeio = analogRead(PinValueSensorMeio);
  ValueSensorPonta = analogRead(PinValueSensorPonta);
  Serial.print("sensor do calcanhar: ");
  Serial.println(ValueSensorCalcanhar);
  Serial.print("sensor do meio: ");
  Serial.println(ValueSensorMeio);
  Serial.printf("sensor da ponta: ");
  Serial.println(ValueSensorPonta);
  Serial.printf("num: ");
  Serial.println(numMotorAtivadoCheck);
}

void vibrar() {
  ledcWrite(2, map(pwmLoad, 0, 100, 350 , 1022));
  delay(tempoMotorMs);
  ledcWrite(2, 0);
}

void checkMotor() {
  limpeza = 1;
  if ((numMotorAtivadoCheck >= ativJokeLoad) && !wifiStatus) {
    sistemaTravado = 1;
    stopTimer();
  }
}

void startTimer() {
  timer = timerBegin(3, 80, true);
  timerAttachInterrupt(timer, &checkMotor, true);
  timerAlarmWrite(timer, tempoCheckUs, true);
  timerAlarmEnable(timer);
}

void stopTimer() {
  timerEnd(timer);
  timer = NULL;
}

void interrupionBlink() {
  digitalWrite(PinLED, interruption = !interruption);
  if ( sistemaTravado == 0) {
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

bool estaParado() {
  ValueAcelerX = analogRead(PinAcelerX); //read from PinAcelerX
  ValueAcelerY = analogRead(PinAcelerY); //read from PinAcelerY
  ValueAcelerZ = analogRead(PinAcelerZ); //read from PinAcelerZ
  if ( ( (ValueAcelerX > 2170) && (ValueAcelerX < 2275) ) && ( (ValueAcelerY > 2170) && (ValueAcelerY < 2275) ) && ( (ValueAcelerZ > 2170) && (ValueAcelerZ < 2275) )      )
    return true;
  else return false;
}

void htmlx() {
  WiFiClient client = servidor.available();   // Listen for incoming clients
  String payload ;
  String vibration;
  String playtime;
  String sleep ;
  if (client) {                             // If a new client connects,
    String request = "";
    boolean emptyLine = true;
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        request += c;
        if (c == '\n' && emptyLine) {
          if (request.indexOf("GET /") >= 0) {
            int index = 0;
            String line = request.substring(request.indexOf("GET /") + 5);
            String information = "";
            while (line[index] != '\n') {
              information += line[index];
              index++;
            }
            payload = information.substring(0, information.indexOf(" HTTP/1.1"));
            if (payload != "favicon.ico" && payload != "") {
              if (payload == "resetConfiguration") {
                pwmLoad = defaultPwm;
                ativJokeLoad = defaultJoke;
                deleteFile(SPIFFS, "/pwm.txt");
                appendFile(SPIFFS, "/pwm.txt", (String)pwmLoad );
                deleteFile(SPIFFS, "/playtime.txt");
                appendFile(SPIFFS, "/playtime.txt", (String)ativJokeLoad);
              } else if (payload == "clearPassCounter") {
                numPassosErradoLoad = 0;
                deleteFile(SPIFFS, "/passos.txt");
                appendFile(SPIFFS, "/passos.txt", "0");
              } else {
                vibration = payload.substring(payload.indexOf("vibration=") + 10, payload.indexOf('&'));
                String  playtime1 =  payload.substring(payload.indexOf("playtime=") + 9);
                playtime = playtime1.substring(0, playtime1.indexOf('&'));
                pwmLoad = vibration.toInt();
                ativJokeLoad = playtime.toInt();
                if (pwmLoad > 100)
                    pwmLoad = 100;
                else if (pwmLoad < 0)
                    pwmLoad = 0;
                if (ativJokeLoad < 1)
                    ativJokeLoad = 1;
                deleteFile(SPIFFS, "/pwm.txt");
                appendFile(SPIFFS, "/pwm.txt", (String)pwmLoad);
                deleteFile(SPIFFS, "/playtime.txt");
                appendFile(SPIFFS, "/playtime.txt", (String)ativJokeLoad);
              }
            }
          }
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          String html = "HTTP/1.1 200 OK\n";
          html += "Content-type:text/html\n";
          html += "Connection: close\n\n";
          html += "<!DOCTYPE html><html>";
          html += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
          html += "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">";
          html += "<style type=text/css>p,span{font-size:18px}html{font-family:sans-serif;-webkit-text-size-adjust:100%;-ms-text-size-adjust:100%}body{margin:0}[hidden]{display:none}abbr[title]{border-bottom:1px dotted}h1{margin:.67em 0;font-size:2em}code{font-family:monospace,monospace;font-size:1em}button,input,select{margin:0;font:inherit;color:inherit}button{overflow:visible}button,select{text-transform:none}button,html input[type=button],input[type=reset],input[type=submit]{-webkit-appearance:button;cursor:pointer}button[disabled],html input[disabled]{cursor:default}button::-moz-focus-inner,input::-moz-focus-inner{padding:0;border:0}input{line-height:normal}input[type=checkbox],input[type=radio]{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box;padding:0}input[type=number]::-webkit-inner-spin-button,input[type=number]::-webkit-outer-spin-button{height:auto}input[type=search]{-webkit-box-sizing:content-box;-moz-box-sizing:content-box;box-sizing:content-box;-webkit-appearance:textfield}input[type=search]::-webkit-search-cancel-button,input[type=search]::-webkit-search-decoration{-webkit-appearance:none}@media print{*,:after,:before{color:#000!important;text-shadow:none!important;background:0 0!important;-webkit-box-shadow:none!important;box-shadow:none!important}abbr[title]:after{content: ( attr(title) )}h2,h3,p{orphans:3;widows:3}h2,h3{page-break-after:avoid}.label{border:1px solid #000}}@font-face{font-family:'Glyphicons Halflings';src:url(../fonts/glyphicons-halflings-regular.eot);src:url(../fonts/glyphicons-halflings-regular.eot?#iefix) format('embedded-opentype'),url(../fonts/glyphicons-halflings-regular.woff2) format('woff2'),url(../fonts/glyphicons-halflings-regular.woff) format('woff'),url(../fonts/glyphicons-halflings-regular.ttf) format('truetype'),url(../fonts/glyphicons-halflings-regular.svg#glyphicons_halflingsregular) format('svg')}*{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}:after,:before{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}html{font-size:10px;-webkit-tap-highlight-color:transparent}body{font-family:Helvetica Neue,Helvetica,Arial,sans-serif;font-size:14px;line-height:1.42857143;color:#333;background-color:#fff}button,input,select{font-family:inherit;font-size:inherit;line-height:inherit}[role=button]{cursor:pointer}.h1,.h2,.h3,.h4,.h5,.h6,h1,h2,h3,h4,h5,h6{font-family:inherit;font-weight:500;line-height:1.1;color:inherit}.h1,.h2,.h3,h1,h2,h3{margin-top:20px;margin-bottom:10px}.h4,.h5,.h6,h4,h5,h6{margin-top:10px;margin-bottom:10px}.h1,h1{font-size:36px}.h2,h2{font-size:30px}.h3,h3{font-size:24px}.h4,h4{font-size:18px}.h5,h5{font-size:14px}.h6,h6{font-size:12px}p{margin:0 0 10px}.text-success{color:#3c763d}.text-info{color:#31708f}.text-danger{color:#a94442}abbr[data-original-title],abbr[title]{cursor:help;border-bottom:1px dotted #777}code{font-family:Menlo,Monaco,Consolas,Courier New,monospace}code{padding:2px 4px;font-size:90%;color:#c7254e;background-color:#f9f2f4;border-radius:4px}.container{padding-right:15px;padding-left:15px;margin-right:auto;margin-left:auto}@media (min-width:768px){.container{width:750px}}@media (min-width:992px){.container{width:970px}}@media (min-width:1200px){.container{width:1170px}}.row{margin-right:-15px;margin-left:-15px}.col-lg-1,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-md-1,.col-md-10,.col-md-11,.col-md-12,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-sm-1,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{position:relative;min-height:1px;padding-right:15px;padding-left:15px}.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{float:left}.col-xs-12{width:100%}.col-xs-11{width:91.66666667%}.col-xs-10{width:83.33333333%}.col-xs-9{width:75%}.col-xs-8{width:66.66666667%}.col-xs-7{width:58.33333333%}.col-xs-6{width:50%}.col-xs-5{width:41.66666667%}.col-xs-4{width:33.33333333%}.col-xs-3{width:25%}.col-xs-2{width:16.66666667%}.col-xs-1{width:8.33333333%}@media (min-width:768px){.col-sm-1,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9{float:left}.col-sm-12{width:100%}.col-sm-11{width:91.66666667%}.col-sm-10{width:83.33333333%}.col-sm-9{width:75%}.col-sm-8{width:66.66666667%}.col-sm-7{width:58.33333333%}.col-sm-6{width:50%}.col-sm-5{width:41.66666667%}.col-sm-4{width:33.33333333%}.col-sm-3{width:25%}.col-sm-2{width:16.66666667%}.col-sm-1{width:8.33333333%}}@media (min-width:992px){.col-md-1,.col-md-10,.col-md-11,.col-md-12,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9{float:left}.col-md-12{width:100%}.col-md-11{width:91.66666667%}.col-md-10{width:83.33333333%}.col-md-9{width:75%}.col-md-8{width:66.66666667%}.col-md-7{width:58.33333333%}.col-md-6{width:50%}.col-md-5{width:41.66666667%}.col-md-4{width:33.33333333%}.col-md-3{width:25%}.col-md-2{width:16.66666667%}.col-md-1{width:8.33333333%}}@media (min-width:1200px){.col-lg-1,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9{float:left}.col-lg-12{width:100%}.col-lg-11{width:91.66666667%}.col-lg-10{width:83.33333333%}.col-lg-9{width:75%}.col-lg-8{width:66.66666667%}.col-lg-7{width:58.33333333%}.col-lg-6{width:50%}.col-lg-5{width:41.66666667%}.col-lg-4{width:33.33333333%}.col-lg-3{width:25%}.col-lg-2{width:16.66666667%}.col-lg-1{width:8.33333333%}}label{display:inline-block;max-width:100%;margin-bottom:5px;font-weight:700}input[type=search]{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}input[type=checkbox],input[type=radio]{margin:4px 0 0;line-height:normal}input[type=file]{display:block}input[type=range]{display:block;width:100%}select[multiple],select[size]{height:auto}input[type=checkbox]:focus,input[type=file]:focus,input[type=radio]:focus{outline:thin dotted;outline:5px auto -webkit-focus-ring-color;outline-offset:-2px}.form-control{display:block;width:100%;height:34px;padding:6px 12px;font-size:14px;line-height:1.42857143;color:#555;background-color:#fff;background-image:none;border:1px solid #ccc;border-radius:4px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075);box-shadow:inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition:border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s}.form-control:focus{border-color:#66afe9;outline:0;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075),0 0 8px rgba(102,175,233,.6);box-shadow:inset 0 1px 1px rgba(0,0,0,.075),0 0 8px rgba(102,175,233,.6)}.form-control::-moz-placeholder{color:#999;opacity:1}.form-control:-ms-input-placeholder{color:#999}.form-control::-webkit-input-placeholder{color:#999}.form-control[disabled],.form-control[readonly]{background-color:#eee;opacity:1}.form-control[disabled]{cursor:not-allowed}input[type=search]{-webkit-appearance:none}@media screen and (-webkit-min-device-pixel-ratio:0){input[type=date].form-control,input[type=datetime-local].form-control,input[type=month].form-control,input[type=time].form-control{line-height:34px}input[type=date].input-sm,input[type=datetime-local].input-sm,input[type=month].input-sm,input[type=time].input-sm{line-height:30px}input[type=date].input-lg,input[type=datetime-local].input-lg,input[type=month].input-lg,input[type=time].input-lg{line-height:46px}}input[type=checkbox][disabled],input[type=radio][disabled]{cursor:not-allowed}.input-sm{height:30px;padding:5px 10px;font-size:12px;line-height:1.5;border-radius:3px}select.input-sm{height:30px;line-height:30px}select[multiple].input-sm{height:auto}.input-lg{height:46px;padding:10px 16px;font-size:18px;line-height:1.3333333;border-radius:6px}select.input-lg{height:46px;line-height:46px}select[multiple].input-lg{height:auto}.btn{display:inline-block;padding:6px 12px;margin-bottom:0;font-size:14px;font-weight:400;line-height:1.42857143;text-align:center;white-space:nowrap;vertical-align:middle;-ms-touch-action:manipulation;touch-action:manipulation;cursor:pointer;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;background-image:none;border:1px solid transparent;border-radius:4px}.btn:active:focus,.btn:focus{outline:thin dotted;outline:5px auto -webkit-focus-ring-color;outline-offset:-2px}.btn:focus,.btn:hover{color:#333;text-decoration:none}.btn:active{background-image:none;outline:0;-webkit-box-shadow:inset 0 3px 5px rgba(0,0,0,.125);box-shadow:inset 0 3px 5px rgba(0,0,0,.125)}.btn[disabled]{cursor:not-allowed;-webkit-box-shadow:none;box-shadow:none;opacity:.65}.btn-success{color:#fff;background-color:#5cb85c;border-color:#4cae4c}.btn-success:focus{color:#fff;background-color:#449d44;border-color:#255625}.btn-success:hover{color:#fff;background-color:#449d44;border-color:#398439}.btn-success:active{color:#fff;background-color:#449d44;border-color:#398439}.btn-success:active:focus,.btn-success:active:hover{color:#fff;background-color:#398439;border-color:#255625}.btn-success:active{background-image:none}.btn-success[disabled],.btn-success[disabled]:active,.btn-success[disabled]:focus,.btn-success[disabled]:hover{background-color:#5cb85c;border-color:#4cae4c}.btn-info{color:#fff;background-color:#5bc0de;border-color:#46b8da}.btn-info:focus{color:#fff;background-color:#31b0d5;border-color:#1b6d85}.btn-info:hover{color:#fff;background-color:#31b0d5;border-color:#269abc}.btn-info:active{color:#fff;background-color:#31b0d5;border-color:#269abc}.btn-info:active:focus,.btn-info:active:hover{color:#fff;background-color:#269abc;border-color:#1b6d85}.btn-info:active{background-image:none}.btn-info[disabled],.btn-info[disabled]:active,.btn-info[disabled]:focus,.btn-info[disabled]:hover{background-color:#5bc0de;border-color:#46b8da}.btn-danger{color:#fff;background-color:#d9534f;border-color:#d43f3a}.btn-danger:focus{color:#fff;background-color:#c9302c;border-color:#761c19}.btn-danger:hover{color:#fff;background-color:#c9302c;border-color:#ac2925}.btn-danger:active{color:#fff;background-color:#c9302c;border-color:#ac2925}.btn-danger:active:focus,.btn-danger:active:hover{color:#fff;background-color:#ac2925;border-color:#761c19}.btn-danger:active{background-image:none}.btn-danger[disabled],.btn-danger[disabled]:active,.btn-danger[disabled]:focus,.btn-danger[disabled]:hover{background-color:#d9534f;border-color:#d43f3a}.btn-link{font-weight:400;color:#337ab7;border-radius:0}.btn-link,.btn-link:active,.btn-link[disabled]{background-color:transparent;-webkit-box-shadow:none;box-shadow:none}.btn-link,.btn-link:active,.btn-link:focus,.btn-link:hover{border-color:transparent}.btn-link:focus,.btn-link:hover{color:#23527c;text-decoration:underline;background-color:transparent}.btn-link[disabled]:focus,.btn-link[disabled]:hover{color:#777;text-decoration:none}.btn-lg{padding:10px 16px;font-size:18px;line-height:1.3333333;border-radius:6px}.btn-sm{padding:5px 10px;font-size:12px;line-height:1.5;border-radius:3px}.btn-xs{padding:1px 5px;font-size:12px;line-height:1.5;border-radius:3px}.btn-block{display:block;width:100%}.btn-block + .btn-block{margin-top:5px}input[type=button].btn-block,input[type=reset].btn-block,input[type=submit].btn-block{width:100%}[data-toggle=buttons]>.btn input[type=checkbox],[data-toggle=buttons]>.btn input[type=radio]{position:absolute;clip:rect(0,0,0,0);pointer-events:none}.label{display:inline;padding:.2em .6em .3em;font-size:75%;font-weight:700;line-height:1;color:#fff;text-align:center;white-space:nowrap;vertical-align:baseline;border-radius:.25em}.label:empty{display:none}.btn .label{position:relative;top:-1px}.label-success{background-color:#5cb85c}.label-success[href]:focus,.label-success[href]:hover{background-color:#449d44}.label-info{background-color:#5bc0de}.label-info[href]:focus,.label-info[href]:hover{background-color:#31b0d5}.label-danger{background-color:#d9534f}.label-danger[href]:focus,.label-danger[href]:hover{background-color:#c9302c}@-webkit-keyframes progress-bar-stripes{from{background-position:40px 0}to{background-position:0 0}}@-o-keyframes progress-bar-stripes{from{background-position:40px 0}to{background-position:0 0}}@keyframes progress-bar-stripes{from{background-position:40px 0}to{background-position:0 0}}.container:after,.container:before,.row:after,.row:before{display:table;content: }.container:after,.row:after{clear:both}</style>";
          html += "</head>";
          html += "<body> <div class='container'><h2>Configuração PARAPÉ</h2> <p>Aqui você poderá alterar a força de vibração do dispositivo conforme seu gosto e além disso, verificar a quantidade de passos errados enquanto o dispositivo permanecer ativo. </p><br >";
          html += "<form method='get'>";
          html += "<div class='row'>";
          html += "<div class='col-xs-12 col-sm-12 col-md-4 col-lg-4'><label for='vibration'><abbr title='Intensidade da vibração'>Vibração</abbr></label>";
          html += "<input class=form-control id=vibration name=vibration value=':vibration' type=number min=1 max=100>";
          html += "</div>";
          html += "<div class='col-xs-12 col-sm-12 col-md-3 col-lg-3'><label for='playtime'><abbr title='Vezes em que o vibrar deve ser desconsiderado por se tratar de uma brincadeira'>Playtime</abbr></label><input class='form-control' id='playtime' value=':playtime'type='number' min='1' name='playtime'></div>";
        //html += "<div class='col-xs-12 col-sm-12 col-md-3 col-lg-3'><label for='sleep'><abbr title='Tempo de Descanso'>Tempo de Descanso(s)</abbr></label><br><input class='form-control' value=':sleep'id='sleep' type='number' name=sleep></div>";
          html += "<div class='col-xs-12 col-sm-12 col-md-12 col-lg-12' style='padding-top: 10px'><button type='submit' class='btn btn-success btn-block'>Salvar Configuração</button></div>";
          html += "</div>";
          html += "</form>";
          html += "<br>";
          html += "<div class='row'>";
          html += " <div class='col-xs-6 col-sm-6 col-md-6 col-lg-6'><p>'Contagem de Passos'</p><span id='passCounter'>:passCounter</span></div>";
          html += "<div class='col-xs-6 col-sm-6 col-md-6 col-lg-6'><a href='clearPassCounter'><button class='btn btn-danger btn-sm' id='clearPassCounter' type='button'>Zerar Passos</button></a>";
          html += "<br><br>";
          html += "<a href='resetConfiguration'><button class='btn btn-info btn-sm' id='resetConfigurations' type='button'>Restaurar Configurações</button></a></div>";
          html += "</div>";
          html += "</body>";
          html += "</html>";
          html.replace(":vibration", (String)pwmLoad);
          html.replace(":playtime", (String)ativJokeLoad);
          html.replace(":sleep", "20");
          html.replace(":passCounter", (String)numPassosErradoLoad);
          client.println(html);
          client.println();
          break;
        }
        if (c == '\n') {  // inicio de uma nova linha
          emptyLine = true;
        } else if (c != '\r') {  // existe pelo menos um caracter na linha atual
          emptyLine = false;
        }
      }
    }
    // Clear the header variable
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
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
    return "erro";
  }
  else {
    while (file.available()) {
      returnString = returnString + (char)file.read();
    }
    return returnString;
  }
}

void appendFile(fs::FS &fs, const char * path, String  msg) {
  Serial.printf("Appending to file: %s  - ", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(msg)) {
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

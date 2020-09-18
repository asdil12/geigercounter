#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

uint16_t min_radiation_interval = 20;
uint16_t radiation_interval_delta_minmax = 130;

const char* ssid = "Chernobyl";



DNSServer dnsServer;
AsyncWebServer server(80);

String get_param(AsyncWebServerRequest *request, String param_name) {
  int paramsNr = request->params();
  for(int i=0;i<paramsNr;i++){
    AsyncWebParameter* p = request->getParam(i);
    if (p->name() == param_name) {
      return p->value();
    }
    return "";
  }
}


class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    //response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
    const char index_html[] = R"rawliteral(
    <!DOCTYPE html><html><head><title>Geiger Counter</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
    input { width: 100%; }
    </style>
    <script>
      function ajax_update(element) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/"+element.id+"?set="+element.value, true);
        xhr.send();
        document.getElementById(element.id+"_value").innerHTML = element.value;
      }
    </script>
    </head><body>
    <p><center><h2>Geigerz&auml;hler</h2><br><i>3.6 R&ouml;ntgen - not great not terrible</i></center></p><br><br>
    Interval: <span id="interval_value"></span><br>
    <input type="range" id="interval" name="points" min="0" max="200" onchange="ajax_update(this);">
    <br><br>
    Delta: <span id="delta_value"></span><br>
    <input type="range" id="delta" name="points" min="50" max="1000" onchange="ajax_update(this);">
    </body></html>
    )rawliteral";
    response->print(index_html);
    /*
    response->print("<p>This is out captive portal front page.</p>");
    response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
    response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
    */
    //response->print("</body></html>");
    request->send(response);
  }
};

void enable_wifi() {
  Serial.print("\r\n\r\nSetting AP (Access Point)…");

  WiFi.softAP(ssid);
  
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);
  //Serial.println(WiFi.localIP());

  //AsyncWebServer server(80);
  dnsServer.start(53, "*", WiFi.softAPIP());
    
  server.on("/interval", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("set")) {
      AsyncWebParameter* p = request->getParam("set");
      min_radiation_interval = atoi(p->value().c_str());
    }
    char b [10];
    itoa(min_radiation_interval, b, 10);
    request->send(200, "text/plain", b);
  });

    server.on("/delta", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("set")) {
      AsyncWebParameter* p = request->getParam("set");
      radiation_interval_delta_minmax = atoi(p->value().c_str());
    }
    char b [10];
    itoa(radiation_interval_delta_minmax, b, 10);
    request->send(200, "text/plain", b);
  });
  
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP

  /*
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  */
  
  server.begin();
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(14, OUTPUT);
  pinMode(2, OUTPUT);

  Serial.begin(9600);

  enable_wifi();
}

// the loop function runs over and over again forever
void loop() {
  dnsServer.processNextRequest();

  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  tone(14, 2546, 5);
  //digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
  delay(random(min_radiation_interval, min_radiation_interval+radiation_interval_delta_minmax));

}

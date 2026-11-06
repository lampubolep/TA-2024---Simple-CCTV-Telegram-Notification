// Setup menggunakan Arduino IDE 2.3.2
// ESP32 v. 2.0.12

#include "esp_camera.h"
#include <WiFiManager.h> // nanti bs konek ke 192.168.4.1
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <GyverOLED.h>
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

// Select camera model

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Setup Wifi
WiFiManager wm;

// Pengaturan Telegram
String chatId = "put your chat id here";
String BOTtoken = "put your token here";

int buzzer = 12;

bool sendPhoto = false;

WiFiClientSecure clientTCP;

UniversalTelegramBot bot(BOTtoken, clientTCP);

bool motionDetected = false;

void startCameraServer();
void setupLedFlash(int pin);

int botRequestDelay = 1000; // scan pesan ke telegram
long lastTimeBotRan; // terakhir pesan direquest

void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();

// Ketika gerakan terdeteksi
static void IRAM_ATTR detectsMovement(void * arg){
  Serial.println("MOTION DETECTED!!!");
  motionDetected = true;
}

void setup() {
  pinMode(buzzer, OUTPUT);
  oled.init(14, 15);
  oled.clear();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // koneksi ke wifi pake wifimanager
  WiFi.mode(WIFI_STA);
  bool res;
  res = wm.autoConnect("nama SSID","password");
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  WiFi.setSleep(false);
  oled.clear();
  if(!res) {
    Serial.println("Gagal konek WiFi");
    oled.println("Gagal konek WiFi");
    ESP.restart();
  }
  else {
    Serial.println("Konek ke http://");
    Serial.print(WiFi.localIP());
    oled.println("Status:");
    oled.println("Terhubung ke wifi");
    oled.println(WiFi.localIP());
  }


  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

// PIR Motion Sensor mode INPUT_PULLUP
  //err = gpio_install_isr_service(0); 
  err = gpio_isr_handler_add(GPIO_NUM_13, &detectsMovement, (void *) 13);  
  if (err != ESP_OK){
    Serial.printf("handler add failed with error 0x%x \r\n", err); 
  }
  err = gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_POSEDGE);
  if (err != ESP_OK){
    Serial.printf("set intr type failed with error 0x%x \r\n", err);
  }

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  

  startCameraServer();

  oled.setCursor(0, 3);
  oled.autoPrintln(true);
  oled.println("");
  oled.println("Selamat datang di");
  oled.println("Rumah Anzil");

}

void loop() {
  if (sendPhoto){
    Serial.println("Preparing photo");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }

  if(motionDetected){
    bot.sendMessage(chatId, "Tok tok! Ada tamu nih. Link streaming (lokal) http://192.168.1.184:81/stream  Online http://anzilnugraha.my.id:8080/stream Mau /tunggusebentar apa /lagipergi tuan?", "");
    Serial.println("Gerakan terdeteksi");
    sendPhotoTelegram();
    motionDetected = false;
  }

  if (millis() > lastTimeBotRan + botRequestDelay){
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages){
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}

//-- setelah loop

String sendPhotoTelegram(){
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--AnzilNugraha\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--AnzilNugraha\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--AnzilNugraha--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=AnzilNugraha");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void handleNewMessages(int numNewMessages){
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++){
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String fromName = bot.messages[i].from_name;
    if (text == "/photo") {
      bot.sendMessage(chatId, "Baik tunggu sebentar.", "");
      sendPhoto = true;
      Serial.println("New photo  request");
    }
    if (text == "/lagipergi"){
      digitalWrite(buzzer, HIGH);
      delay(400);
      digitalWrite(buzzer, LOW);
      delay(100);
      
      oled.clear();
      oled.setCursor(0, 2);
      oled.setScale(2);
      oled.autoPrintln(true);
      oled.println("Maaf lagi pergi nih!");
      bot.sendMessage(chatId, "Sudah ditampilkan tuan.", "");
      delay(15000);
      oled.clear();
      oled.setScale(1);
      oled.println(WiFi.localIP());
      oled.println("");
      oled.println("Selamat datang di");
      oled.println("Rumah Anzil");
    }
    if (text == "/tunggusebentar"){
      digitalWrite(buzzer, HIGH);
      delay(400);
      digitalWrite(buzzer, LOW);
      delay(100);
      
      oled.clear();
      oled.setCursor(0, 2);
      oled.setScale(2);
      oled.autoPrintln(true);
      oled.println("Tunggu sebentar ya!");
      bot.sendMessage(chatId, "Sudah ditampilkan tuan.", "");
      delay(15000);
      oled.clear();
      oled.setScale(1);
      oled.println(WiFi.localIP());
      oled.println("");
      oled.println("Selamat datang di");
      oled.println("Rumah Anzil");
    }
    if (text == "/start"){
      String welcome = "Sistem notifikasi kehadiran tamu telah dihidupkan.\n";
      welcome += "/photo : request foto ke ESP32 CAM \n";
      welcome += "/lagipergi : kirim pesan lagi pergi\n";
      welcome += "/tunggusebentar : kirim pesan tunggu sebentar\n";
      welcome += "Link streaming (lokal) http://192.168.1.184:81/stream \n";
      welcome += "Online http://anzilnugraha.my.id:8080/stream";
      bot.sendMessage(chatId, welcome, "Markdown");
    }
  }
}

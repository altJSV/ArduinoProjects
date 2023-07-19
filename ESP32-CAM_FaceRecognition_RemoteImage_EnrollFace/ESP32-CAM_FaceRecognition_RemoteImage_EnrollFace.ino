/*
ESP32-CAM Enroll faces by getting remote images from web server and recognize faces automatically.
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-6-29 21:30
https://www.facebook.com/francefu

Arduino ESP32 version 1.0.6
*/

//輸入WIFI連線帳號密碼
const char* ssid = "teacher";
const char* password = "87654321";

//輸入AP端連線帳號密碼  http://192.168.4.1
const char* apssid = "esp32-cam";
const char* appassword = "12345678";         //AP密碼至少要8個字元以上  

//人臉辨識同一人人臉註冊影像數
#define ENROLL_CONFIRM_TIMES 5
//人臉辨識註冊人數
#define FACE_ID_SAVE_NUMBER 7

//設定人臉辨識顯示的人名
String recognize_face_matched_name[7] = {"Name0","Name1","Name2","Name3","Name4","Name5","Name6"};    // 7 persons

//以官方範例get-Still按鈕取得CIF(400x296)解析度可辨識到人臉照片上傳到github網站空間或其他網站空間
//註冊人臉照片 5 images * 7 person = 35 photos
String imageDomain[5] = {"fustyles.github.io", "fustyles.github.io", "fustyles.github.io", "fustyles.github.io", "fustyles.github.io"};
String imageRequest[5] = {"/webduino/test/1.jpg", "/webduino/test/2.jpg", "/webduino/test/3.jpg", "/webduino/test/4.jpg", "/webduino/test/5.jpg"};
int image_width = 400;  
int image_height = 296;
//CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120)

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>    //SPIFFS存取函式庫
#include <FS.h>        //檔案存取函式庫
#include "soc/soc.h"             //用於電源不穩不重開機 
#include "soc/rtc_cntl_reg.h"    //用於電源不穩不重開機 
#include "esp_camera.h"          //視訊函式
#include "img_converters.h"      //影像格式轉換函式
#include "fb_gfx.h"              //影像繪圖函式
#include "fd_forward.h"          //人臉偵測函式
#include "fr_forward.h"          //人臉辨識函式

//安信可ESP32-CAM模組腳位設定
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//初始值
static mtmn_config_t mtmn_config = {0};
static face_id_list id_list = {0};
int8_t enroll_id = 0;

void FaceMatched(int faceid) {  //辨識到註冊人臉
  if (faceid==0) {  
  } 
  else if (faceid==1) { 
  } 
  else if (faceid==2) { 
  } 
  else if (faceid==3) { 
  } 
  else if (faceid==4) { 
  } 
  else if (faceid==5) { 
  } 
  else if (faceid==6) {
  } 
  else {
  }   
}

void FaceNoMatched() {  //辨識到陌生人臉
  
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //關閉電源不穩就重開機的設定
    
  Serial.begin(115200);
  Serial.setDebugOutput(true);  //開啟診斷輸出
  Serial.println();

  //視訊組態設定  https://github.com/espressif/esp32-camera/blob/master/driver/include/esp_camera.h
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  //
  // WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
  //            Ensure ESP32 Wrover Module or other board with PSRAM is selected
  //            Partial images will be transmitted if image exceeds buffer size
  //   
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){  //是否有PSRAM(Psuedo SRAM)記憶體IC
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  //視訊初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  //可自訂視訊框架預設大小(解析度大小)
  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_CIF);    //解析度 UXGA(1600x1200), SXGA(1280x1024), XGA(1024x768), SVGA(800x600), VGA(640x480), CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120), QXGA(2048x1564 for OV3660)

  //s->set_vflip(s, 1);  //垂直翻轉
  //s->set_hmirror(s, 1);  //水平鏡像

  //閃光燈(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);  

  WiFi.mode(WIFI_STA);

  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));


  for (int i=0;i<2;i++) {
    WiFi.begin(ssid, password);    //執行網路連線
  
    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;    //等待10秒連線
    } 
  
    if (WiFi.status() == WL_CONNECTED) {    //若連線成功
      WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);   //設定SSID顯示客戶端IP         
      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");
  
      for (int i=0;i<5;i++) {   //若連上WIFI設定閃光燈快速閃爍
        ledcWrite(4,10);
        delay(200);
        ledcWrite(4,0);
        delay(200);    
      }
      
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {    //若連線失敗
    ESP.restart();
  }    
  
  //閃光燈(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);  

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mounted error");
    ESP.restart();
  } else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }    

  enrollImageRemote();  //讀取遠端圖檔註冊人臉
}

void loop() {
  faceRecognition();
  delay(1000);
}

void enrollImageRemote() {  //取得遠端照片註冊人臉
  if (WiFi.status() != WL_CONNECTED) 
    ESP.restart();
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above
  
  //人臉偵測參數設定
  //https://github.com/espressif/esp-dl/blob/master/face_detection/README.md
  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 20;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 10;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
    
  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);   
  
  String filename = "/enrollface.jpg";  //spiffs存放遠端照片檔名
  String domain = "";
  String request = "";
  int port = 443;
      
  //取得雲端照片註冊人臉，有時會讀取失敗，需要自行加條件判斷是否已完成註冊或重啟電源重新讀取。
  int len = sizeof(imageDomain)/sizeof(*imageDomain);
  if (len>0) {
    for (int i=0;i<len;i++) {
      String domain = imageDomain[i];
      String request = imageRequest[i];
  
      Serial.println("");
      Serial.print("Connecting to ");
      Serial.println(domain);
        
      if (client_tcp.connect(domain.c_str(), port)) {
        Serial.println("GET " + request);
        client_tcp.println("GET " + request + " HTTP/1.1");
        client_tcp.println("Host: " + domain);
        client_tcp.println("Content-type:image/jpeg; charset=utf-8");
        client_tcp.println("Connection: close");
        client_tcp.println();
    
        String getResponse="";
        boolean state = false;
        int waitTime = 1000;   // timeout 1 seconds
        long startTime = millis();
        char c;
        File file = SPIFFS.open(filename, FILE_WRITE);  //將遠端照片存入spiffs
        while ((startTime + waitTime) > millis()) {
          while (client_tcp.available()) {
              c = client_tcp.read();
              if (state==false) {
                //Serial.print(String(c)); 
              }
              else if (state==true) {
                file.print(c);
              }
                         
              if (c == '\n') {
                if (getResponse.length()==0) state=true; 
                getResponse = "";
              } 
              else if (c != '\r') {
                getResponse += String(c);
              }
              startTime = millis();
           }
        }
        client_tcp.stop();
        
        file.close();
     
        file = SPIFFS.open(filename);  //讀取暫存spiffs照片註冊
        if(!file){
          Serial.println("Failed to open file for reading");   
        } else {
          Serial.println("file size: "+String(file.size())); 
          char *buf;
          buf = (char*) malloc (sizeof(char)*file.size());
          long i = 0;
          while (file.available()) {
            buf[i] = file.read(); 
            i++;  
          }
        
          dl_matrix3du_t *aligned_face = NULL;
          int8_t left_sample_face = NULL;
          dl_matrix3du_t *image_matrix = NULL;
          
          image_matrix = dl_matrix3du_alloc(1, image_width, image_height, 3);  //分配內部記憶體
          if (!image_matrix) {
              Serial.println("dl_matrix3du_alloc failed");
          } else { 
              fmt2rgb888((uint8_t*)buf, file.size(), PIXFORMAT_JPEG, image_matrix->item);  //影像格式轉換RGB格式
        
              box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);  //執行人臉偵測取得臉框數據
              if (net_boxes){
                Serial.println("faces = " + String(net_boxes->len));  //偵測到的人臉數
                Serial.println();
                for (int i = 0; i < net_boxes->len; i++){  //列舉人臉位置與大小
                    Serial.println("index = " + String(i));
                    int x = (int)net_boxes->box[i].box_p[0];
                    Serial.println("x = " + String(x));
                    int y = (int)net_boxes->box[i].box_p[1];
                    Serial.println("y = " + String(y));
                    int w = (int)net_boxes->box[i].box_p[2] - x + 1;
                    Serial.println("width = " + String(w));
                    int h = (int)net_boxes->box[i].box_p[3] - y + 1;
                    Serial.println("height = " + String(h));
                    Serial.println();
      
                    //註冊人臉
                    if (i==0) {
                      aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
                      if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
                        if(!aligned_face){
                            Serial.println("Could not allocate face recognition buffer");
                        } 
                        else {
                          int8_t left_sample_face = enroll_face(&id_list, aligned_face);
              
                          if(left_sample_face == (ENROLL_CONFIRM_TIMES - 1)){
                              enroll_id = id_list.tail;
                              Serial.printf("Enrolling Face ID: %d\n", enroll_id);
                          }
                          Serial.printf("Enrolling Face ID: %d sample %d\n", enroll_id, ENROLL_CONFIRM_TIMES - left_sample_face);
                          if (left_sample_face == 0){
                              enroll_id = id_list.tail;
                              //Serial.printf("Enrolled Face ID: %d\n", enroll_id);
                          }
                          Serial.println();
                        }
                        dl_matrix3du_free(aligned_face);
                      }
                    }
                } 
                dl_lib_free(net_boxes->score);
                dl_lib_free(net_boxes->box);
                dl_lib_free(net_boxes->landmark);
                dl_lib_free(net_boxes);                                
                net_boxes = NULL;
              }
              else {
                Serial.println("No Face");    //未偵測到人臉
                Serial.println();
              }
              dl_matrix3du_free(image_matrix);
            }
            free(buf);      
        }
        file.close();
      }  
    }
  }
}

void faceRecognition() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Camera capture failed");
      ESP.restart();
  }
  size_t out_len, out_width, out_height;
  uint8_t * out_buf;
  bool s;
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  if (!image_matrix) {
      esp_camera_fb_return(fb);
      Serial.println("dl_matrix3du_alloc failed");
      return;
  }
  out_buf = image_matrix->item;
  out_len = fb->width * fb->height * 3;
  out_width = fb->width;
  out_height = fb->height;
  s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
  esp_camera_fb_return(fb);
  if(!s){
      dl_matrix3du_free(image_matrix);
      Serial.println("to rgb888 failed");
      return;
  }
  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);  //執行人臉偵測
  if (net_boxes){
      run_face_recognition(image_matrix, net_boxes);  //執行人臉辨識
      dl_lib_free(net_boxes->score);
      dl_lib_free(net_boxes->box);
      dl_lib_free(net_boxes->landmark);
      dl_lib_free(net_boxes);                                
      net_boxes = NULL;
  }
  dl_matrix3du_free(image_matrix);
}

//人臉辨識函式
static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes){  
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if(!aligned_face){
        Serial.println("Could not allocate face recognition buffer");
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
        matched_id = recognize_face(&id_list, aligned_face);  //人臉辨識
        if (matched_id >= 0) {  //若辨識為已註冊之人臉
            Serial.printf("Match Face ID: %u\n", matched_id);
            int name_length = sizeof(recognize_face_matched_name) / sizeof(recognize_face_matched_name[0]);
            if (matched_id<name_length) {
              Serial.printf("Match Face Name: %s\n", recognize_face_matched_name[matched_id]);
            }
            else {
              Serial.printf("Match Face Name: No name");
            }  
            Serial.println();
            FaceMatched(matched_id);  //辨識到註冊人臉執行指令控制
        } else {  //若辨識為未註冊之人臉
            Serial.println("No Match Found");
            Serial.println();
            matched_id = -1;
            FaceNoMatched();  //辨識為陌生人臉執行指令控制
        }
    } else {  //若偵測出人臉，但無法進行識別
        Serial.println("Face Not Aligned");
        Serial.println();
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

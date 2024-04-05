
//-------------- Thư viện --------------
#include <SPI.h>   
 #include <SD.h>
#include <MFRC522.h>                            // RFID RC522
#include <ESP32Servo.h>                              // Servo SG90
#include <FirebaseESP32.h>                      // Firebase module ESP32
#include <WiFi.h>                               // Wifi cho ESP32

//-------------- Thư viện dùng để lấy thời gian --------------
#include <NTPClient.h>                          
#include <WiFiUdp.h>
#include <TimeLib.h>

//-------------- Khai báo biến đếm --------------
unsigned int cardCountHISTORY = 0;  // Biến đếm lịch sử thẻ RFID
unsigned int cardCount = 0;         // Biến đếm thẻ RFID
unsigned int d = 0;

//-------------- Định nghĩa mạng wifi --------------
#define WIFI_SSID "NAMLUN"                    // ID mạng
#define WIFI_PASSWORD "987987644"              // PW

//-------------- Định nghĩa firebase --------------
#define FIREBASE_HOST "https://rifid-ee24a-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "9Bv7UTyX5lcgGa5Aqky3eyWd86I477A2cSaznu3j"  

FirebaseData firebaseData;
FirebaseJson json;
FirebaseData fbdo;

//-------------- Định nghĩa PIN --------------
#define SERVO_PIN 12                          // chọn chân cho Servo
#define LED_PIN_GREEN 26                      // Chân điều khiển LED màu xanh
#define LED_PIN_RED 25                        // Chân điều khiển LED màu đỏ
#define LED_PIN_YELLOW 27                     // Chân điều khiển LED vàng
#define RST_PIN   0                         // Sử dụng chân GPIO5 cho Reset
#define SS_PIN    5                           // Sử dụng chân GPIO21 cho chip select
#define BUZZER_PIN 34                         // Chân điều khiển BUZZ

//-------------- Khởi tạo các đối tượng --------------
MFRC522 mfrc522(SS_PIN, RST_PIN);       // khởi tạo đối tượng MFRC522
Servo myservo;                          // Khởi tạo đối tượng Servo
WiFiUDP ntpUDP;                         // Khởi tạo đối tượng lấy thời gian
NTPClient timeClient(ntpUDP, "vn.pool.ntp.org", 7 * 3600); // 7 * 3600 = UTC +7 (giờ Việt Nam)

//-------------- Setup --------------
void setup() {
  Serial.begin(115200);                       // Khởi tạo Serial

  // Kết nối Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");

  // Đồng bộ thời gian từ máy chủ NTP
  timeClient.begin();

  while (!timeClient.update()) 
  {
    timeClient.forceUpdate();
  }  

  // Set default time zone for TimeLib
  setTime(timeClient.getEpochTime());
  
  // Khởi tạo kết nối RFID
  SPI.begin();                            // Khởi động giao tiếp SPI
  mfrc522.PCD_Init();                     // Khởi tạo mô-đun RFID

  // Khởi động kết nối Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  Firebase.reconnectWiFi(true);
  myservo.attach(SERVO_PIN);          // kết nối Servo với SERVO_PIN( chân 14)
  pinMode(LED_PIN_GREEN, OUTPUT);     // Đặt chân LED màu xanh là OUTPUT
  pinMode(LED_PIN_RED, OUTPUT);       // Đặt chân LED màu đỏ là OUTPUT
  pinMode(LED_PIN_YELLOW, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

//-------------- Hàm Time --------------
String Time()
{
  //-------------- Cập nhật thời gian -------------
  timeClient.update();

  //-------------- Lấy thời gian --------------
  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();

  // Xuất thời gian thành xâu
  String Time = String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond);  

  return Time;
}
String Date()
{
  //-------------- Cập nhật thời gian -------------
  timeClient.update();

  //-------------- Lấy thời gian --------------
  int currentYear = year();
  int currentMonth = month();
  int currentDay = day();

  // Xuất thời gian thành xâu
  String Date = String(currentDay) + "/" + String(currentMonth) + "/" + String(currentYear);

  return Date;
}


//-------------- Kiểm tra RFID trên cơ sở dữ liệu firebase --------------
bool checkCardValidity(const String& rfid)
{
  if (Firebase.getString(firebaseData, "/Card_Valid"))            // Kiểm tra dữ liệu trong thẻ "/Card_Valid"
  {
    String cardValid = firebaseData.stringData();
    
    if (cardValid.indexOf(rfid) != -1)
    {
      return true;
    }
  }
  return false;
}

void loop() 
{
  String TimeValue = Time();
  String DateValue = Date();
  //-------------- Mở cửa bằng Web --------------
  if(Firebase.getString(fbdo, "/Door/State")) 
  {
    Serial.println("Download success: " + String(fbdo.stringData()));
    { 
      if(fbdo.stringData() == "ON")
      {
        digitalWrite(LED_PIN_YELLOW, LOW);
        Serial.println("Access granted!");              // Mở cửa thành công
        myservo.write(90);                              // mở cửa với góc 90 độ
        digitalWrite(LED_PIN_GREEN, HIGH);      
        if (d == 0)
        {
          // Tạo đối tượng JSON và thêm dữ liệu        
          FirebaseJson json;
          json.add("rfid", "Web");
          json.add("Bol","True");
          json.add("Time", TimeValue);
          json.add("Date", DateValue);   

          // Tạo đường dẫn
          String pathWeb = "/Card/" + String(cardCountHISTORY);
          // Gửi đối tượng JSON vào danh sách trên Firebase
          Firebase.setJSON(firebaseData, pathWeb, json);
          if (firebaseData.dataAvailable()) 
          {
            Serial.println(firebaseData.httpCode());
            Serial.println(firebaseData.errorReason());
          }
          cardCountHISTORY++;  
          d = 1;  
        } 
      }
      else 
      {
        myservo.write(0);                               // đóng cửa
        digitalWrite(LED_PIN_GREEN, LOW);      
        d = 0;
        //-------------- Mở cửa bằng thẻ --------------  
        //-------------- Kiểm tra trạng thái --------------
        if(Firebase.getString(fbdo, "/ledState/State"))     
        //-------------- Trạng thái Mở cửa ---------------
        if(fbdo.stringData() == "OFF")
        {      
          digitalWrite(LED_PIN_YELLOW, LOW);
          //-------------- Kiểm tra sự hiện diện của thẻ RFID --------------
          if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
          {
            // Lấy thẻ RFID
            String rfid = "";
            for (byte i = 0; i < mfrc522.uid.size; i++) 
            {
              rfid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
              rfid.concat(String(mfrc522.uid.uidByte[i], HEX));
            }

            // in thẻ RFID
            Serial.println("Card ID: " + rfid);
            Serial.println("Time Value: " + TimeValue);
            Serial.println("Date Value: " + DateValue);
            
            //-------------- Dừng đọc --------------
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
          
            //-------------- Check RFID Tag --------------
            //-------------- RFID True --------------
            if (checkCardValidity(rfid))
            {
              //-------------- Thực thi lệnh mở cửa --------------
              Serial.println("Access granted!");              // In ra Monitor thành công
              myservo.write(90);                              // mở cửa
              digitalWrite(LED_PIN_GREEN, HIGH);              // LED_GREEN sáng báo hiệu đã mở cửa
              delay(5000);                                    // Đợi 5s
              myservo.write(0);                               // đóng cửa                   
              digitalWrite(LED_PIN_GREEN, LOW);               // LED_GREEN sáng báo hiệu đã đóng cửa

              //-------------- Tạo đối tượng JSON và thêm dữ liệu --------------        
              FirebaseJson json;
              json.add("Bol","True");
              json.add("Time", TimeValue);
              json.add("Date", DateValue);
              json.add("rfid", rfid);

              //-------------- Tạo đường dẫn --------------
              String path = "/Card/" + String(cardCountHISTORY);
              
              //-------------- Gửi đối tượng JSON vào danh sách trên Firebase --------------
              Firebase.setJSON(firebaseData, path, json);
              if (firebaseData.dataAvailable()) 
              {
                Serial.println(firebaseData.httpCode());
                Serial.println(firebaseData.errorReason());
              }
              //-------------- Tăng biến đếm lịch sử --------------
              cardCountHISTORY++;
            } 
            //-------------- RFID False --------------
            else
            {
              //-------------- Thực thi lệnh cảnh báo --------------
              Serial.println("Access denied!");              // In ra Monitor từ chối
              digitalWrite(LED_PIN_RED, HIGH);               // LED_RED sáng cảnh báo
              digitalWrite(BUZZER_PIN, HIGH);                // Mở còi cảnh báo
              delay(1000);                                   // Đợi 1s
              digitalWrite(LED_PIN_RED, LOW);              // LED_GREEN sáng báo hiệu đã đóng cửa
              digitalWrite(BUZZER_PIN, LOW);
              
              //-------------- Tạo đối tượng JSON và thêm dữ liệu --------------        
              FirebaseJson json;
              json.add("Bol","False");
              json.add("Time", TimeValue);
              json.add("Date", DateValue);
              json.add("rfid", rfid);

              //-------------- Tạo đường dẫn --------------
              String path = "/Card/" + String(cardCountHISTORY);
                
              //-------------- Gửi đối tượng JSON vào danh sách trên Firebase --------------
              Firebase.setJSON(firebaseData, path, json);
              if (firebaseData.dataAvailable()) 
              {
                Serial.println(firebaseData.httpCode());
                Serial.println(firebaseData.errorReason());
              }
              //-------------- Tăng biến đếm lịch sử --------------
              cardCountHISTORY++;
            }
          }
        }
        //-------------- ADD thẻ --------------
        else
        {
          digitalWrite(LED_PIN_YELLOW, HIGH);
          
          //-------------- Kiểm tra sự hiện diện của thẻ RFID --------------
          if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
          {
            // Lấy thẻ RFID
            String rfid = "";
            for (byte i = 0; i < mfrc522.uid.size; i++) 
            {
              rfid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
              rfid.concat(String(mfrc522.uid.uidByte[i], HEX));
            }

            // in thẻ RFID
            Serial.println("Card ID: " + rfid);
            Serial.println("Time Value: " + TimeValue);
            Serial.println("Date Value: " + DateValue);
              
            //-------------- Dừng đọc --------------
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
              
            //-------------- Tạo đường dẫn --------------
            String pathValid = "/Card_Valid/" + String(cardCount);

            //-------------- Gửi giá trị thẻ lên Firebase --------------
            Firebase.setString(firebaseData, pathValid, rfid);
            if (firebaseData.dataAvailable()) 
            {
              Serial.println(firebaseData.httpCode());
              Serial.println(firebaseData.errorReason());
            }

            //-------------- Tăng giá trị biến đếm --------------
            cardCount++;
            digitalWrite(LED_PIN_YELLOW, LOW);
            delay(1000);
            digitalWrite(LED_PIN_YELLOW, HIGH);
          }
        }    
      }
    }
  }
}
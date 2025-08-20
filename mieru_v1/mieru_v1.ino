#include <M5StickCPlus.h>
#include <WiFiClient.h>           // IFTTTを経由する場合必要（WiFiClientSecureと入れ替え）
#include <WiFi.h>                 // 共通で必要
#include <WiFiClientSecure.h>     // LINE Notify 単体の場合必要
#include <WiFiMulti.h>            // 複数のアクセスポイントを動的に切り替えるために必要

#define INPUT_PIN 33
#define PUMP_PIN 32

// WiFiClient client;
WiFiMulti wifiMulti;

bool flag = true;
bool alarm_flg = false;
int rawADC;
int lastWifiStatus = WL_DISCONNECTED;
int btn_pw = 0;
String makerEvent = "";
String makerKey = "";
const char* serverUrl = "maker.ifttt.com";
const char* msg[] = {"power_on...", "water_the_plants!"};

// LINE Notify設定
const char* host = "";
const char* token = "";

WiFiServer server(80);

// 再起動（リスタート）処理
void restart() {
  // 電源ボタン状態取得（1秒以下のONで「2」1秒以上で「1」すぐに「0」に戻る）
  btn_pw = M5.Axp.GetBtnPress();
  if (btn_pw == 2) {  // 電源ボタン短押し（1秒以下）なら
    ESP.restart();    // 再起動
  }
}

// LINE Notify
// void doGet(String value) {
//   Serial.println("start do get...");
//   if (!client.connect(serverUrl, 80)) {
//     Serial.println("Connection failed");
//   } else {
//     Serial.println("Connected to server");
//     // HTTP Requestを生成
//     String url = "/trigger/" + makerEvent + "/with/key/" + makerKey;
//     url += "?value1=" + value;
//     client.println("GET " + url + " HTTP/1.1");
//     client.print("Host: ");
//     client.println(serverUrl);
//     client.println("Connection: close");
//     client.println();
//     Serial.print("Waiting for response ");

//     int count = 0;
//     while (!client.available()) {
//       delay(50);
//       Serial.print(".");
//     }
//     while (client.available()) {
//       char c = client.read();
//       Serial.write(c);
//     }

//     if (!client.connected()) {
//       Serial.println();
//       Serial.println("disconnecting from server.");
//       client.stop();
//     }
//   }
// }

// LINE Notify 純正
void notify_line(const char* value) {
  // HTTPSへアクセス（SSL通信）するためのライブラリ
  WiFiClientSecure client;

  // サーバー証明書の検証を行わずに接続する場合に必要
  client.setInsecure();
  
  Serial.println("Try");
  
  //LineのAPIサーバにSSL接続（ポート443:https）
  if (!client.connect(host, 443)) {
    Serial.println("Connection failed");
    return;
  }
  Serial.println("Connected");

  // リクエスト送信
  String query = String("message=") + String(value);
  String request = String("") +
               "POST /api/notify HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + token + "\r\n" +
               "Content-Length: " + String(query.length()) +  "\r\n" + 
               "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                query + "\r\n";
  client.print(request);
 
  // 受信完了まで待機 
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readStringUntil('\n');
  Serial.println(line);  
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.print("mieru");

  pinMode(INPUT_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(10, OUTPUT);

  digitalWrite(10, 1);
  // WiFiアクセスポイントを追加（他に追加したいアクセスポイントがあったら、ここで追加する）
  wifiMulti.addAP("you62");

    // WiFi接続
  int wifiStatus = wifiMulti.run();

  if(wifiStatus != lastWifiStatus) {
    switch(wifiStatus) {
      case WL_NO_SHIELD:
      Serial.println("no shiled");
      break;
      case WL_IDLE_STATUS:
      Serial.println("idle");
      break;
      case WL_NO_SSID_AVAIL:
      Serial.println("no ssid available");
      break;
      case WL_SCAN_COMPLETED:
      Serial.println("scan completed");
      break;
      case WL_CONNECTED:
      Serial.println("connected");
      Serial.print("ssid:");
      Serial.println(WiFi.SSID());
      break;
      case WL_CONNECT_FAILED:
      Serial.println("connect failed");
      break;
      case  WL_CONNECTION_LOST:
      Serial.println("connection lost");
      break;
      case  WL_DISCONNECTED:
      Serial.println("disconnected");
      break;
    }
    lastWifiStatus = wifiStatus;
  }
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 70);
  M5.Lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP());
  Serial.println("WiFi Ready!");

  server.begin();
  notify_line(msg[0]);
  // doGet(msg[0]);
}

void loop() {
  rawADC = analogRead(INPUT_PIN);
  if (rawADC >= 1850 && !alarm_flg) {
    // doGet(msg[1]);
    notify_line(msg[1]);
    alarm_flg = true;
    digitalWrite(10, 0);
    M5.Beep.tone(2600);
    delay(500);
    M5.Beep.mute();
    // pump start
    digitalWrite(PUMP_PIN, 1);
    delay(1000);
    // flag = !flag;
    // pump stop
    digitalWrite(PUMP_PIN, 0);
  }
  M5.lcd.fillRect(80, 100, 240, 50, BLACK);
  M5.Lcd.setCursor(20, 100);
  M5.Lcd.print("water: " + String(rawADC));
  Serial.print("water: ");
  Serial.println(rawADC);
  if (M5.BtnA.wasPressed()) {
      digitalWrite(PUMP_PIN, 1);
      delay(1000);
      // flag = !flag;
      digitalWrite(PUMP_PIN, 0);
  }
  M5.update();
  delay(500);
}
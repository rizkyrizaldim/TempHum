#include "DHT.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AntaresESPHTTP.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define ACCESSKEY "720ed28c85b89432:5c144690ba160c11"
#define WIFISSID "DBT"
#define PASSWORD "telkom2021"

#define projectName "IoT_Sensor"
#define deviceName "TH1"

#define DHTPIN 4
#define DHTTYPE DHT11

const unsigned long interval = 10000; // 10 m interval
unsigned long previousMillis = 0;

AntaresESPHTTP antares(ACCESSKEY); // Create an object for interacting with Antares
DHT dht(DHTPIN, DHTTYPE);

#define BOTtoken "6342942223:AAHutz_2f2ql-kM2mm4s8K8lCNlB9yynXxs" // Bot Token dari BotFather
#define CHAT_ID "-4123163923"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

int h = 0;
float t = 0;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);

    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String control = "Selamat Datang, " + from_name + ".\n";
      control += "Gunakan Commands Di Bawah Untuk Monitoring DHT11\n\n";
      control += "/Temperature Untuk Monitoring Suhu \n";
      control += "/Humidity Untuk Monitoring Kelembapan \n";
      bot.sendMessage(chat_id, control, "");
    }
    if (text == "/Temperature") {
      int t = dht.readTemperature();
      if (isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
      t -= 1.5; // Kurangi 2.2 dari nilai suhu
      String suhu = "Status Suhu ";
      suhu += t;
      suhu += " ⁰C\n";
      bot.sendMessage(chat_id, suhu, "");
    }

    if (text == "/Humidity") {
      int h = dht.readHumidity();
      if (isnan(h)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
      String lembab = "Status Kelembapan ";
      lembab += h;
      lembab += " %Rh\n";
      bot.sendMessage(chat_id, lembab, "");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  dht.begin();

  antares.setDebug(true); // Turn on debugging. Set to "false" if you don't want messages to appear on the serial monitor
  antares.wifiConnection(WIFISSID, PASSWORD);

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Starting...");

  // Koneksi Ke Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFISSID, PASSWORD);
#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
#endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {

  if (millis() - previousMillis > interval) {
    previousMillis = millis();

    int hum = dht.readHumidity();      // Read the humidity value from the DHT11 sensor
    int temp = dht.readTemperature();  // Read the temperature value from the DHT11 sensor
    if (isnan(hum) || isnan(temp)) {     // Check if there's an error reading the sensor
      Serial.println("Failed to read DHT sensor!");
      return;
    }
    temp -= 1.5; // Kurangi 2.2 dari nilai suhu
    Serial.println("Temperature: " + (String)temp + " *C"); // Print the temperature in Celsius
    Serial.println("Humidity: " + (String)hum + " %");      // Print the humidity as a percentage

    // Display temperature on
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((SCREEN_WIDTH - 12 * 6) / 2,0); // Tengah layar
    display.println("Suhu = ");
    display.setTextSize(2); // Mengubah ukuran teks untuk suhu
    display.setCursor((SCREEN_WIDTH - 6 * 6) / 2, 16); // Tengah layar secara horizontal, letakkan sedikit lebih rendah
    display.println((String)temp + " C");
    display.display();


    // Add the temperature and humidity data to the storage buffer
    antares.add("temperature", temp);
    antares.add("humidity", hum);

    // Send the data from the storage buffer to Antares
    antares.send(projectName, deviceName);
    if (temp > 28) {
      // Send a message to Telegram
      String message = "Warning! Suhu Panas. Suhu saat ini: " + String((int)temp) + " °C";
      bot.sendMessage(CHAT_ID, message, "");
    }
  }

  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}

void sendSensor() {
  delay(2000);
  h = dht.readHumidity();
  t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.println("");
  Serial.print(F("Humidity : "));
  Serial.print(h);
  Serial.println(F("% "));
  Serial.print(F("Temperature : "));
  Serial.print(t);
  Serial.print(F("°C  "));

  delay(1000);
}

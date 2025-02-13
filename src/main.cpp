#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <string>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include "DHT12.h"
#include "bmm150.h"
#include "bmm150_defs.h"
#include "Adafruit_SHT4x.h"
#include <main.h>
#include <WebServer.h>

const char* WIFI_SSID = "TP-LINK_D8DD";
const char* WIFI_PASSWORD = "43331192";

WebServer server(80);

DHT12 dht12;
BMM150 bmm = BMM150();
bmm150_mag_data value_offset;
Adafruit_BMP280 bmp280;

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

sensors_event_t humidity, temp;

enum ScreenState { SCREEN1, SCREEN2, SCREEN3, SCREEN4, SCREEN5, SCREEN6, SCREEN7, SCREEN8, SCREEN9, SCREEN10, SCREEN11, SCREEN12 };
ScreenState currentScreenState = SCREEN1;
const int NUM_SCREEN_STATES = 12;

int vol;
int bat_level;

int workTime = 25 * 60;
int breakTime = 5 * 60;
int timer = workTime; 
bool isWorking = true;
bool isRunning = false;
int completedCycles = 0;
float pressure = 0;
bool btnA_pressed = false;
bool btnB_pressed = false;
bool btnC_pressed = false;

float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

M5Canvas canvas(&M5.Lcd);  // Создаем буфер для двойной буферизации

// Добавляем глобальные переменные для WiFi сканера
int networksFound = 0;
const int MAX_NETWORKS = 10;  // Максимальное количество отображаемых сетей
int currentScroll = 0;  // Для прокрутки списка сетей

// Добавляем структуру для хранения данных о качестве сигнала
struct SignalQuality {
    int rssi;
    int channel;
    int strength;  // 0-4
    String encryption;
    int noiseLevel;
};

SignalQuality currentSignal;

const int GRAPH_POINTS = 100;
float tempHistory[GRAPH_POINTS];
int tempIndex = 0;

void connectToWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        M5.Lcd.println("WiFi подключен");
        M5.Lcd.print("IP: ");
        M5.Lcd.println(WiFi.localIP());
    } else {
        M5.Lcd.println("Ошибка подключения WiFi");
    }
    delay(2000);
}


void read_sensors() {
    sht4.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    vol = StickCP2.Power.getBatteryVoltage();
    bat_level = StickCP2.Power.getBatteryLevel();
    pressure = bmp280.readPressure();
    
    StickCP2.Imu.getAccelData(&accX, &accY, &accZ);
}

void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        // Устанавливаем кодировку UTF-8 для корректного отображения русских символов
        server.sendHeader("Content-Type","text/html; charset=UTF-8");
        String html = "<html><body>";
        html += "<h1>Показания датчиков</h1>";
        html += "<p>Температура: " + String(temp.temperature-5) + " °C</p>";
        html += "<p>Влажность: " + String(humidity.relative_humidity) + " %</p>";
        html += "<p>Давление: " + String(pressure/100.0) + " hPa</p>";
        html += "<p>Заряд батареи: " + String(bat_level) + " %</p>";
        html += "<p>Напряжение: " + String(vol/1000.0) + " V</p>";
        html += "<script>setTimeout(function(){window.location.reload();}, 5000);</script>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    });

    server.begin();
}

void handleWebServer() {
    server.handleClient();
}

void toggleTimer() {
  isRunning = !isRunning;
}

void updateScreen() {
  clearScreen();
  
  switch (currentScreenState) {
    case SCREEN1:
      drawScreen1();
      break;
    case SCREEN2:
      drawScreen2();
      break;
    case SCREEN3:
      drawScreen3();
      break;
    case SCREEN4:
      drawScreen4();
      break;
    case SCREEN5:
      drawScreen5();
      break;
    case SCREEN6:
      drawScreen6();
      break;
    case SCREEN7:
      drawScreen7();
      break;
    case SCREEN8:
      drawScreen8();
      break;
    case SCREEN9:
      drawScreen9();
      break;
    case SCREEN10:
      drawScreen10();
      break;
    case SCREEN11:
      drawScreen11();
      break;
    case SCREEN12:
      drawScreen12();
      break;
  }
}

void clearScreen() { 
    canvas.fillScreen(BLACK);
}

void resetTimer() {
  isRunning = false;
  timer = isWorking ? workTime : breakTime;
  displayTime();
}

void adjustTime(bool increase) {
  int adjustment = 60;
  if (isWorking) {
    workTime = max(60, workTime + (increase ? adjustment : -adjustment));
    timer = workTime;
  } else {
    breakTime = max(60, breakTime + (increase ? adjustment : -adjustment));
    timer = breakTime;
  }
  displayTime();
}

void changeScreenState() {
  currentScreenState = static_cast<ScreenState>((currentScreenState + 1) % NUM_SCREEN_STATES);
  updateScreen();
}

void changeScreenStateBack() {
  currentScreenState = static_cast<ScreenState>((currentScreenState - 1 + NUM_SCREEN_STATES) % NUM_SCREEN_STATES);
  updateScreen();
}

void initialize_sht40() {
    Serial.println("Adafruit SHT4x test");
    if (! sht4.begin(&Wire)) {
        Serial.println("Could not inititalize SHT4x");
    }
}

boolean checkButtons() {
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button A pressed");
    btnA_pressed = true;
  } else {
    btnA_pressed = false;
  }
  if (M5.BtnB.wasPressed()) {
    Serial.println("Button B pressed");
    btnB_pressed = true;
  } else {
    btnB_pressed = false;
  }
  if (M5.BtnC.wasPressed()) {
    Serial.println("Button C pressed");
    btnC_pressed = true;
  } else {
    btnC_pressed = false;
  }

  return false;
}

void displayTime() {
  Serial.println("display time");
  int minutes = timer / 60;
  int seconds = timer % 60;

  int maxTime = isWorking ? workTime : breakTime;

  canvas.fillScreen(isWorking ? RED : GREEN);

  // M5.Lcd.setTextColor(WHITE, isWorking ? RED : GREEN);
  canvas.setTextSize(2);
  canvas.setCursor(10, 20);
  canvas.printf("Mode: %s", isWorking ? "Work" : "Pause");

  canvas.setCursor(10, 60);
  canvas.printf("Time: %02d:%02d", minutes, seconds);

  canvas.setCursor(10, 100);
  canvas.printf("Cycles: %d", completedCycles);
}

void drawScreen1() {
    Serial.println("Draw screen 1");
    read_sensors();
  
    uint32_t timestamp = millis();
    timestamp = millis() - timestamp;

    Serial.print("Temperature: "); Serial.print(temp.temperature-5); Serial.println(" degrees C");
    Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

    canvas.setTextSize(1);
    canvas.setCursor(0, 20, 2);
    canvas.printf("Temp: %2.1f Humi: %2.0f%%", temp.temperature-5.0, humidity.relative_humidity);
    canvas.setCursor(0, 40, 2);
    canvas.printf("pressure: %2.1f", pressure);

    canvas.setCursor(0, 60, 2);
    canvas.printf("IP: %s", WiFi.localIP().toString().c_str());

    canvas.setCursor(0, 80, 2);
    canvas.printf("AccX:%5.2f Y:%5.2f Z:%5.2f", accX, accY, accZ);

    // Draw battery level bar
    int barWidth = 120;  // Ширина полного бара
    int barHeight = 10;  // Высота бара
    int barX = 60;       // Позиция X 
    int barY = 0;      // Позиция Y

    // Рамка бара
    canvas.drawRect(barX, barY, barWidth, barHeight, WHITE);
    
    // Заполненная часть бара в зависимости от уровня заряда
    int fillWidth = (bat_level * barWidth) / 100;
    canvas.fillRect(barX, barY, fillWidth, barHeight, GREEN);
    // Отображение числового значения уровня заряда
    canvas.setCursor(barX + barWidth + 5, barY + 2);
    canvas.printf("%d%%", bat_level);
}

void drawScreen2() {
    Serial.println("Draw screen 2");

    if (btnA_pressed) {
        toggleTimer();
    }

    if (isRunning) {
        delay(1000);
        timer--;

        if (timer <= 0) {
            isWorking = !isWorking; 
            timer = isWorking ? workTime : breakTime;

            if (!isWorking) {
                completedCycles++;
            }

            M5.Speaker.tone(1000, 200);
            delay(300);
            M5.Speaker.tone(1000, 200);
        }
    }
    displayTime();
}

void drawScreen3() {
    // Рисуем звездное небо
    for (int i = 0; i < 50; i++) {
        int x = random(0, M5.Lcd.width());
        int y = random(0, M5.Lcd.height());
        canvas.drawPixel(x, y, WHITE);
    }
    
    // Рисуем луну
    canvas.fillCircle(180, 30, 20, YELLOW);
    canvas.fillCircle(170, 25, 20, BLACK);
    
    // Рисуем горы
    for (int x = 0; x < M5.Lcd.width(); x += 40) {
        int height = random(30, 60);
        canvas.fillTriangle(x, M5.Lcd.height(), 
                           x + 20, M5.Lcd.height() - height,
                           x + 40, M5.Lcd.height(),
                           BLUE);
    }
    
    // Рисуем дерево
    canvas.fillRect(30, 70, 10, 30, 0x8B4513);
    canvas.fillTriangle(20, 70, 35, 40, 50, 70, GREEN);
    canvas.fillTriangle(20, 55, 35, 25, 50, 55, GREEN);

}

void drawScreen4() {
    
    // Рисуем аналоговые часы
    int centerX = M5.Lcd.width() / 2;
    int centerY = M5.Lcd.height() / 2;
    int radius = min(centerX, centerY) - 10;
    
    // Рисуем циферблат
    canvas.drawCircle(centerX, centerY, radius, WHITE);
    
    // Рисуем метки часов
    for (int i = 0; i < 12; i++) {
        float angle = i * 30 * PI / 180;
        int x1 = centerX + (radius - 5) * sin(angle);
        int y1 = centerY - (radius - 5) * cos(angle);
        int x2 = centerX + radius * sin(angle);
        int y2 = centerY - radius * cos(angle);
        canvas.drawLine(x1, y1, x2, y2, WHITE);
    }
    
    // Получаем текущее время
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Рисуем стрелки
    // Часовая стрелка
    float hourAngle = (timeinfo->tm_hour % 12 + timeinfo->tm_min / 60.0) * 30 * PI / 180;
    int hourX = centerX + (radius * 0.5) * sin(hourAngle);
    int hourY = centerY - (radius * 0.5) * cos(hourAngle);
    canvas.drawLine(centerX, centerY, hourX, hourY, RED);
    
    // Минутная стрелка
    float minAngle = timeinfo->tm_min * 6 * PI / 180;
    int minX = centerX + (radius * 0.7) * sin(minAngle);
    int minY = centerY - (radius * 0.7) * cos(minAngle);
    canvas.drawLine(centerX, centerY, minX, minY, GREEN);
    
    // Секундная стрелка
    float secAngle = timeinfo->tm_sec * 6 * PI / 180;
    int secX = centerX + (radius * 0.9) * sin(secAngle);
    int secY = centerY - (radius * 0.9) * cos(secAngle);
    canvas.drawLine(centerX, centerY, secX, secY, BLUE);
}

void drawScreen5() {
    // Получаем текущее время
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Форматируем время в строку
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    // Очищаем предыдущее значение
    canvas.setTextSize(3);
    canvas.setTextColor(WHITE, BLACK);
    
    // Центрируем текст
    int textWidth = canvas.textWidth(timeStr);
    int x = (M5.Lcd.width() - textWidth) / 2;
    int y = M5.Lcd.height() / 2 - 12;
    
    // Выводим время
    canvas.drawString(timeStr, x, y);
    
    // Выводим дату под часами
    char dateStr[11];
    sprintf(dateStr, "%02d.%02d.%04d", 
            timeinfo->tm_mday,
            timeinfo->tm_mon + 1,
            timeinfo->tm_year + 1900);
            
    canvas.setTextSize(2);
    textWidth = canvas.textWidth(dateStr);
    x = (M5.Lcd.width() - textWidth) / 2;
    y += 40;
    
    canvas.drawString(dateStr, x, y);
}

void drawNixieDigit(int x, int y, int digit) {
    const int digitWidth = 20;
    const int digitHeight = 35;
    
    // Фоновое свечение
    canvas.fillRect(x, y, digitWidth, digitHeight, 0x4A69);
    
    // Рисуем "стеклянную трубку"
    canvas.drawRect(x, y, digitWidth, digitHeight, 0x8410);
    
    // Рисуем цифру с оранжевым свечением
    canvas.setTextSize(3);
    canvas.setTextColor(ORANGE);
    canvas.setCursor(x + 3, y + 5);
    canvas.printf("%d", digit);
    
    // Добавляем эффект свечения
    canvas.drawRect(x + 1, y + 1, digitWidth - 2, digitHeight - 2, 0xFD20);
}

void drawScreen6() {
    // Получаем текущее время
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    int hours = timeinfo->tm_hour;
    int minutes = timeinfo->tm_min;
    int seconds = timeinfo->tm_sec;
    
    // Позиции для цифр
    const int startX = 20;
    const int startY = 30;
    const int spacing = 25;
    
    // Часы
    drawNixieDigit(startX, startY, hours / 10);
    drawNixieDigit(startX + spacing, startY, hours % 10);
    
    // Разделитель
    canvas.fillCircle(startX + spacing * 2, startY + 10, 2, ORANGE);
    canvas.fillCircle(startX + spacing * 2, startY + 25, 2, ORANGE);
    
    // Минуты
    drawNixieDigit(startX + spacing * 2.5, startY, minutes / 10);
    drawNixieDigit(startX + spacing * 3.5, startY, minutes % 10);
    
    // Разделитель
    canvas.fillCircle(startX + spacing * 4.5, startY + 10, 2, ORANGE);
    canvas.fillCircle(startX + spacing * 4.5, startY + 25, 2, ORANGE);
    
    // Секунды
    drawNixieDigit(startX + spacing * 5, startY, seconds / 10);
    drawNixieDigit(startX + spacing * 6, startY, seconds % 10);
}

void drawScreen7() {
    static unsigned long lastScanTime = 0;
    const unsigned long SCAN_INTERVAL = 10000;  // Интервал обновления списка сетей (10 секунд)
    
    // Обновляем список сетей каждые 10 секунд
    if (millis() - lastScanTime > SCAN_INTERVAL) {
        networksFound = WiFi.scanNetworks();
        lastScanTime = millis();
    }
    
    canvas.setTextSize(1);
    canvas.setTextColor(WHITE, BLACK);
    canvas.setCursor(0, 0);
    canvas.printf("Found %d networks", networksFound);
    
    // Отображаем список сетей
    int y = 15;
    int displayLimit = min(networksFound, MAX_NETWORKS);
    
    for (int i = currentScroll; i < currentScroll + displayLimit && i < networksFound; i++) {
        // Определяем цвет в зависимости от уровня сигнала
        uint16_t signalColor;
        int rssi = WiFi.RSSI(i);
        if (rssi > -50) signalColor = GREEN;
        else if (rssi > -70) signalColor = YELLOW;
        else signalColor = RED;
        
        // Рисуем индикатор уровня сигнала
        int signalStrength = map(rssi, -100, -40, 1, 4);
        for (int j = 0; j < 4; j++) {
            if (j < signalStrength) {
                canvas.fillRect(2 + j*4, y + 2, 3, 8 - j*2, signalColor);
            } else {
                canvas.drawRect(2 + j*4, y + 2, 3, 8 - j*2, DARKGREY);
            }
        }
        
        // Выводим информацию о сети
        canvas.setTextColor(WHITE);
        canvas.setCursor(20, y);
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 15) {
            ssid = ssid.substring(0, 12) + "...";
        }
        canvas.printf("%s", ssid.c_str());
        
        canvas.setCursor(120, y);
        canvas.printf("%ddBm", rssi);
        
        // Иконка защиты
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
            canvas.drawPixel(115, y + 4, WHITE);
            canvas.drawPixel(116, y + 4, WHITE);
            canvas.fillRect(114, y + 5, 4, 3, WHITE);
        }
        
        y += 12;
    }
    
    // Управление прокруткой
    if (btnA_pressed && currentScroll > 0) {
        currentScroll--;
    }
    if (btnC_pressed && currentScroll < networksFound - MAX_NETWORKS) {
        currentScroll++;
    }
    
    // Индикатор прокрутки
    if (networksFound > MAX_NETWORKS) {
        int scrollBarHeight = M5.Lcd.height() - 15;
        int thumbHeight = scrollBarHeight * MAX_NETWORKS / networksFound;
        int thumbY = 15 + (scrollBarHeight - thumbHeight) * currentScroll / (networksFound - MAX_NETWORKS);
        canvas.drawRect(M5.Lcd.width() - 4, 15, 4, scrollBarHeight, DARKGREY);
        canvas.fillRect(M5.Lcd.width() - 4, thumbY, 4, thumbHeight, WHITE);
    }
}

String getEncryptionType(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN:
            return "None";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA+WPA2";
        default:
            return "Unknown";
    }
}

void drawSignalStrengthMeter(int x, int y, int strength, uint16_t color) {
    const int barWidth = 4;
    const int barGap = 2;
    const int barMaxHeight = 20;
    
    for (int i = 0; i < 4; i++) {
        int barHeight = (i + 1) * 5;
        if (i < strength) {
            canvas.fillRect(x + i * (barWidth + barGap), y + (barMaxHeight - barHeight), 
                          barWidth, barHeight, color);
        } else {
            canvas.drawRect(x + i * (barWidth + barGap), y + (barMaxHeight - barHeight), 
                          barWidth, barHeight, DARKGREY);
        }
    }
}

void drawScreen8() {
    canvas.setTextSize(1);
    
    // Обновляем данные о текущем подключении
    if (WiFi.status() == WL_CONNECTED) {
        currentSignal.rssi = WiFi.RSSI();
        currentSignal.channel = WiFi.channel();
        currentSignal.strength = map(currentSignal.rssi, -100, -40, 0, 4);
        currentSignal.noiseLevel = random(-95, -85); // Примерное значение шума
        
        // Заголовок
        canvas.setTextColor(GREEN);
        canvas.setCursor(0, 0);
        canvas.printf("Connected to: %s", WiFi.SSID().c_str());
        
        // Основная информация
        canvas.setTextColor(WHITE);
        
        // Сигнал
        canvas.setCursor(0, 15);
        canvas.print("Signal Strength:");
        drawSignalStrengthMeter(100, 15, currentSignal.strength, 
                               currentSignal.strength > 2 ? GREEN : 
                               currentSignal.strength > 1 ? YELLOW : RED);
        
        // RSSI
        canvas.setCursor(0, 35);
        canvas.printf("RSSI: %d dBm", currentSignal.rssi);
        
        // Канал
        canvas.setCursor(0, 45);
        canvas.printf("Channel: %d", currentSignal.channel);
        
        // Шум
        canvas.setCursor(0, 55);
        canvas.printf("Noise: %d dBm", currentSignal.noiseLevel);
        
        // SNR (Signal-to-Noise Ratio)
        int snr = currentSignal.rssi - currentSignal.noiseLevel;
        canvas.setCursor(0, 65);
        canvas.printf("SNR: %d dB", snr);
        
        // Качество соединения
        canvas.setCursor(0, 75);
        canvas.print("Quality: ");
        if (snr > 40) {
            canvas.setTextColor(GREEN);
            canvas.print("Excellent");
        } else if (snr > 25) {
            canvas.setTextColor(GREEN);
            canvas.print("Good");
        } else if (snr > 15) {
            canvas.setTextColor(YELLOW);
            canvas.print("Fair");
        } else {
            canvas.setTextColor(RED);
            canvas.print("Poor");
        }
        
        // IP адрес
        canvas.setCursor(0, 95);
        canvas.printf("IP: %s", WiFi.localIP().toString().c_str());
        
    } else {
        canvas.setTextColor(RED);
        canvas.setCursor(0, M5.Lcd.height()/2 - 10);
        canvas.print("Not connected to WiFi");
    }
}

void drawScreen9() {
    canvas.setTextSize(1);
    
    // CPU информация
    canvas.setCursor(0, 0);
    canvas.printf("CPU Freq: %d MHz", ESP.getCpuFreqMHz());
    
    // Память
    canvas.setCursor(0, 15);
    canvas.printf("Free RAM: %d KB", ESP.getFreeHeap() / 1024);
    canvas.setCursor(0, 25);
    canvas.printf("Total RAM: %d KB", ESP.getHeapSize() / 1024);
    
    // Flash память
    canvas.setCursor(0, 40);
    canvas.printf("Flash Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
    
    // Время работы
    unsigned long uptime = millis() / 1000;
    int days = uptime / (24 * 3600);
    int hours = (uptime % (24 * 3600)) / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;
    
    canvas.setCursor(0, 55);
    canvas.printf("Uptime: %dd %02d:%02d:%02d", days, hours, minutes, seconds);
}

void drawScreen10() {
    // Добавляем новую точку
    tempHistory[tempIndex] = temp.temperature - 5;
    tempIndex = (tempIndex + 1) % GRAPH_POINTS;
    
    // Находим min и max для масштабирования
    float minTemp = tempHistory[0];
    float maxTemp = tempHistory[0];
    for(int i = 1; i < GRAPH_POINTS; i++) {
        if(tempHistory[i] < minTemp) minTemp = tempHistory[i];
        if(tempHistory[i] > maxTemp) maxTemp = tempHistory[i];
    }
    
    // Отрисовка графика
    const int graphHeight = 100;
    const int graphWidth = M5.Lcd.width() - 20;
    const int graphY = 120;
    
    for(int i = 0; i < GRAPH_POINTS-1; i++) {
        int x1 = 10 + (i * graphWidth / GRAPH_POINTS);
        int x2 = 10 + ((i+1) * graphWidth / GRAPH_POINTS);
        int y1 = graphY - (tempHistory[i] - minTemp) * graphHeight / (maxTemp - minTemp);
        int y2 = graphY - (tempHistory[(i+1)] - minTemp) * graphHeight / (maxTemp - minTemp);
        
        canvas.drawLine(x1, y1, x2, y2, WHITE);
    }
    
    // Текущая температура крупно
    canvas.setTextSize(2);
    canvas.setCursor(0, 10);
    canvas.printf("%.1f C", temp.temperature - 5);
}

void drawScreen11() {
    canvas.setTextSize(1);
    
    // Центр экрана
    int centerX = M5.Lcd.width() / 2;
    int centerY = M5.Lcd.height() / 2;
    int radius = min(centerX, centerY) - 20;
    
    // Рисуем круг компаса
    canvas.drawCircle(centerX, centerY, radius, WHITE);
    
    // Рисуем стрелку на основе акселерометра
    float angle = atan2(accY, accX);
    int arrowX = centerX + cos(angle) * radius;
    int arrowY = centerY + sin(angle) * radius;
    
    canvas.drawLine(centerX, centerY, arrowX, arrowY, RED);
    
    // Выводим значения ускорения
    canvas.setCursor(0, 0);
    canvas.printf("X: %.2f", accX);
    canvas.setCursor(0, 10);
    canvas.printf("Y: %.2f", accY);
    canvas.setCursor(0, 20);
    canvas.printf("Z: %.2f", accZ);
}

void drawScreen12() {
    canvas.setTextSize(1);
    canvas.setCursor(0, 0);
    canvas.println("Scan to connect:");
    
    // Генерируем URL для QR-кода
    String url = "http://" + WiFi.localIP().toString();
    
    // Создаем QR код
    const int qrSize = min(M5.Lcd.width(), M5.Lcd.height()) - 40;
    const int qrX = (M5.Lcd.width() - qrSize) / 2;
    const int qrY = 20;

    // Рисуем QR код
    canvas.qrcode(url, qrX, qrY, qrSize, 2);
    
    canvas.setCursor(0, M5.Lcd.height() - 20);
    canvas.println(url);
}

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);

    Wire.begin(0, 26);
    Serial.begin(115200);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setRotation(3);
    
    // Инициализация буфера
    canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
    canvas.setTextSize(2);

    initialize_sht40();

    Serial.println("Initialize done!");

    connectToWiFi();

    setupWebServer();

    drawScreen1();
}

void loop() {
  M5.update();
  handleWebServer();

  // Синхронизация времени с NTP каждые 12 часов
  static unsigned long lastNTPSync = 0;
  if (WiFi.status() == WL_CONNECTED && (lastNTPSync == 0 || millis() - lastNTPSync > 12 * 60 * 60 * 1000)) {
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    
    Serial.println("Ожидание синхронизации времени...");
    while (time(nullptr) < 24 * 3600) {
      delay(100);
    }
    
    lastNTPSync = millis();
    Serial.println("Время синхронизировано");
  }

  
  checkButtons();
  if (btnB_pressed) {
    changeScreenState();
  }
  if (btnC_pressed) {
    changeScreenStateBack();
  }
  
  static unsigned long lastActivityTime = millis();
  static bool screenOn = true;

  // Проверяем нажатие любой кнопки
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
    lastActivityTime = millis();
    if (!screenOn) {
      M5.Lcd.wakeup();
      screenOn = true;
    }
  }

  // Проверяем время бездействия
  if (screenOn && (millis() - lastActivityTime > 15000)) {
    M5.Lcd.sleep();
    screenOn = false;
  }

  if (screenOn) {
    updateScreen();
    canvas.pushSprite(0, 0);  // Отображаем буфер на экране
  }


  delay(50);
}

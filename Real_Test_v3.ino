#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdarg.h> // Для работы с переменным числом аргументов

// === ПИНЫ ===
#define BUTTON_PIN D5       // Кнопка 1 — основное нажатие
#define SELECT_BUTTON D6    // Кнопка 2 — выбор таймера / тест мелодии
#define BUZZER_PIN D8       // Пьезо-модуль
#define SECOND_BUZZER_PIN D7 // Второй динамик
#define VOLTAGE_PIN A0      // Пин для измерения напряжения

// === ЭКРАН ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
unsigned long lastPressTime = 0;
unsigned long pressIntervalSum = 0;
unsigned int pressCount = 0;
bool buttonPressed = false;

// Настройки таймера
const unsigned long timerOptions[] = { 60 * 60000UL, 30 * 60000UL, 10 * 60000UL };
byte selectedTimerIndex = 2; // По умолчанию — 10 минут
unsigned long targetAlarmTime = 0;
bool alarmTriggered = false;

// Для кнопки выбора
unsigned long selectButtonPressedTime = 0;
bool selectButtonPressed = false;

// Прокрутка заголовка
int scrollPos = 0;

// Заголовок программы
String programTitleUTF8 = "Тест. Как я попал в эту локацию?";

// === ДЕБАУНС для кнопок ===
unsigned long buttonDebounceDelay = 50; // мс
unsigned long lastButtonPress = 0;
unsigned long lastSelectButtonPress = 0;

// === ИЗМЕРЕНИЕ НАПРЯЖЕНИЯ ===
float batteryVoltage = 0.0f;
unsigned long lastVoltageReadTime = 0;
const long voltageReadInterval = 60000; // раз в 60 секунд

// === ФУНКЦИЯ КОНВЕРТАЦИИ UTF-8 -> CP1251 для String ===
String utf8rus(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };
  k = source.length(); i = 0;
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}

// === Универсальная функция вывода строк на русском языке ===
void printFormatted(int x, int y, const char* format, ...) {
  char buffer[128];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  String output = utf8rus(String(buffer));
  display.setCursor(x, y);
  display.println(output);
}

// === МЕЛОДИЯ (упрощённая версия) ===
#include "melody.h"

// === ЧТЕНИЕ НАПРЯЖЕНИЯ ===
void readBatteryVoltage() {
    int sensorValue = analogRead(VOLTAGE_PIN); // Считываем значение с A0
    //batteryVoltage = (sensorValue / 1023.0) * 5.0; // Применяем формулу для 5V
    batteryVoltage = (sensorValue / 1023.0) * 3.3 * 13; // Учитываем делитель напряжения
    // Если используете делитель напряжения с R1 = R2 = 10k, делим на 2
    //batteryVoltage *= 2; // Увеличиваем в 2 раза
}

void setup() {
  //Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SELECT_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SECOND_BUZZER_PIN, OUTPUT); // Инициализация второго динамика

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    //Serial.println("SSD1306 allocation failed");
    for (;;); // Остановить программу
  }

  display.cp437(true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(30, 20);
  display.println(utf8rus("Привет МИР!\n\n\n\nверсия 3.0"));  
  display.display();
  delay(2000);

  targetAlarmTime = millis() + timerOptions[selectedTimerIndex];

  // Инициализация начальных значений
  lastPressTime = 0;
  pressIntervalSum = 0;
  pressCount = 0;

  // Первое измерение напряжения
  readBatteryVoltage();
  lastVoltageReadTime = millis();
}

void loop() {
  checkMainButton();
  checkSelectButton();
  checkAlarm();

  // Периодическое измерение напряжения
  if (millis() - lastVoltageReadTime > voltageReadInterval) {
    readBatteryVoltage();
    lastVoltageReadTime = millis();
  }

  updateDisplay();
  delay(100);
}

void checkMainButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState == LOW && !buttonPressed) {
    unsigned long now = millis();
    if (now - lastButtonPress > buttonDebounceDelay) {
      // Звуковой сигнал на обоих динамиках
      tone(BUZZER_PIN, 1000, 50);
      tone(SECOND_BUZZER_PIN, 1000, 50);

      if (lastPressTime != 0) {
        unsigned long interval = now - lastPressTime;
        pressIntervalSum += interval;
        pressCount++;
      }

      lastPressTime = now;
      buttonPressed = true;
      lastButtonPress = now;
      selectButtonPressedTime = now; // Засекаем время для проверки длительного удержания
    }
  }

  if (currentState == HIGH && buttonPressed) {
    unsigned long pressDuration = millis() - selectButtonPressedTime;
    buttonPressed = false;

    // Долгое нажатие (2 сек) - переключаем таймер и играем мелодию
    if (pressDuration >= 2000) {
      selectedTimerIndex = (selectedTimerIndex + 1) % 3;
      targetAlarmTime = millis() + timerOptions[selectedTimerIndex];
      alarmTriggered = false;
      playMelody();
    }
  }
}

void checkSelectButton() {
  bool state = digitalRead(SELECT_BUTTON);

  if (state == LOW && !selectButtonPressed) {
    unsigned long now = millis();
    if (now - lastSelectButtonPress > buttonDebounceDelay) {
      selectButtonPressedTime = now;
      selectButtonPressed = true;
      lastSelectButtonPress = now;
    }
  }

  if (state == HIGH && selectButtonPressed) {
    unsigned long pressDuration = millis() - selectButtonPressedTime;
    selectButtonPressed = false;

    if (pressDuration >= 2000) {
      playMelody(); // Долгое нажатие — тест мелодии
    } else {
      selectedTimerIndex = (selectedTimerIndex + 1) % 3;
      targetAlarmTime = millis() + timerOptions[selectedTimerIndex];
      alarmTriggered = false;
    }
  }
}

void checkAlarm() {
  if (!alarmTriggered && millis() >= targetAlarmTime) {
    playMelody(); // Проигрываем мелодию
    alarmTriggered = true;

    // Перезапуск таймера с тем же интервалом
    targetAlarmTime = millis() + timerOptions[selectedTimerIndex];
  }

  // Сбрасываем флаг для нового цикла
  if (millis() < targetAlarmTime) {
    alarmTriggered = false;
  }
}

void updateDisplay() {
  display.clearDisplay();

  // === Анимация прокрутки заголовка (туда-сюда) ===
  static int scrollPos = 0;
  static int scrollDirection = 1; // +1 — вправо, -1 — влево

  String title = utf8rus(programTitleUTF8);
  int titleLength = title.length();
  int charWidth = 6;
  int maxScroll = titleLength * charWidth - SCREEN_WIDTH;

  scrollPos += scrollDirection;

  if (scrollPos >= maxScroll) {
    scrollDirection = -1;
  } else if (scrollPos <= 0) {
    scrollDirection = 1;
  }

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(-scrollPos, 0);
  display.print(title);

  // Разделительная линия
  display.drawLine(0, 16, 127, 16, WHITE);

  // Счетчик нажатий + напряжение
  int actualPresses = pressCount + (lastPressTime != 0 ? 1 : 0);
  printFormatted(0, 20, "Нажатия: %d  (%.1fв)", actualPresses, batteryVoltage);

  // Средний интервал
  if (pressCount > 0) {
    float avgIntervalMs = (float)pressIntervalSum / (pressCount);
    int secondsTotal = (int)(avgIntervalMs / 1000.0f + 0.5f); // мс -> сек, округление

    int hours = secondsTotal / 3600;
    int minutes = (secondsTotal % 3600) / 60;
    int seconds = secondsTotal % 60;

    printFormatted(0, 32, "Интервал: %02d:%02d:%02d", hours, minutes, seconds);
  } else {
    printFormatted(0, 32, "Интервал: --:--:--");
  }

  // Время с последнего нажатия
  unsigned long timeSinceLast = millis() - lastPressTime;
  int secondsSince = timeSinceLast / 1000;
  int hours = secondsSince / 3600;
  int minutes = (secondsSince % 3600) / 60;
  int secs = secondsSince % 60;
  printFormatted(0, 44, "Нажали: %02d:%02d:%02d", hours, minutes, secs);

  // Таймер с временем
  unsigned long timeLeft = targetAlarmTime - millis();
  int secondsLeft = timeLeft / 1000;
  int minutesLeft = secondsLeft / 60;
  secondsLeft %= 60;

  const char* timerText[] = {"Таймер: 60 мин", "Таймер: 30 мин", "Таймер: 10 мин"};
  printFormatted(0, 56, "%s(%02d:%02d)", timerText[selectedTimerIndex], minutesLeft, secondsLeft);

  display.display();
}

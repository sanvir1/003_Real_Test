## Описание программы "Тест реальности" для Lolin 8266 Arduino

### Общее описание
Программа "Тест реальности" предназначена для измерения времени между нажатиями кнопки и ведения лога этих событий. Она использует две кнопки: первая для записи времени нажатий, вторая для очистки логов.

### Функциональность
1. **Кнопка 1 (Запись времени)**:
   - При каждом нажатии кнопки программа фиксирует текущее время.
   - Вычисляет:
     - Среднее время между всеми нажатиями.
     - Время между последним и предыдущим нажатиями.
   - Записывает данные в лог, включая:
     - Время нажатия.
     - Среднее время.
     - Время между последними нажатиями.

2. **Кнопка 2 (Очистка логов)**:
   - При нажатии на кнопку 2 все записи в логе удаляются.

### Структура программы

```cpp
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>

const int button1Pin = D1; // Кнопка 1
const int button2Pin = D2; // Кнопка 2

unsigned long lastPressTime = 0;
unsigned long currentPressTime = 0;
unsigned long totalPressTime = 0;
int pressCount = 0;

void setup() {
    Serial.begin(115200);
    pinMode(button1Pin, INPUT_PULLUP);
    pinMode(button2Pin, INPUT_PULLUP);
}

void loop() {
    if (digitalRead(button1Pin) == LOW) {
        currentPressTime = millis();
        if (pressCount > 0) {
            unsigned long interval = currentPressTime - lastPressTime;
            totalPressTime += interval;
            Serial.print("Время между последним и предыдущим нажатием: ");
            Serial.println(interval);
        }
        lastPressTime = currentPressTime;
        pressCount++;
        
        float averageTime = (pressCount > 1) ? (totalPressTime / (pressCount - 1)) : 0;
        Serial.print("Среднее время между нажатиями: ");
        Serial.println(averageTime);
        Serial.print("Время нажатия: ");
        Serial.println(currentPressTime);

        delay(500); // Задержка для предотвращения дребезга
    }

    if (digitalRead(button2Pin) == LOW) {
        Serial.println("Логи очищены.");
        totalPressTime = 0;
        pressCount = 0;
        delay(500); // Задержка для предотвращения дребезга
    }
}
```

### Объяснение кода
- **Инициализация**: Настройка пинов для кнопок и начальная инициализация переменных.
- **Основной цикл**:
  - Проверка состояния кнопки 1: если нажата, фиксируется время, вычисляются интервалы и среднее время.
  - Проверка состояния кнопки 2: если нажата, очищаются все логи.

### Заключение
Программа "Тест реальности" является простым и эффективным инструментом для измерения времени между событиями. Возможность очистки логов делает её удобной для многократного использования.

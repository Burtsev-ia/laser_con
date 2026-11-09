const int lcdPin1 = 9;
const int lcdPin2 = 10;

// Настройки оптической передачи
float baudRate = 2.0;               // Частота 2 Гц (2 бита в секунду)
long bitDuration = 1000 / baudRate; // 500 мс на один бит

// Безопасная частота переменного тока для ЖК (50 Гц)
const int acFrequency = 50;
const int acDelay = 1000 / (2 * acFrequency);

// C-массив для хранения входящей строки
char messageBuffer[128];
int messageLength = 0;

// Переменные состояния передачи
int currentCharIndex = 0;
int currentBitIndex = 6; // 7-bit ASCII читаем с 6-го по 0-й бит
bool isTransmitting = false;
bool isDark = false;

unsigned long previousBitMillis = 0;

void setup() {
  pinMode(lcdPin1, OUTPUT);
  pinMode(lcdPin2, OUTPUT);
  Serial.begin(9600);
  Serial.println("Готов к передаче 7-bit ASCII. Введите текст:");
}

void loop() {
  // 1. Чтение данных из консоли в массив
  if (Serial.available() > 0 && !isTransmitting) {
    // Читаем строку до символа переноса
    messageLength = Serial.readBytesUntil('\n', messageBuffer, 127);
    messageBuffer[messageLength] = '\0'; // Завершаем строку
    
    if (messageLength > 0) {
      Serial.print("Передаю: ");
      Serial.println(messageBuffer);
      
      isTransmitting = true;
      currentCharIndex = 0;
      currentBitIndex = 6;
      previousBitMillis = millis();
    }
  }

  unsigned long currentMillis = millis();

  // 2. Логика побитовой передачи
  if (isTransmitting) {
    if (currentMillis - previousBitMillis >= bitDuration) {
      previousBitMillis = currentMillis;
      currentBitIndex--; // Переходим к следующему биту

      if (currentBitIndex < 0) {
        currentBitIndex = 6; // Сбрасываем индекс бита для новой буквы
        currentCharIndex++;  // Переходим к следующей букве
        
        if (currentCharIndex >= messageLength) {
          isTransmitting = false; // Сообщение закончилось
          isDark = false;         // Очищаем экран
          Serial.println("Передача завершена.");
        }
      }
    }

    if (isTransmitting) {
      // Извлекаем текущий бит с помощью побитового сдвига
      char currentChar = messageBuffer[currentCharIndex];
      int bitValue = (currentChar >> currentBitIndex) & 0x01;
      
      isDark = (bitValue == 1);
    }
  }

  // 3. Аппаратное управление экраном (с сохранением AC-сигнала)
  if (isDark) {
    digitalWrite(lcdPin1, HIGH);
    digitalWrite(lcdPin2, LOW);
    delay(acDelay);
    
    digitalWrite(lcdPin1, LOW);
    digitalWrite(lcdPin2, HIGH);
    delay(acDelay);
  } else {
    digitalWrite(lcdPin1, LOW);
    digitalWrite(lcdPin2, LOW);
    delay(acDelay * 2);
  }
}
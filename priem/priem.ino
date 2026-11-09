const int PHOTODIODE_PIN = A0; 

// --- НАСТРОЙКИ ПРИЕМНИКА ---
const unsigned long T_MS = 250; 
const float LIGHT_OFFSET_MV = -15.0; 
const int K_PERIODS = 100; 
// ---------------------------

float baseline_mv = 0.0;     
float threshold_mv = 0.0;    

bool isReceiving = false;
int darkCounter = 0;
int k_counter = 0;               
unsigned long lastSampleTime = 0;

// --- ПЕРЕМЕННЫЕ ДЛЯ ASCII ---
String ascii_str = "";
uint8_t current_byte = 0;
int bit_index = 0;
// ----------------------------

void setup() {
  Serial.begin(115200);
  
  Serial.println("Калибровка фонового освещения (1 секунда)...");
  Serial.println("Не светите лазером на датчик!");
  
  long sum_adc = 0;
  const int calib_samples = 100;
  
  for (int i = 0; i < calib_samples; i++) {
    sum_adc += analogRead(PHOTODIODE_PIN);
    delay(10); 
  }
  
  float avg_adc = (float)sum_adc / calib_samples;
  baseline_mv = (avg_adc * 3300.0) / 1023.0;
  threshold_mv = baseline_mv + LIGHT_OFFSET_MV;
  
  Serial.print("Фон сохранен: ");
  Serial.print(baseline_mv);
  Serial.print(" мВ. Порог срабатывания: ");
  Serial.print(threshold_mv);
  Serial.println(" мВ.");
  Serial.println("Ожидание лазерного сигнала...");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastSampleTime >= T_MS) {
    lastSampleTime += T_MS; 

    uint16_t raw_adc = analogRead(PHOTODIODE_PIN);
    float voltage_mv = (raw_adc * 3300.0) / 1023.0;

    bool bitState = (voltage_mv < threshold_mv);

    if (!isReceiving) {
      // Подстройка динамического фона
      if (bitState == false) {
        baseline_mv = (baseline_mv * 0.9) + (voltage_mv * 0.1);
        threshold_mv = baseline_mv + LIGHT_OFFSET_MV;
      }

      if (bitState == true) { 
        // 1. Поймали СТАРТОВЫЙ БИТ.
        isReceiving = true;
        darkCounter = 0;
        k_counter = 0; 
        
        ascii_str = "";
        current_byte = 0;
        bit_index = 0; 
        // ВАЖНО: Мы НЕ добавляем эту '1' в current_byte. Это просто пробуждение!

        Serial.print("Начало передачи! Данные: ");
      } else {
        k_counter++;
        if (k_counter >= K_PERIODS) {
          k_counter = 0;
          Serial.print("[Фон: ");
          Serial.print(baseline_mv, 0); 
          Serial.print(" мВ, Порог: ");
          Serial.print(threshold_mv, 0);
          Serial.println(" мВ]");
        }
      }
    } else {
      // 2. Идет прием данных (ровно по 8 бит на символ)
      if (bitState == true) {
        Serial.print("1");
        darkCounter = 0; 
        current_byte = (current_byte << 1) | 1; 
      } else {
        Serial.print("0");
        darkCounter++;   
        current_byte = (current_byte << 1) | 0;
      }

      bit_index++;

      // Как только собрали 8 бит
      if (bit_index == 8) {
        if (current_byte == 0) {
          // Если пришел байт NUL (00000000), сразу завершаем
          isReceiving = false; 
        } else {
          // Записываем букву, только если это читаемый ASCII-символ (от пробела до '~')
          // Это защитит строку от вопросиков и кракозябр.
          if (current_byte >= 32 && current_byte <= 126) {
            ascii_str += (char)current_byte;
          }
          current_byte = 0;
          bit_index = 0;
        }
      }

      // 3. Завершение по NUL (7 нулей подряд)
      if (darkCounter >= 7) {
        isReceiving = false;
      }

      // Если сработало любое из условий завершения:
      if (isReceiving == false) {
        darkCounter = 0;
        k_counter = 0; 
        Serial.println(); 
        Serial.print("Завершено. Текст: [ ");
        Serial.print(ascii_str);
        Serial.println(" ]");
        lastSampleTime = millis(); // сброс таймера
      }
    }
  }
}
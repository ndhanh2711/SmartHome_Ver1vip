#include "sensor_handle.h"
#include "DHT.h"
//Wifi
#define ERA_LOCATION_VN
#define ERA_AUTH_TOKEN "b70461d1-3d3a-47fe-acd3-43b6a7f11c3e"
#include <Arduino.h>
#include <ERa.hpp>
#include <Automation/ERaSmart.hpp>
#include <Time/ERaEspTime.hpp>


const char ssid[] = "Quang Hai T3";
const char pass[] = "19741975";

ERaEspTime syncTime;
ERaSmart smart(ERa, syncTime);

//Task
// Khai báo Task Handle
TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;
TaskHandle_t Task3Handle = NULL;
TaskHandle_t Task4Handle = NULL;
TaskHandle_t Task5Handle = NULL;

#define BUZZER_PIN 4
#define FAN_PIN 32

#define LED1 5
#define LED2 18
#define LED3 19

void Data_Handle(void *pvParameters);
void Buzzer_Fan_Handle(void *pvParameters);

void setup() {
    Serial.begin(115200);

    ERa.setScanWiFi(true);
    ERa.begin(ssid, pass);

    dht1.begin();
    dht2.begin();
    dht3.begin();

    // Cấu hình chân cảm biến
    pinMode(Flame1_PIN, INPUT);
    pinMode(Flame2_PIN, INPUT);
    pinMode(Flame3_PIN, INPUT);

    pinMode(sensorPin1, INPUT);
    pinMode(sensorPin2, INPUT);
    pinMode(sensorPin3, INPUT);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);

    // Cấu hình chân đầu ra
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);


    // Tạo Task với FreeRTOS
    xTaskCreatePinnedToCore(Data_Handle, "Data_Handle",              4096, NULL, 5, &Task1Handle, 0);
    xTaskCreatePinnedToCore(Buzzer_Fan_Handle, "Buzzer_Fan_Handle",  1024, NULL, 1, &Task2Handle, 1);
    xTaskCreatePinnedToCore(App_Handle, "App_Handle",                4096, NULL, 4, &Task3Handle, 1);
    xTaskCreatePinnedToCore(Send_Data_App, "Send_Data_App",          4096, NULL, 3, &Task4Handle, 0);
}

// Task đọc dữ liệu từ cảm biến
void Data_Handle(void *pvParameters) {
    while (1) {
        // Đọc giá trị từ cảm biến
        Flame1_Value = flame1_getValue();
        Flame2_Value = flame2_getValue();
        Flame3_Value = flame3_getValue();

        CO1_Value = MQ21_getValue();
        CO2_Value = MQ22_getValue();
        CO3_Value = MQ23_getValue();

        Temp1_Value = temp1_getValue();
        Temp2_Value = temp2_getValue();
        Temp3_Value = temp3_getValue();

        Hum1_Value = hum1_getValue();
        Hum2_Value = hum2_getValue();
        Hum3_Value = hum3_getValue();

        // In giá trị cảm biến để debug
        Serial.print("Flame: ");
        Serial.print(Flame1_Value); Serial.print(" ");
        Serial.print(Flame2_Value); Serial.print(" ");
        Serial.print(Flame3_Value); Serial.print(" | ");

        Serial.print("CO: ");
        Serial.print(CO1_Value); Serial.print(" ");
        Serial.print(CO2_Value); Serial.print(" ");
        Serial.println(CO3_Value);

        vTaskDelay(pdMS_TO_TICKS(100)); // Cập nhật mỗi 100ms
    }
}

// Task kiểm tra giá trị cảm biến và điều khiển buzzer + quạt
void Buzzer_Fan_Handle(void *pvParameters) {
    while (1) {
        bool fire_detected = Flame1_Value || Flame2_Value || Flame3_Value;
        bool gas_detected = CO1_Value || CO2_Value || CO3_Value;

        if (fire_detected || gas_detected) {
            digitalWrite(BUZZER_PIN, HIGH);
            digitalWrite(FAN_PIN, HIGH);
        } else {
            digitalWrite(BUZZER_PIN, LOW);
            digitalWrite(FAN_PIN, LOW);
        }

        vTaskDelay(pdMS_TO_TICKS(200)); // Kiểm tra mỗi 200ms
    }
}
// Task 1: Gửi dữ liệu lên app
void Send_Data_App(void *pvParameters) {
    while (1) {
        if (isWiFiConnected()) {
            sendDataToApp();
        } else {
            Serial.println("WiFi Disconnected! Skipping data send...");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Giảm tần suất gửi
    }
}
// Task 2: Xử lý lệnh từ App
void App_Handle(void *pvParameters) {
    while (1) {
        ERa.run();
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Kiểm tra kết nối WiFi
bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Gửi dữ liệu đo lên app
void sendDataToApp() {
    ERa.virtualWrite(V0, Temp1_Value);
    ERa.virtualWrite(V1, Temp2_Value);
    ERa.virtualWrite(V2, Temp3_Value);

    ERa.virtualWrite(V3, Hum1_Value);
    ERa.virtualWrite(V4, Hum2_Value);
    ERa.virtualWrite(V5, Hum3_Value);


    ERa.virtualWrite(V6, Flame1_Value ? "WARNING" : "GOOD");
    ERa.virtualWrite(V7, Flame2_Value ? "WARNING" : "GOOD");
    ERa.virtualWrite(V8, Flame3_Value ? "WARNING" : "GOOD");

    ERa.virtualWrite(V9, CO1_Value  ? "WARNING" : "GOOD");
    ERa.virtualWrite(V10,CO2_Value  ? "WARNING" : "GOOD");
    ERa.virtualWrite(V11,CO3_Value  ? "WARNING" : "GOOD");
}
void loop() {
    // Không dùng loop() khi sử dụng FreeRTOS
}
ERA_WRITE(V13) {
    int led1 = param.getInt();
    digitalWrite(LED1, led1 ? HIGH : LOW); // Đảm bảo đúng mức logic
    ERa.virtualWrite(V13, led1); // Đồng bộ trạng thái về app
}
ERA_WRITE(V16) {
    int led2 = param.getInt();
    digitalWrite(LED2, led2 ? HIGH : LOW); // Đảm bảo đúng mức logic
    ERa.virtualWrite(V16, led2); // Đồng bộ trạng thái về app
}
ERA_WRITE(V17) {
    int led3 = param.getInt();
    digitalWrite(LED3, led3 ? HIGH : LOW); // Đảm bảo đúng mức logic
    ERa.virtualWrite(V17, led3); // Đồng bộ trạng thái về app
}
ERA_WRITE(V12) {
    int fan = param.getInt();
    digitalWrite(FAN_PIN, fan ? HIGH : LOW); // Đảm bảo đúng mức logic
    ERa.virtualWrite(V12, fan); // Đồng bộ trạng thái về app
}
// ERA_WRITE(V14) {
//     int led1 = param.getInt();
//     digitalWrite(LED1, led1 ? HIGH : LOW); // Đảm bảo đúng mức logic
//     ERa.virtualWrite(V14, led1); // Đồng bộ trạng thái về app
// }

#pragma once
#include <Arduino.h>

/* ========================================================================== */
/*                               CẤU HÌNH CHÂN (PINS)                         */
/* ========================================================================== */
#define PIN_SS          5
#define PIN_RST         4
#define PIN_SERVO       13
#define PIN_BUZZER      14
#define PIN_I2C_SDA     21
#define PIN_I2C_SCL     22
#define PIN_LED_RED     25   // Đèn tín hiệu ĐỎ: Đóng / Cấm vào / Bãi đầy
#define PIN_LED_GREEN   26   // Đèn tín hiệu XANH: Mở / Mời xe qua
#define PIN_TRIG        27   // Cảm biến siêu âm HC-SR04: Trigger phát xung
#define PIN_ECHO        32   // Cảm biến siêu âm HC-SR04: Echo nhận xung
#define PIN_DHT         15   // Cảm biến DHT22: Giám sát nhiệt độ, độ ẩm hầm đỗ xe

/* ========================================================================== */
/*                               CẤU HÌNH LCD                                 */
/* ========================================================================== */
#define LCD_I2C_ADDR    0x27
#define LCD_COLS        16
#define LCD_ROWS        2

/* ========================================================================== */
/*                            CẤU HÌNH BARIE (SERVO)                          */
/* ========================================================================== */
const int SERVO_OPEN_ANGLE        = 90;   // Góc mở barie (độ)
const int SERVO_CLOSE_ANGLE       = 0;    // Góc đóng barie (độ)
const unsigned long BARRIER_HOLD_MS     = 3500; // Thời gian giữ mở cho xe qua (ms)
const unsigned long BARRIER_MOVE_MS     = 500;  // Thời gian ước tính servo quay (ms)

/* ========================================================================== */
/*                           CẤU HÌNH THỜI GIAN & PHÍ                         */
/* ========================================================================== */
const unsigned long SCAN_DEBOUNCE_MS    = 5000; // Khoảng cách tối thiểu giữa 2 lần quét cùng 1 thẻ (ms)
const unsigned long MSG_DISPLAY_MS      = 2500; // Thời gian hiển thị cảnh báo (ms)
const uint32_t BASE_PARKING_FEE         = 5000; // Phí gửi xe cơ bản (VNĐ)
const uint32_t HOURLY_PARKING_FEE       = 2000; // Phụ thu mỗi block thời gian (VNĐ / 10s demo)

/* ========================================================================== */
/*                           CẤU HÌNH WI-FI & MQTT                            */
/* ========================================================================== */
const char* const WIFI_SSID             = "Wokwi-GUEST";      // Tên WiFi (Wokwi mặc định: Wokwi-GUEST)
const char* const WIFI_PASS             = "";                 // Mật khẩu WiFi (Wokwi để trống)

const char* const MQTT_BROKER           = "broker.hivemq.com";// MQTT Broker công khai
const int         MQTT_PORT             = 1883;               // Port MQTT mặc định

// Tiền tố topic riêng của nhóm (sửa ở đây sẽ tự cập nhật tất cả topics)
#define MQTT_PREFIX                     "parking_n4/"

const char* const MQTT_TOPIC_SCAN       = MQTT_PREFIX "rfid/scan";    // Topic Publish sự kiện quét thẻ
const char* const MQTT_TOPIC_CMD        = MQTT_PREFIX "barie/cmd";    // Topic Subscribe nhận lệnh điều khiển
const char* const MQTT_TOPIC_BARIE_STAT = MQTT_PREFIX "barie/status"; // Topic Publish trạng thái barie (open/closed)
const char* const MQTT_TOPIC_ALERT      = MQTT_PREFIX "alert";        // Topic Publish cảnh báo (INVALID_CARD)
const char* const MQTT_TOPIC_STATUS     = MQTT_PREFIX "status";       // Topic Publish trạng thái ESP32 (Online / Heartbeat)
const char* const MQTT_TOPIC_ENV        = MQTT_PREFIX "env";          // Topic Publish nhiệt độ, độ ẩm hầm đỗ xe
const char* const MQTT_TOPIC_FIRE       = MQTT_PREFIX "fire";         // Topic Cảnh báo cháy & Chế độ thoát hiểm
const unsigned long MQTT_RECONNECT_MS   = 5000;                       // Chu kỳ thử kết nối lại MQTT nếu mất mạng (ms)


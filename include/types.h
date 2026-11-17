#pragma once
#include <Arduino.h>

/**
 * @brief Trạng thái hoạt động của hệ thống (Finite State Machine)
 */
enum SystemState {
  STATE_IDLE,          // Chờ quẹt thẻ
  STATE_OPENING,       // Servo đang quay mở barie
  STATE_OPEN,          // Barie đang mở, đếm ngược giữ mở
  STATE_CLOSING,       // Servo đang quay đóng barie
  STATE_ALERT_HOLD,    // Giữ màn hình cảnh báo / từ chối
  STATE_FIRE_EVACUATE  // Báo cháy khẩn cấp, mở toang Barie thoát hiểm!
};

/**
 * @brief Cấu trúc dữ liệu thẻ gửi xe
 */
struct VehicleCard {
  String uid;             // Mã UID dạng chuẩn "XX XX XX XX"
  String licensePlate;    // Biển số xe
  String ownerName;       // Tên chủ phương tiện
  bool isParked;          // Trạng thái: true = Đang trong bãi | false = Đã ra ngoài
  unsigned long entryTime;// Thời điểm vào bãi (millis)
};

/**
 * @brief Kết quả xử lý sự kiện quẹt thẻ
 */
enum ScanResultType {
  SCAN_RESULT_IGNORED,    // Bỏ qua do debounce
  SCAN_RESULT_BARRIER_BUSY,// Barie đang hoạt động
  SCAN_RESULT_INVALID,    // Thẻ không có trong hệ thống
  SCAN_RESULT_CHECK_IN,   // Xe vào bãi thành công
  SCAN_RESULT_CHECK_OUT   // Xe ra bãi thành công
};

struct ScanResult {
  ScanResultType type;
  int cardIndex;
  unsigned long durationSec;
  uint32_t fee;
};

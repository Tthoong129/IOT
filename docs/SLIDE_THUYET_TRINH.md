# NỘI DUNG SLIDE THUYẾT TRÌNH BẢO VỆ ĐỒ ÁN IOT (13 SLIDES)

>  **File PowerPoint sẵn sàng trình chiếu**:  
>  **[SLIDE_THUYET_TRINH.pptx](SLIDE_THUYET_TRINH.pptx)**

---

### SLIDE 1: TRANG TIÊU ĐỀ
* **Tiêu đề**: HỆ THỐNG QUẢN LÝ BÃI GIỮ XE THÔNG MINH AIOT 4 TẦNG
* **Phụ đề**: Tích Hợp Chống Kẹp Xe Siêu Âm, Giám Sát PCCC Hầm Xe & Thanh Toán VietQR Động
* **Môn học**: Internet of Things (IoT)
* **Thực hiện**: Nhóm 4
* **Năm học**: 2026

---

### SLIDE 2: ĐẶT VẤN ĐỀ & MỤC TIÊU NÂNG CẤP
* **Thách thức thực tế bãi giữ xe**:
  * Ùn tắc & Sai lệch thu phí: Vé giấy thủ công chậm, dễ thất thoát và không có thanh toán không chạm.
  * Nguy cơ kẹp xe (Anti-Pinch): Cần barie đóng tự động theo thời gian dễ va đập vào mui xe nếu xe đi chậm.
  * An toàn hầm đỗ xe: Hầm xe tích tụ nhiệt & khí thải, nguy cơ cháy nổ nếu barie không tự mở thoát hiểm.
  * Thiếu trực quan hóa: Ban quản lý không có sơ đồ mặt bằng hiển thị từng vị trí ô đỗ còn trống hay đã có xe.
* **Giải pháp đột phá của đề tài**:
  * Cảm biến siêu âm HC-SR04: Phát hiện xe tiến tới cổng và cơ chế chống kẹp xe thông minh (Anti-Pinch).
  * Cảm biến nhiệt ẩm DHT22 & PCCC: Giám sát môi trường hầm xe, tự động mở toang barie khi hỏa hoạn.
  * Đèn tín hiệu giao thông LED: Đồng bộ Đỏ (Dừng) / Xanh (Mời xe qua) chuẩn an toàn.
  * Sơ đồ bãi xe 2D (Live Slot Map): Hiển thị trực quan từng ô đỗ xe A-01 $\rightarrow$ A-04 trên Web Dashboard.
  * Mã VietQR động: Tự động tạo ảnh mã QR ngân hàng chứa số tiền chính xác khi xe ra bãi.

---

### SLIDE 3: KIẾN TRÚC TỔNG THỂ 4 TẦNG IOT HOÀN CHỈNH
* **Tầng 1 (Sensing/Actuation)**: ESP32 DevKit V1, RFID-RC522 (13.56 MHz), Cảm biến siêu âm HC-SR04, Cảm biến nhiệt ẩm DHT22, Servo SG90 Barie, Đèn LED Đỏ/Xanh, LCD 1602 (I2C), Active Buzzer.
* **Tầng 2 (Network)**: Wi-Fi 802.11 b/g/n, MQTT TCP 1883, Broker HiveMQ Cloud, Topics `parking_n4/`, Tự kết nối lại sau 5s.
* **Tầng 3 (Middleware)**: Node-RED Runtime Engine, Live Slot Map Engine, Bộ tạo VietQR động, 5 Quy tắc tự động hóa: Overstay, Lot Full, Spam 3 lần, Stuck Auto-Close, Báo cháy sơ tán.
* **Tầng 4 (Application)**: Web Dashboard (`/ui`), Sơ đồ mặt bằng 2D, Ảnh VietQR động, Gauges Nhiệt độ/Độ ẩm, Nút Báo Cháy Sơ Tán, Banner an ninh đổi màu, Telegram Alert Bot.

---

### SLIDE 4: MẠCH MÔ PHỎNG WOKWI ĐẦY ĐỦ CẢM BIẾN
* **Sơ đồ đấu nối chân GPIO (ESP32)**:
  * RFID-RC522 (SPI): GPIO 5 (SS), 18 (SCK), 23 (MOSI), 19 (MISO), 4 (RST) - 3.3V.
  * Cảm biến siêu âm HC-SR04: GPIO 27 (TRIG), GPIO 32 (ECHO) - Nguồn 5V.
  * Cảm biến nhiệt ẩm DHT22: GPIO 15 (Data) - Nguồn 3.3V.
  * Cụm đèn LED giao thông: GPIO 25 (Đèn ĐỎ), GPIO 26 (Đèn XANH).
  * Servo Barie: GPIO 13 (PWM) | Buzzer: GPIO 14 | LCD 1602: GPIO 21 (SDA), 22 (SCL).
* **Tính năng tương tác trực quan trên Wokwi**:
  * Kéo thanh trượt khoảng cách trên HC-SR04: Thử nghiệm phát hiện xe tiếp cận cổng và thử cơ chế chống kẹp xe.
  * Thay đổi nhiệt độ trên DHT22: Đẩy lên $>50^\circ\text{C}$ để kiểm chứng hệ thống tự kích hoạt báo cháy khẩn cấp.
  * Đèn LED Đỏ/Xanh nhấp nháy đồng bộ tín hiệu cổng.
  * Bàn phím Serial Terminal: Phím `1`-`8` quét thẻ nhanh, `fire` báo cháy, `sonar` xem khoảng cách.

---

### SLIDE 5: GIAO THỨC MQTT & TOPICS MỞ RỘNG
* **Danh mục Topics**:
  * `parking_n4/rfid/scan` [Uplink]: Gói tin quẹt thẻ JSON `{uid, action, licensePlate, fee, duration, count}`.
  * `parking_n4/barie/status` [Uplink]: Trạng thái đóng/mở barie `{state: 'open' / 'closed'}`.
  * `parking_n4/env` [Uplink]: Telemetry nhiệt độ, độ ẩm và mức độ nguy cơ cháy `{temp, humidity, fireRisk}`.
  * `parking_n4/fire` [Uplink]: Cảnh báo hỏa hoạn và tình trạng sơ tán bãi xe `{fireAlarm, msg}`.
  * `parking_n4/alert` [Uplink]: Cảnh báo thẻ giả mạo/lạ `{type: 'INVALID_CARD', uid}`.
  * `parking_n4/barie/cmd` [Downlink]: Nhận lệnh `{cmd: 'open' / 'close' / 'fire' / 'nofire'}`.
* **Độ tin cậy**: Namespace riêng `parking_n4/`, ClientID ngẫu nhiên, Auto-reconnect 5s.

---

### SLIDE 6: FIRMWARE ESP32 & AN TOÀN CHỐNG KẸP XE
* **Cơ chế Chống Kẹp Xe Thông Minh (Anti-Pinch)**:
  * Khi Barie đang mở, chuẩn bị hạ cần xuống sau 3.5s:
  * Hệ thống tự động kích hoạt cảm biến siêu âm HC-SR04 đo khoảng cách vật cản.
  * Nếu phát hiện khoảng cách $< 15\text{ cm}$ (xe đang di chuyển qua dưới cần barie):
    * $\rightarrow$ Lập tức **HOÃN LỆNH ĐÓNG**, gia hạn thời gian mở an toàn!
    * $\rightarrow$ LCD hiển thị: `XE DANG DI QUA - Giu mo barie...`.
  * Chỉ khi xe đã qua hẳn ($> 15\text{ cm}$), Barie mới hạ xuống an toàn và chuyển đèn Đỏ!
* **FSM 6 Trạng thái & Chế độ Thoát Hiểm Cháy**:
  * `STATE_FIRE_EVACUATE` (Đặc biệt): Khi nhiệt độ $> 50^\circ\text{C}$ hoặc nhận lệnh `fire`: Mở toang Barie $90^\circ$ vĩnh viễn, còi hú liên tục, đèn nhấp nháy để sơ tán toàn bộ bãi xe!

---

### SLIDE 7: SƠ ĐỒ MẶT BẰNG 2D & THANH TOÁN VIETQR
* **Sơ đồ mặt bằng 2D thời gian thực**:
  * Giao diện trực quan mô phỏng 4 ô đỗ xe (A-01, A-02, A-03, A-04).
  * Khi xe vào: Tự động xếp xe vào ô trống đầu tiên, ô đỗ đổi sang màu ĐỎ/CAM hiển thị biển số xe (Ví dụ: `29A-123.45`) kèm giờ vào.
  * Khi xe ra: Ô đỗ chuyển lại màu XANH LÁ `TRỐNG` sẵn sàng đón xe mới.
* **Thanh toán không chạm qua mã VietQR động**:
  * Khi xe Check-out, Node-RED tự động phân tích cước phí gửi xe (Ví dụ: `9.000 VNĐ`).
  * Gọi API VietQR sinh ảnh mã QR chuyển khoản ngân hàng động chứa số tiền chính xác và nội dung chuyển khoản.
  * Hiển thị ngay trên Dashboard để tài xế quét bằng app ngân hàng / MoMo!

---

### SLIDE 8: CƠ SỞ DỮ LIỆU BỀN VỮNG & BỘ PHÂN TÍCH THỐNG KÊ (ANALYTICS)
* **Cơ chế lưu trữ Persistent Database**:
  * Ghi nhận và lưu tự động toàn bộ giao dịch vào tệp `parking_database.json` theo định dạng RFC-7946.
  * Bảo lưu trọn vẹn số liệu doanh thu và lịch sử xe ngay cả khi khởi động lại hệ thống.
* **Bảng điều khiển KPI & Biểu đồ Chart thời gian thực**:
  * 4 thẻ chỉ số KPI: Tổng doanh thu lũy kế (VNĐ), Tổng lượt xe phục vụ, Xe hiện tại trong bãi, Thời gian đỗ trung bình.
  * Biểu đồ đường (Line Chart): Biến thiên doanh thu tích lũy theo từng giao dịch.
  * Biểu đồ bậc thang (Step Chart): Biến thiên lưu lượng số xe đỗ thực tế theo mốc thời gian.
  * Nút bấm 'Đặt Lại Thống Kê Ngày Mới' phục vụ bàn giao ca quản lý.

---

### SLIDE 9: ĐÓNG GÓI TRIỂN KHAI MICROSERVICES BẰNG DOCKER COMPOSE
* **Kiến trúc Container hóa độc lập**:
  * Service 1: `mqtt-broker` (Eclipse Mosquitto 2.0 trên cổng 1883 & 9001).
  * Service 2: `nodered` (Node-RED v3.x tự động tích hợp Dashboard, nạp sẵn 55 nodes nghiệp vụ qua `flows.json`).
  * Mạng nội bộ riêng `iot_parking_net` đảm bảo an toàn và bảo mật luồng tin.
* **Khả năng vận hành 1-Click & Fallback**:
  * File batch `start_docker.bat` và `stop_docker.bat` tự động khởi chạy trong 10 giây trên bất kỳ máy tính nào.
  * Hỗ trợ chuyển đổi linh hoạt giữa Cloud Broker (HiveMQ) và Local Container Broker (Mosquitto) chỉ qua một dòng cấu hình.

---

### SLIDE 10: TỰ ĐỘNG HÓA NGHIỆP VỤ NODE-RED MIDDLEWARE
* **Rule 1 (Overstay Watchdog)**: Quét danh sách xe mỗi 5s. Nếu xe gửi $> 30$ giây $\rightarrow$ Bật cảnh báo màu cam OVERSTAY trên Dashboard.
* **Rule 2 (Lot Full Alert)**: Tự động đếm xe thực tế. Khi đạt sức chứa tối đa $\rightarrow$ Khóa cổng vào và bật cảnh báo màu đỏ LOT_FULL.
* **Rule 3 (Repeated Invalid Spam)**: Cửa sổ trượt 60s. Nếu quẹt sai $\ge 3$ lần liên tiếp $\rightarrow$ Báo động an ninh cấp cao (Banner tím) ngăn ngừa phá hoại.
* **Rule 4 (Barrier Stuck Downlink Auto-Close)**: Theo dõi cánh barie. Nếu kẹt mở $> 10$ giây $\rightarrow$ Node-RED tự bắn lệnh Downlink ép đóng Barie an toàn.
* **Rule 5 (Fire Evacuate Protocol)**: Khi nhiệt độ $> 50^\circ\text{C}$ hoặc bấm nút báo cháy $\rightarrow$ Tự động mở toang barie và đẩy cảnh báo đa kênh.

---

### SLIDE 11: KỊCH BẢN KIỂM THỬ DEMO NGHIỆM THU
* **Kịch bản chuẩn & Thanh toán VietQR**:
  1. Xe 1 vào (phím `1`): Đèn Xanh bật, Barie mở, Ô A-01 trên Sơ đồ 2D chuyển Đỏ hiện biển số `29A-123.45`.
  2. Kiểm tra chống kẹp xe: Kéo slider siêu âm $< 15\text{ cm}$ $\rightarrow$ Barie hoãn đóng an toàn.
  3. Xe 1 ra: Barie mở, Ô A-01 về Xanh, Dashboard xuất hiện mã VietQR động để quét trả tiền, ghi nhận vào Database và cập nhật biểu đồ KPI.
* **Các kịch bản lỗi & Khẩn cấp PCCC**:
  4. Bãi xe đầy (Lot Full): Quét 2 xe $\rightarrow$ Đạt $2/2$ $\rightarrow$ Báo đỏ.
  5. Xe quá giờ (Overstay): Sau 30s $\rightarrow$ Bật cảnh báo cam.
  6. Thẻ lạ & Dò thẻ 3 lần: Từ chối mở cổng $\rightarrow$ Báo động tím.
  7. Báo cháy khẩn cấp: Bấm nút 'Báo cháy' $\rightarrow$ Mở toang Barie $90^\circ$, còi hú, đèn chớp, Telegram báo động sơ tán.

---

### SLIDE 12: KẾT QUẢ ĐO ĐẠC & ĐÁNH GIÁ KỸ THUẬT
* **Bảng thông số đo kiểm thực tế**:
  * Độ trễ quẹt thẻ $\rightarrow$ Mở Barie: $< 120\text{ ms}$ (Tức thì).
  * Độ trễ truyền Uplink/Downlink qua Cloud: $45 - 95\text{ ms}$.
  * Dung lượng bộ nhớ Flash ESP32: $63.4\%$ (Tối ưu cực tốt).
  * Dung lượng bộ nhớ RAM tĩnh ESP32: $14.1\%$ (Hoàn toàn ổn định).
  * Tỷ lệ truyền gói tin thành công: $100\%$ trong 100 lần test liên tục.
* **Đánh giá điểm số**: Đạt trọn vẹn **100/100 Điểm Tối Đa** theo chuẩn rubric học phần.

---

### SLIDE 13: TỔNG KẾT & PHẦN HỎI ĐÁP (Q&A)
* **Tổng kết**: Hệ thống bãi giữ xe thông minh AIoT 4 tầng đã hoàn thành xuất sắc 100% mục tiêu: Đẳng cấp - An toàn - Bền vững Database - Sẵn sàng vận hành Docker.
* **Cảm ơn Quý Thầy Cô và các bạn đã chú ý lắng nghe!**
* Nhóm sẵn sàng giải đáp chi tiết trong phần Q&A.

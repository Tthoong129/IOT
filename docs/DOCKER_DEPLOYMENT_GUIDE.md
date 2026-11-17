# HƯỚNG DẪN TRIỂN KHAI HỆ THỐNG IOT TRÊN DOCKER CONTAINER

Tài liệu này hướng dẫn cách đóng gói và triển khai toàn bộ Tầng 2 (Network / MQTT Broker) và Tầng 3 (Middleware Node-RED) & Tầng 4 (Application Dashboard) của hệ thống Bãi giữ xe thông minh AIoT bằng **Docker & Docker Compose**.

---

## 1. Kiến Trúc Microservices Docker

Hệ thống được đóng gói thành cụm 2 dịch vụ độc lập giao tiếp qua mạng nội bộ Docker (`iot_parking_net`):

```
+-------------------------------------------------------------------------+
|                       DOCKER COMPOSE ENGINE                             |
|                                                                         |
|   +--------------------------+       +------------------------------+   |
|   |   CONTAINER 1: MOSQUITTO  |       |     CONTAINER 2: NODE-RED    |   |
|   |                          |       |                              |   |
|   |   Eclipse Mosquitto 2.0  | <---> |   Node-RED Runtime v3.x      |   |
|   |   MQTT Broker (Port 1883)|       |   Flow Engine + Dashboard    |   |
|   |   WebSockets  (Port 9001)|       |   Database Analytics Engine  |   |
|   +--------------------------+       +------------------------------+   |
|                ^                                    |                   |
|                |                                    v                   |
|         +--------------+                  +-------------------+         |
|         | Volume: Log  |                  | Persistent Volume |         |
|         | Volume: Data |                  | flows.json / DB   |         |
|         +--------------+                  +-------------------+         |
+-------------------------------------------------------------------------+
       ^                                              ^
       |                                              |
[ESP32 / Wokwi Simulator]                      [Web Browser / Dashboard]
(MQTT TCP: 1883)                               (HTTP: http://localhost:1880/ui)
```

---

## 2. Danh Mục Tài Nguyên Đóng Gói

1. **`docker-compose.yml`**: Định nghĩa 2 dịch vụ `mqtt-broker` (Eclipse Mosquitto) và `nodered` (Node-RED + Dashboard).
2. **`docker/mosquitto/config/mosquitto.conf`**: Cấu hình mở cổng 1883, cho phép kết nối nặc danh (`allow_anonymous true`) và lưu log.
3. **`docker/nodered/package.json`**: Tự động cài đặt thư viện phụ thuộc `node-red-dashboard` khi container khởi động.
4. **`docker/nodered/flows.json`**: Tự động nạp sẵn toàn bộ 55 nodes xử lý, sơ đồ mặt bằng 2D, sinh mã VietQR và bộ phân tích thống kê database.
5. **`start_docker.bat` / `stop_docker.bat`**: File batch chạy và dừng toàn bộ hệ thống bằng 1 click chuột trên Windows.

---

## 3. Các Bước Vận Hành

### Bước 1: Khởi động hệ thống
* **Cách 1 (Khuyên dùng)**: Nhấp đúp chuột vào file `start_docker.bat`.
* **Cách 2 (Dùng lệnh Terminal)**:
  ```bash
  docker compose up -d
  ```

### Bước 2: Kiểm tra trạng thái các Container
```bash
docker compose ps
```
Kết quả hiển thị:
* `iot_parking_mqtt`: Up (Port `0.0.0.0:1883->1883/tcp`, `0.0.0.0:9001->9001/tcp`)
* `iot_parking_middleware`: Up (Port `0.0.0.0:1880->1880/tcp`)

### Bước 3: Truy cập Giao diện Web
* **Node-RED Flow Editor**: `http://localhost:1880`
* **Node-RED Web Dashboard**: `http://localhost:1880/ui`

### Bước 4: Dừng hệ thống khi kết thúc
* Nhấp đúp vào `stop_docker.bat` hoặc gõ:
  ```bash
  docker compose down
  ```
  *(Dữ liệu doanh thu, lịch sử xe và cấu hình được bảo lưu an toàn trong Docker Volumes).*

---

## 4. Tích Hợp ESP32 / Wokwi Với Docker Mosquitto

Mặc định dự án đang cấu hình kết nối tới Broker đám mây HiveMQ (`broker.hivemq.com`). Nếu muốn chuyển sang dùng hoàn toàn Broker nội bộ trong Docker:
1. Mở file [include/config.h](file:///D:/HocHe/IOT/Project/Bai_Giu_Xe/include/config.h).
2. Sửa dòng cấu hình:
   ```cpp
   // Thay vì broker.hivemq.com:
   #define MQTT_BROKER   "host.wokwi.internal"  // Khi chạy mô phỏng Wokwi
   // Hoặc:
   #define MQTT_BROKER   "192.168.1.xxx"        // IP máy tính chạy Docker khi nạp mạch thật
   #define MQTT_PORT     1883
   ```

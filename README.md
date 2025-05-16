# 🌊 Water Quality Monitoring System

## 📌 Mục tiêu đề tài
Hệ thống giám sát chất lượng môi trường nước theo thời gian thực, sử dụng STM32 và ESP32, kết nối Firebase, với giao diện web trực quan.

## 🧠 Thành phần chính
- STM32F103C8T6: Đọc dữ liệu cảm biến
- ESP32: Gửi dữ liệu lên Firebase, nhận điều khiển
- Cảm biến: pH, TDS, độ đục, nhiệt độ, mực nước
- Mạch in thiết kế bằng Altium Designer
- Giao diện web: Giám sát, điều khiển bơm, relay
- Kết nối: WiFi, giao tiếp UART giữa STM32 ↔ ESP32

## 🚀 Hướng dẫn chạy thử
1. Mở file Altium trong thư mục `schematics/`
2. Nạp code cho STM32 từ thư mục `stm32_code/`
3. Nạp code cho ESP32 từ `esp32_code/`
4. Mở giao diện `web/index.html` để xem dữ liệu
5. Firebase đã cấu hình trong `web/firebase-config.js`

## 📷 Hình ảnh
![Giao diện web](images/web_interface.png)

## 📽 Video demo
Xem tại: [video_demo/demo.mp4](video_demo/demo.mp4)

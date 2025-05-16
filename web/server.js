
require("dotenv").config();
const express = require("express");
const axios = require("axios");
const cors = require("cors");

const app = express();
const port = 3000;


const GEMINI_API_KEY = "your_api_key";
const WEATHER_API_KEY = "your_weather_key"; 
const LOCATION = "Hanoi";

app.use(cors());
app.use(express.json());

app.get("/", (req, res) => {
  res.send("Server đang chạy.");
});

app.get("/analyze", async (req, res) => {
  try {

    const firebaseResponse = await axios.get("https://your_host/sensor.json");
    const data = firebaseResponse.data;

    if (!data) {
      return res.status(500).json({ error: "Không có dữ liệu từ Firebase." });
    }


    const weatherResponse = await axios.get(
      `https://api.weatherapi.com/v1/forecast.json?key=${WEATHER_API_KEY}&q=${LOCATION}&days=1&lang=vi`
    );

    const weather = weatherResponse.data.forecast.forecastday[0].day;
    const condition = weather.condition.text;
    const avgTemp = weather.avgtemp_c;
    const maxWind = weather.maxwind_kph;
    const humidity = weather.avghumidity;

    // 3. Tạo prompt phân tích
    const prompt = `Dữ liệu cảm biến nước:
- Độ ẩm không khí: ${data.humidity}%
- Nhiệt độ môi trường: ${data.tempDHT}°C
- Tổng chất rắn hòa tan (TDS): ${data.tds} ppm
- Độ đục nước: ${data.turbidity} NTU
- Độ pH: ${data.pH}
- Mực nước: ${data.waterLevel} cm

Dự báo thời tiết hôm nay:
- Trạng thái: ${condition}
- Nhiệt độ trung bình: ${avgTemp}°C
- Gió: ${maxWind} km/h
- Độ ẩm: ${humidity}%

Yêu cầu:
Đánh giá chất lượng nước và ảnh hưởng từ thời tiết đến hệ thống. Gợi ý hành động phù hợp (bật/tắt máy lọc, sủi, bơm...). Trình bày ngắn gọn, rõ ràng trong vài từ, và không in đậm
 bất cứ từ nào.`;

    // 4. Gửi POST đến Gemini Flash 2.0
    const geminiResponse = await axios.post(
      `https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=${GEMINI_API_KEY}`,
      {
        contents: [
          {
            parts: [{ text: prompt }]
          }
        ]
      },
      {
        headers: {
          "Content-Type": "application/json"
        }
      }
    );

    const reply = geminiResponse.data?.candidates?.[0]?.content?.parts?.[0]?.text || "Không có phản hồi từ Gemini.";
    res.json({ result: reply });
  } catch (error) {
    console.error("Lỗi khi gọi Gemini:", error.message);
    res.status(500).json({ error: "Đã xảy ra lỗi khi phân tích dữ liệu bằng Gemini." });
  }
});

app.listen(port, () => {
  console.log(`Server đang chạy tại http://localhost:${port}`);
});
require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');

// Khởi tạo ứng dụng Web Server
const app = express();

// 1. Cấu hình Middleware (Giúp Server hiểu được gói tin JSON mà mạch ESP32 gửi lên)
app.use(cors());
app.use(express.json());
app.use(express.static('Public'));

// 2. Gắn cái "cửa tiếp tân" (API Routes) mà em đã viết vào hệ thống
// Lưu ý: Đường dẫn này trỏ chính xác vào file rfid_route.js trong thư mục routes
const rfidRoutes = require('./routes/rfid_route'); 
app.use('/api', rfidRoutes); 

// 3. Kết nối với Cơ sở dữ liệu MongoDB Atlas
console.log("Đang thử kết nối đến MongoDB Atlas...");
mongoose.connect(process.env.MONGO_URI, {
  family: 4 // Ép Node.js dùng IPv4
})
  .then(() => console.log('SUCCESS: Đã kết nối xuyên suốt tới Database!'))
  .catch((error) => console.error('FAIL: Lỗi kết nối MongoDB:', error.message));

// 4. Khởi động Server và giữ nó chạy liên tục
const PORT = process.env.PORT;
app.listen(PORT, () => {
  console.log(`Server đang hoạt động và lắng nghe tại cổng: ${PORT}`);
});
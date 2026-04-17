const mongoose = require('mongoose');

// Định nghĩa cấu trúc của một dòng lịch sử quẹt thẻ
const historySchema = new mongoose.Schema({
  uid: { type: String, required: true },
  name: { type: String, required: true },
  auth_type: { type: String, enum: ['rfid', 'pin'], required: true }, // Lưu lại xem người này mở bằng thẻ hay phím
}, { timestamps: true }); // BÍ QUYẾT: Dòng này sẽ tự động sinh ra cột 'createdAt' ghi lại chính xác ngày giờ phút giây mở cửa!

// Xuất ra với tên 'History' để MongoDB tự tạo một collection mới tên là 'histories'
module.exports = mongoose.model('History', historySchema);
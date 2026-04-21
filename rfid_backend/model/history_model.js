const mongoose = require('mongoose');

// Định nghĩa cấu trúc của một dòng lịch sử quẹt thẻ
const historySchema = new mongoose.Schema(
  {
    uid: { type: String, required: true },
    name: { type: String, required: true },
    auth_type: { type: String, enum: ['rfid', 'pin'], required: true },
  }, 
  { timestamps: true } //Option của Schema để tự động sinh cốt 'createdAt' ghi lại chính xác ngày giờ phút giây truy cập
); 

module.exports = mongoose.model('History', historySchema);
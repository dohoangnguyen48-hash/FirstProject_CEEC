const mongoose = require('mongoose');

const rfidSchema = new mongoose.Schema(
  {
    uid: { type: String, required: true, unique: true },
    name: { type: String, required: true },
    status: { type: String, enum: ['active', 'inactive'], default: 'active' },
    pin: { type: String, required: true, unique: true } 
  }, 
  { timestamps: true } //Option của Schema để tự động sinh cốt 'createdAt' ghi lại chính xác ngày giờ phút giây truy cập
); 

module.exports = mongoose.model('RfidCard', rfidSchema);
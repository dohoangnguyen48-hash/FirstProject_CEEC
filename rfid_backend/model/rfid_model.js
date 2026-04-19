const mongoose = require('mongoose');

const rfidSchema = new mongoose.Schema({
  uid: { type: String, required: true, unique: true },
  name: { type: String, required: true },
  status: { type: String, enum: ['active', 'inactive'], default: 'active' },
  pin: { type: String, required: true, unique: true } 
}, { timestamps: true }); 

module.exports = mongoose.model('RfidCard', rfidSchema);
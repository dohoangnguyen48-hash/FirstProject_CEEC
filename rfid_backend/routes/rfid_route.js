const express = require('express');
const router = express.Router();
const RfidCard = require('../model/rfid_model'); 
const History = require('../model/history_model'); // VỪA THÊM: Mang cuốn sổ lịch sử ra bàn tiếp tân

// API 1: Kiểm tra quyền truy cập
router.post('/check', async (req, res) => {
    try {
        const { auth_type, credential } = req.body; 

        if (!auth_type || !credential) {
            return res.status(400).json({ status: "error", allow_access: false, oled_message: "Thieu auth_type hoac credential" });
        }

        // Logic phân loại tìm kiếm
        let searchQuery = {};
        if (auth_type === 'rfid') {
            searchQuery = { uid: credential };
        } else if (auth_type === 'pin') {
            searchQuery = { pin: credential };
        } else {
            return res.status(400).json({ status: "error", allow_access: false, oled_message: "auth_type khong hop le" });
        }

        const user = await RfidCard.findOne(searchQuery);

        if (user) {
            if (user.status === 'active') {
                
                // --- BẮT ĐẦU GHI LỊCH SỬ ---
                // Tạo một bản ghi mới nhét vào cuốn sổ History
                const newLog = new History({
                    uid: user.uid, // Lấy UID của người vừa quẹt
                    name: user.name, // Lấy tên
                    auth_type: auth_type // Ghi nhận hình thức mở cửa
                });
                await newLog.save(); // Lệnh này bắt Node.js chạy vào Database để lưu
                console.log(`📝 Đã ghi vào sổ lịch sử: ${user.name} mở bằng ${auth_type}`);
                // ---------------------------

                res.status(200).json({ 
                    status: "success", 
                    allow_access: true,
                    oled_message: `Xin chao ${user.name}!` 
                });
            } else {
                res.status(403).json({ status: "failed", allow_access: false, oled_message: "The bi khoa!" });
            }
        } else {
            res.status(404).json({ status: "failed", allow_access: false, oled_message: "Sai the!" });
        }
    } catch (error) {
        res.status(500).json({ status: "error", allow_access: false, oled_message: "Loi he thong", error: error.message });
    }
});

// ==========================================
// API 2: LẤY DANH SÁCH LỊCH SỬ (Dành cho Web)
// ==========================================
router.get('/history', async (req, res) => {
    try {
        // Lục trong sổ lịch sử lấy ra dữ liệu. 
        // .sort({ createdAt: -1 }) giúp sắp xếp người quẹt thẻ mới nhất lên đầu bảng.
        // .limit(50) chỉ lấy 50 lần quẹt gần nhất để trang web không bị lag nếu dữ liệu quá nhiều.
        const logs = await History.find().sort({ createdAt: -1 }).limit(50);
        
        res.status(200).json({ 
            status: "success", 
            total: logs.length,
            data: logs 
        });
    } catch (error) {
        res.status(500).json({ status: "error", message: "Lỗi khi lấy lịch sử", error: error.message });
    }
});

// ==========================================
// API 3: ĐỔI MẬT KHẨU (ĐỔI PIN)
// ==========================================
router.post('/change-pin', async (req, res) => {
    try {
        const { uid, new_pin } = req.body;

        // 1. Kiểm tra xem ESP32/Web có gửi thiếu thông tin không
        if (!uid || !new_pin) {
            return res.status(400).json({ 
                status: "error", 
                allow_access: false,
                oled_message: "Thieu uid hoac new pin" 
            });
        }

        // 2. Tìm người dùng trong Database dựa vào mã UID
        const user = await RfidCard.findOne({ uid: uid });

        if (!user) {
            return res.status(404).json({ 
                status: "error", 
                allow_access: false,
                oled_message: "Quyen da bi thay doi!" 
            });
        }
        if(new_pin === user.pin){
            return res.status(400).json({
                status: "error",
                allow_access: false,
                oled_message: "PIN moi phai khac!"
            })
        }
        // 3. Nếu PIN cũ đúng -> Cập nhật PIN mới và lưu lại
        user.pin = new_pin;
        await user.save();

        // 4. Trả kết quả về
        res.status(200).json({ 
            status: "success", 
            allow_access: true,
            oled_message: "Doi PIN OK!" 
        });

    } catch (error) {
        res.status(500).json({ status: "error", allow_access: false, oled_message: "Loi he thong", error: error.message });
    }
});

// ==========================================
// API 4: THÊM THẺ MỚI (Dành cho Admin Web)
// ==========================================
router.post('/cards', async (req, res) => {
    try {
        const { uid, name } = req.body;

        // 1. Kiểm tra đầu vào
        if (!uid || !name) {
            return res.status(400).json({ status: "error", message: "Vui lòng nhập đủ UID và Tên!" });
        }

        // 2. Kiểm tra xem mã UID này đã bị ai lấy chưa
        const existingCard = await RfidCard.findOne({ uid: uid });
        if (existingCard) {
            return res.status(409).json({ status: "error", message: "Mã thẻ này đã tồn tại trong hệ thống!" });
        }

        // 3. Tạo thẻ mới (Mongoose sẽ tự lấy PIN default là '0000' và status default là 'active')
        const newCard = new RfidCard({ uid: uid, name: name });
        await newCard.save();

        res.status(201).json({ status: "success", message: "Đã thêm thẻ thành công!" });

    } catch (error) {
        res.status(500).json({ status: "error", message: "Lỗi server", error: error.message });
    }
});

// ==========================================
// API 5: LẤY DANH SÁCH THẺ
// ==========================================
router.get('/cards', async (req, res) => {
    try {
        // Lấy toàn bộ thẻ, xếp thẻ mới tạo lên đầu
        const cards = await RfidCard.find().sort({ createdAt: -1 });
        res.status(200).json({ status: "success", data: cards });
    } catch (error) {
        res.status(500).json({ status: "error", message: "Lỗi lấy danh sách thẻ" });
    }
});

// ==========================================
// API 6: KHÓA / MỞ KHÓA THẺ
// ==========================================
router.put('/cards/:uid/toggle', async (req, res) => {
    try {
        const card = await RfidCard.findOne({ uid: req.params.uid });
        if (!card) {
            return res.status(404).json({ status: "error", message: "Không tìm thấy thẻ!" });
        }

        // Đảo ngược trạng thái: Nếu đang active thì thành inactive và ngược lại
        card.status = (card.status === 'active') ? 'inactive' : 'active';
        await card.save();

        res.status(200).json({ status: "success", new_status: card.status });
    } catch (error) {
        res.status(500).json({ status: "error", message: "Lỗi cập nhật trạng thái" });
    }
});

module.exports = router;
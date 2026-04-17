document.addEventListener('DOMContentLoaded', () => {
    fetchHistory();
    fetchCards(); // Gọi thêm hàm tải danh sách thẻ
});

// Hàm 1: Lấy danh sách thẻ
async function fetchCards() {
    try {
        const response = await fetch('/api/cards');
        const result = await response.json();
        const tableBody = document.getElementById('cards-table-body');
        tableBody.innerHTML = ''; 

        if (result.status === 'success') {
            result.data.forEach(card => {
                const isChecked = card.status === 'active' ? 'checked' : '';
                const statusBadge = card.status === 'active' 
                    ? '<span class="badge badge-success">Đang hoạt động</span>' 
                    : '<span class="badge badge-danger">Đã bị khóa</span>';

                const row = `
                    <tr>
                        <td class="font-weight-bold">${card.uid}</td>
                        <td>${card.name}</td>
                        <td>${card.pin}</td>
                        <td>${statusBadge}</td>
                        <td>
                            <label class="switch">
                                <input type="checkbox" ${isChecked} onchange="toggleCardStatus('${card.uid}')">
                                <span class="slider"></span>
                            </label>
                        </td>
                    </tr>
                `;
                tableBody.innerHTML += row;
            });
        }
    } catch (error) {
        console.error('Lỗi lấy danh sách thẻ:', error);
    }
}

// Hàm 2: Gọi API đổi trạng thái khi bấm công tắc
async function toggleCardStatus(uid) {
    try {
        const response = await fetch(`/api/cards/${uid}/toggle`, { method: 'PUT' });
        const result = await response.json();
        if (result.status === 'success') {
            fetchCards(); // Tải lại bảng để cập nhật màu sắc chữ Active/Inactive
        }
    } catch (error) {
        alert('Lỗi cập nhật trạng thái thẻ');
    }
}

// Hàm 3: Lấy Lịch sử (Giữ nguyên như cũ)
async function fetchHistory() {
    try {
        const response = await fetch('/api/history');
        const result = await response.json();
        const tableBody = document.getElementById('history-table-body');
        tableBody.innerHTML = ''; 

        if (result.status === 'success') {
            result.data.forEach(log => {
                const timeString = new Date(log.createdAt).toLocaleString('vi-VN');
                const row = `
                    <tr>
                        <td>${timeString}</td>
                        <td class="font-weight-bold">${log.name}</td>
                        <td><span class="badge badge-info">${log.auth_type}</span></td>
                    </tr>
                `;
                tableBody.innerHTML += row;
            });
        }
    } catch (error) {
        console.error('Lỗi khi tải lịch sử:', error);
    }
}

// Hàm 4: Xử lý Thêm Thẻ Mới
const addCardForm = document.getElementById('add-card-form');
if (addCardForm) {
    addCardForm.addEventListener('submit', async (e) => {
        e.preventDefault(); 
        const uidValue = document.getElementById('uid-input').value;
        const nameValue = document.getElementById('name-input').value;

        try {
            const response = await fetch('/api/cards', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ uid: uidValue, name: nameValue })
            });

            const result = await response.json();
            if (result.status === 'success') {
                addCardForm.reset(); 
                fetchCards(); // Tải lại bảng danh sách thẻ để thấy thẻ mới
            } else {
                alert('Lỗi: ' + result.message);
            }
        } catch (error) {
            console.error(error);
        }
    });
}
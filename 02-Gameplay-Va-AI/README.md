# Gameplay và AI: đọc một nhiệm vụ từ đầu đến cuối

Mục tiêu của phần này là giúp bạn giải thích được **vì sao người chơi phải quan sát, ra lệnh, quyết định, rồi thu dọn hiện trường**, và nối từng hành vi đó với nơi thực thi trong source hiện có. Gun gameplay được phân tích riêng ở quyển 03; ở đây ta đặt súng vào luật chơi lớn hơn.

Các nhận định được đọc từ snapshot local ngày **07/10/2026**, không phải tuyên bố về bản thương mại mới nhất. Mọi đường `Source/...`, `Config/...`, `Plugins/...` trong phần này đều tương đối với `Ready Or Not/` ở workspace gốc. Số dòng giúp tìm nhanh trong snapshot; sau khi sửa source hãy tìm lại bằng tên symbol. `Đã đọc source` không có nghĩa là đã chạy toàn bộ nhánh gameplay trong Editor.

1. [Vòng nhiệm vụ và quyền sở hữu trạng thái](01-Vong-Nhiem-Vu.md)
2. [Mục tiêu, ROE, điểm và kết quả](02-Muc-Tieu-ROE-Diem.md)
3. [Tương tác, bắt giữ, báo cáo và vật chứng](03-Tuong-Tac-Bat-Giu-Vat-Chung.md)
4. [Cửa, phá cửa và thiết kế căn phòng](04-Cua-Va-Breaching.md)
5. [Cơ thể người chơi, di chuyển, máu và inventory](05-Player-Movement-Health-Inventory.md)
6. [AI nhận biết thế giới và thay đổi morale](06-AI-Perception-Morale.md)
7. [AI chọn hành động, nghi phạm và dân thường](07-AI-Quyet-Dinh-Hanh-Vi.md)
8. [Đội SWAT, mệnh lệnh và điều hướng](08-SWAT-Command-Navigation.md)
9. [Đọc lại hai gameplay flow người dùng cung cấp](09-Doi-Chieu-Gameplay-Flow.md)

Nếu mới học Unreal, đọc [bản đồ kiến trúc](../04-Kien-Truc-Va-Quy-Trinh/01-Ban-Do-Source.md) trước chương 1, rồi làm bài tập cuối từng chương. Đừng học toàn bộ AI trước khi có một phòng với cửa, một người chơi và một trạng thái kết thúc rõ ràng.

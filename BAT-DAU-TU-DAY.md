# Bắt đầu từ đây

Bạn không cần đọc hết source trước khi làm một thứ chơi được. Cũng không nên bắt đầu bằng việc copy toàn bộ nhân vật và hy vọng các dependency tự nối với nhau. Mục tiêu của lộ trình này là giúp bạn biết **mình đang ở tầng nào, đang hỏi điều gì và cần sản phẩm nào để chứng minh đã hiểu**.

## Trong 30 phút đầu

1. Đọc [bản đồ kiến thức](01-Ban-Do-Kien-Thuc/01-ban-do.md). Chọn một câu hỏi: “một phát bắn hoạt động thế nào?” hoặc “vì sao nghi phạm chưa đầu hàng?”.
2. Đọc [một feature từ hành vi đến source](01-Ban-Do-Kien-Thuc/02-doc-mot-feature.md). Viết input, state trước/sau và thứ bạn quan sát được.
3. Mở [Gun Lab](05-Gun-Lab/README.md) trong project đã build. Dùng một khẩu trước, không bắt đầu bằng việc thay toàn bộ thông số.
4. Đọc [luồng Gun Gameplay](03-Gun-Gameplay/README.md), đặt đường dẫn và symbol bên cạnh hành động vừa thử.
5. Ghi một điều đã xác minh và một điều chưa biết. Đó là kết quả học đầu tiên, không phải số file đã mở.

## Chọn lối vào theo nền tảng

| Bạn hiện ở đâu? | Đọc trước | Bài làm đầu tiên |
|---|---|---|
| Mới với Unreal/C++ | [Tám tầng](01-Ban-Do-Kien-Thuc/01-ban-do.md) và [thuật ngữ](01-Ban-Do-Kien-Thuc/06-thuat-ngu.md) | Phân biệt Class, CDO, Blueprint và instance bằng một khẩu súng |
| Biết Blueprint, ít đọc C++ | [Cách lần feature](01-Ban-Do-Kien-Thuc/02-doc-mot-feature.md) | Tìm nơi input gọi vào native class và nơi dữ liệu Blueprint được dùng |
| Biết C++, chưa làm game lớn | [Kiến trúc dự án](04-Kien-Truc-Va-Quy-Trinh/README.md) | Vẽ owner của ammo, reload, pose và damage, không cho chúng dùng chung một state giả |
| Muốn học cảm giác bắn | [Gun Gameplay](03-Gun-Gameplay/README.md) và [Gun Lab](05-Gun-Lab/README.md) | HIP single → ADS single → hold trigger → reload trên cùng một loadout |
| Muốn dựng game hoàn chỉnh | [Gameplay/AI](02-Gameplay-Va-AI/README.md) và [lộ trình](01-Ban-Do-Kien-Thuc/05-lo-trinh-thuc-hanh.md) | Một phòng, một mục tiêu, một kết quả, rồi chơi lại |

## Cách sử dụng AI khi học

Giao một câu hỏi có thể kiểm chứng: “Lần từ input fire tới điểm ammo bị trừ trong snapshot này, nêu symbol, state owner và ca reload bị hủy”. Đừng giao “giải thích toàn bộ file 15.000 dòng” rồi dùng bản tóm tắt ấy thay cho việc đọc nhánh thật.

Khi yêu cầu AI triển khai, kèm một [feature dossier](01-Ban-Do-Kien-Thuc/03-hop-dong-feature.md), write-set hẹp, dependency và acceptance. Yêu cầu nó ghi **đã chạy gì**, **chưa chạy gì**, **artifact ở đâu**. Nếu đổi engine, asset, config hoặc weapon class, kết quả kiểm chứng cũ không tự chuyển sang cấu hình mới.

## Ba câu hỏi tự kiểm tra sau mỗi chương

- Tôi có chỉ ra được owner và điểm commit của hành động không?
- Tôi có một trace hoặc tình huống tái lập để phản bác lời giải thích của mình không?
- Tôi đang đọc dữ liệu native, suy luận từ tên file hay đề xuất một implementation mới?

Khi trả lời được, quay lại [mục lục](00-MucLuc.md), chọn nhánh tiếp theo. Sách không yêu cầu đọc tuần tự tất cả trước khi thử một thay đổi nhỏ.

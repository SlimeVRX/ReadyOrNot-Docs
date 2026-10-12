# Ready Or Not — Technical Design Document bằng tiếng Việt

Phần này giúp bạn hiểu Ready or Not như một hệ thống có quan hệ nhân quả: vì sao người chơi phải đi chậm, quan sát, ra lệnh, lựa chọn cách xử lý, rồi bảo toàn hiện trường. Từ đó, bạn có thể chia công việc tái tạo thành những vòng chơi nhỏ có thể làm và kiểm tra được.

Nội dung được biên soạn lại từ **RoN_TDD_en.pdf**, 116 trang, ghi ngày **27/09/2026**; bản tiếng Việt được bổ sung vào bộ sách ngày **12/10/2026**. PDF tự giới thiệu là tài liệu R&D suy ngược từ cây source UE 5.3. Đây **không phải TDD chính thức của VOID Interactive**, và những nhận định trong PDF không tự trở thành mô tả của bản game bán lẻ hiện tại. [Hồ sơ nguồn và cách đối chiếu](21-nguon-va-doi-chieu.md) giải thích rõ phạm vi này.

Bản này diễn giải các cơ chế bằng tiếng Việt, giữ tên class, symbol, công thức và điều kiện quan trọng để bạn tra cứu. Nó bao quát 19 mục gốc từ §0 đến §18, thêm lộ trình thực hành và phiếu triển khai. Đây là bản biên soạn phục vụ học tập, không phải bản dịch đối dòng của từng câu.

## Lộ trình đọc

Nếu bạn chưa biết bắt đầu từ đâu, đọc [tổng kết hệ thống](00-tom-tat.md), [vòng chơi](01-tong-quan-vong-choi.md), rồi [lộ trình tái tạo](19-lo-trinh-tai-tao.md). Sau đó chọn một mốc nhỏ, dùng các chương cơ chế như sách tra cứu và điền [phiếu triển khai](20-phieu-trien-khai.md). Chưa cần hiểu hết AI hoặc networking trước khi làm một căn phòng có đầu và cuối.

| Mục TDD | Chương tiếng Việt | Trang PDF gốc |
|---|---|---|
| §0 | [Tổng kết và cách nhìn toàn bộ hệ thống](00-tom-tat.md) | 3–4 |
| §1 | [Trụ cột thiết kế và vòng chơi](01-tong-quan-vong-choi.md) | 5–7 |
| §2 | [Kiến trúc codebase và dữ liệu](02-kien-truc-codebase.md) | 8–14 |
| §3 | [Nhân vật, di chuyển và tương tác](03-player-movement-interaction.md) | 15–21 |
| §4 | [Vũ khí, đường đạn và hit registration](04-weapons-ballistics-hitreg.md) | 22–27 |
| §5 | [Damage, health và bắt giữ](05-damage-health-arrest.md) | 28–34 |
| §6 | [Trang bị, inventory và vật triển khai](06-equipment-inventory.md) | 35–41 |
| §7 | [Cửa, khóa, bẫy và phá cửa](07-doors-breaching.md) | 42–48 |
| §8 | [Giao diện và quy trình chỉ huy đội](08-squad-command-ux.md) | 49–55 |
| §9 | [Trí tuệ nhân tạo](09-ai.md) | 56–61 |
| §10 | [Nhiệm vụ, ROE và chấm điểm](10-mission-roe-scoring.md) | 62–68 |
| §11 | [Commander, đội hình, stress và planning](11-commander-meta-planning.md) | 69–75 |
| §12 | [Quy trình làm level](12-level-design-pipeline.md) | 76–78 |
| §13 | [Hệ thống âm thanh](13-audio.md) | 79–82 |
| §14 | [Trình diễn và cảm giác điều khiển](14-presentation-feel.md) | 83–89 |
| §15 | [Mạng, nền tảng và mod](15-network-platform-mods.md) | 90–96 |
| §16 | [Công cụ, build và kiểm thử](16-tooling-build-tests.md) | 97–103 |
| §17 | [Danh mục vấn đề cần kiểm chứng](17-known-issues.md) | 104–113 |
| §18 | [Các mẫu thiết kế có thể học lại](18-reuse-playbook.md) | 114–116 |

## Từ tài liệu tới một bản game do bạn làm

PDF cho biết một hệ thống được mô tả là hoạt động thế nào. Các phần 01–06 của ReadyOrNot-Docs đã có một đường đọc khác: lần theo snapshot source, config, asset và lab. **Đặt hai đường này cạnh nhau**, thay vì lấy một mô tả trong PDF làm bằng chứng rằng bản dựng của bạn đã có tính năng đó.

| Câu hỏi | Nơi bắt đầu |
|---|---|
| Vì sao game cần hệ thống này? | §1 và chương tương ứng trong phần TDD |
| Ai giữ trạng thái, dữ liệu đi qua đâu? | §2, các bảng owner và [kiến trúc hiện có](../04-Kien-Truc-Va-Quy-Trinh/README.md) |
| Source trong snapshot có nhánh này không? | [Gameplay và AI](../02-Gameplay-Va-AI/README.md), [Gun Gameplay](../03-Gun-Gameplay/README.md) và định vị file/symbol |
| Nó thật sự chạy trên bản dựng của tôi chưa? | [Gun Lab](../05-Gun-Lab/README.md) hoặc fixture riêng với log, input, kết quả |
| Tôi nên tự viết gì tiếp? | [Lộ trình tái tạo](19-lo-trinh-tai-tao.md), [phiếu triển khai](20-phieu-trien-khai.md) |

Ví dụ, “AI có morale” chưa phải yêu cầu đủ để triển khai. Bạn cần biết morale nhận input nào, đổi khi nào, action nào đọc nó, người chơi quan sát được gì và server nào quyết định. Một clip đầu hàng đẹp cũng chưa thay thế được state bắt giữ, báo cáo và điểm nhiệm vụ. Mỗi chương sẽ nối các tầng đó lại.

## Quy ước đọc bằng chứng

- **TDD §x, tr. y:** diễn giải nội dung PDF. Định vị `file:line` đi kèm là định vị tài liệu cung cấp, chưa mặc nhiên được đọc lại trên snapshot mới.
- **INFERRED / UNVERIFIED:** giữ tính suy luận hoặc chưa kiểm chứng của tác giả gốc.
- **Bài thực hành / đề xuất tái tạo:** thiết kế bài tập cho dự án của bạn, không phải kết quả implementation vừa hoàn thành.
- **SOURCE / CONFIG / BUILD / RUNTIME:** dùng nghĩa đã định trong [phạm vi bộ sách](../README.md). Đọc source, build thành công và chơi được là các mức khác nhau.

Danh mục §17 ghi **118 vấn đề theo PDF**, gồm 12 high, 38 medium và 68 low. Đây là các kết luận của lần phân tích mà PDF mô tả, không phải 118 lỗi vừa được chạy thử lại hay danh sách lỗi của mọi phiên bản Ready or Not.

## Phạm vi phần mới

Phần này bổ sung tài liệu học và kế hoạch tái tạo; không tự tạo ra toàn bộ gameplay. Các trang online chứa nội dung biên soạn và metadata nguồn, không đính kèm PDF gốc, source game, engine hoặc asset. Khi cần đối chiếu trang, dùng bản PDF bạn đang giữ. Những chi tiết thiết kế suy ngược được học dưới dạng nguyên lý và implementation do bạn tự viết.

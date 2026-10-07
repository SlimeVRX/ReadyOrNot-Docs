# Kiến trúc và quy trình: học cách một dự án lớn được nối lại

Source lớn không cần được hiểu hết cùng lúc. Điều cần là một cách tìm đường ổn định: từ trải nghiệm tới state, từ state tới chủ sở hữu, từ chủ sở hữu tới code và asset, rồi quay lại runtime để kiểm chứng. Phần này đưa ra bản đồ và thứ tự học cho việc đó.

1. [Bản đồ source và ranh giới hệ thống](01-Ban-Do-Source.md)
2. [Từ C++ tới Blueprint, asset và level](02-Asset-Blueprint-Level.md)
3. [Multiplayer và quyền sở hữu trạng thái](03-Multiplayer-State-Ownership.md)
4. [Save, config, mode và phiên bản](04-Save-Config-Version.md)
5. [Lộ trình kiến thức để dựng lại](05-Lo-Trinh-Tai-Tao.md)
6. [Quy trình phát triển và hồ sơ kiểm chứng](06-Quy-Trinh-Kiem-Chung.md)

Đường source trong phần này tương đối với thư mục `Ready Or Not/`, trừ chỗ ghi rõ `Engine/` là thư mục ở workspace gốc. Tài liệu diễn giải source local ngày **07/10/2026**. Các đề xuất thiết kế lại được ghi là đề xuất; chúng không được coi là kiến trúc nguyên văn của dự án.

Tinh thần tổ chức được tham khảo từ các bộ tài liệu local Fortnite-Docs và Paldark-Docs: đọc trải nghiệm trước, học theo phụ thuộc, dùng contract và acceptance criteria. Nội dung Ready or Not được xây từ source của chính project; các quy tắc nội bộ hoặc quyết định của game khác không tự áp dụng sang đây.

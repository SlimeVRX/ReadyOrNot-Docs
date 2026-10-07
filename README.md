# ReadyOrNot Docs

[Đọc website](https://slimevrx.github.io/ReadyOrNot-Docs/) · [Repository](https://github.com/SlimeVRX/ReadyOrNot-Docs) · [Bắt đầu từ đây](BAT-DAU-TU-DAY.md)

Bộ sách tiếng Việt để học cách một dự án gameplay lớn hoạt động: đi từ vòng nhiệm vụ, class và dữ liệu đến Gun Gameplay, AI, animation, mạng và quy trình kiểm chứng. Phần thực hành gồm một **Native Gun Lab** bổ sung vào project cục bộ, sử dụng các hệ thống súng sẵn có.

## Nguồn nào đang được nghiên cứu?

Snapshot tại workspace ReadyOrNot, dùng **custom Unreal Engine 5.3.2**, quan sát **07/10/2026**. Engine ghi Changelist 0 và BranchName Unknown; bộ source không có commit/release tag đủ để gán một phiên bản bán lẻ. Những mô tả trong sách chỉ áp dụng cho snapshot được kiểm kê.

Kiểm kê filesystem ban đầu có **1.512 file C++/header/build rules, 349.488 dòng văn bản**, **135.184 file .uasset**, **1.343 file .umap** và **23 descriptor plugin của project**. Dòng văn bản gồm cả comment và dòng trống; file map bao gồm bản đồ nội bộ/sublevel/thử nghiệm. Những con số này không chứng minh số màn chơi thương mại hoặc mức hoàn thiện. Xem [catalog snapshot](06-Catalogs/README.md) để biết phương pháp và dữ liệu cập nhật sau khi tạo lab.

## Đọc bằng chứng thế nào?

| Nhãn | Nó chứng minh gì? | Nó chưa chứng minh gì? |
|---|---|---|
| SOURCE | Đã đọc implementation, symbol và đường dẫn cụ thể | Nhánh ấy đang chạy với mọi cấu hình |
| CONFIG / CDO | Đã đọc config hoặc default của asset/class | Giá trị runtime sau override, attachment, trạng thái và mạng |
| GENERATED | Script đã tạo artifact và lưu được | Người chơi có thể dùng trọn vẹn |
| BUILD | Target đã compile/link | Gameplay, nội dung asset hoặc cảm giác đúng |
| RUNTIME | Một ca chạy được ghi lại với input và kết quả | Các ca chưa thử, multiplayer hoặc parity toàn game |
| MANUAL | Người chơi đối chiếu điều kiện được ghi nhận | Một chứng nhận khách quan 1:1 |
| INFERENCE / PROPOSED | Suy luận hoặc thiết kế bài tập | Sự thật đã có sẵn trong source |

Mỗi chương trích **đường dẫn tương đối + symbol + dòng**. Đường dẫn source/asset là địa chỉ tra cứu cục bộ; website không chứa bản sao nội dung source gốc. Hash nằm trong catalog để phát hiện thay đổi snapshot. Sau một chỉnh sửa source, tìm lại symbol trước khi tin số dòng cũ.

## Sách và level là hai sản phẩm liên quan

Sách giải thích cách tái tạo theo từng lớp. Lab dùng lại implementation và asset native để có một mốc đối chiếu gần nguồn nhất, giúp bạn đo trước khi tự viết lại. **Lab native không phải một bản game độc lập đã được tái tạo**, và việc equip được mọi entry không tự chứng minh mọi animation/âm thanh/attachment hoặc cảm giác đã được kiểm tra.

Trạng thái thực nghiệm nằm ở [Gun Lab](05-Gun-Lab/README.md); các receipt ghi đúng từng ca. Lỗi asset từ quá trình chuyển UE hoặc phụ thuộc còn thiếu phải được giữ như giới hạn, không che bằng cơ chế súng thay thế.

## Tổ chức

1. [Bản đồ kiến thức và phương pháp học](01-Ban-Do-Kien-Thuc/README.md)
2. [Gameplay và AI](02-Gameplay-Va-AI/README.md)
3. [Gun Gameplay](03-Gun-Gameplay/README.md)
4. [Kiến trúc và quy trình dự án](04-Kien-Truc-Va-Quy-Trinh/README.md)
5. [Native Gun Lab](05-Gun-Lab/README.md)
6. [Catalog và bằng chứng](06-Catalogs/README.md)

## Liên hệ với ba bộ Docs trước

**Wardogs-Docs** cung cấp cách phân rã gunfeel và bài học: đúng animation asset chưa đủ nếu dùng sai additive base, thời điểm tick hoặc camera. **Fortnite-Docs** cung cấp cách đi từ vòng chơi đến capability, dữ liệu, lộ trình và acceptance. **Paldark-Docs** cung cấp cách tách owner, dependency, quyết định kiến trúc và bằng chứng hoàn thành.

Đây là các tài liệu tham khảo về phương pháp. Các quy tắc Human Gate, target engine hoặc policy ở dự án khác không tự trở thành lệnh hay ràng buộc của ReadyOrNot.

## Phạm vi công bố

Repository công khai chứa nội dung phân tích do bộ sách biên soạn, sơ đồ dựng lại, catalog metadata, công cụ và harness học tập mới. Không đưa source game/Engine, mesh, animation, âm thanh, texture, bank, map nhị phân hay khóa truy cập vào repo. Hai ảnh flow người dùng cung cấp là tư liệu quan sát cục bộ; sách đối chiếu và vẽ lại quan hệ, không biến mỗi node thành tính năng đã xác minh.

Ready or Not và các tài nguyên gốc thuộc chủ sở hữu tương ứng. Sách là nghiên cứu độc lập cho học tập phi thương mại; không cấp quyền phân phối lại tài nguyên gốc.

## Build website

Node.js 22 trở lên. Chạy:

```sh
npm ci
npm run docs:build
npm run docs:check
```

GitHub Actions xuất bản từ main. [Hướng dẫn cập nhật và xuất bản](WEBSITE.md).

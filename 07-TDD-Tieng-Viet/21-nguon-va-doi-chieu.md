# Nguồn TDD và cách đối chiếu với bộ sách

## Hồ sơ tài liệu

| Trường | Giá trị đã đọc |
|---|---|
| File người dùng cung cấp | `RoN_TDD_en.pdf` |
| Tiêu đề trên bìa | Ready Or Not — Technical Design Document |
| Phạm vi tự mô tả | Reverse-engineered R&D reference, UE 5.3 source tree |
| Ngày trên bìa | 27/09/2026 |
| Số trang | 116, số trang vật lý khớp footer của phần nội dung đã đọc |
| Ngôn ngữ nội dung | Tiếng Anh; dòng bìa có chữ “Tiếng Việt” |
| Tác giả/tư cách chính thức | Không có căn cứ trong file để gán đây là TDD chính thức của VOID |
| Biên soạn tiếng Việt | 12/10/2026, phần 07 của ReadyOrNot-Docs |

SHA-256 của file dùng cho lần biên soạn này:

```text
4e408cc4d1589125cab426f4a1c02c297b1e8dbab9bd70463b521f653a2d9650
```

Hash giúp bạn xác định đúng tài liệu trước khi đối chiếu trang; nó không xác thực tác giả hoặc độ đúng của mọi kết luận trong tài liệu. Metadata của PDF ghi thời gian tạo theo UTC ngày 26/09, tương ứng 27/09 theo múi giờ +07; không dùng thời gian xuất PDF để suy ra ngày phát hành bản game.

## Các phần của bản tiếng Việt

[Mục lục phần TDD](README.md) ánh xạ đủ §0–§18 với trang gốc. [Coverage JSON](coverage.json) giữ cùng ánh xạ ở dạng máy đọc được. Mỗi chương diễn giải mục tương ứng, giữ các thuật ngữ và điều kiện quan trọng, rồi thêm bài tập tái tạo. Chương 19 và phiếu 20 là phần tổ chức học do bộ sách bổ sung.

Không xem đây là một bản dịch chứng thực hoặc bản sao nguyên định dạng của PDF. Các bảng và sơ đồ phục vụ giải thích được viết lại; tên symbol vẫn giữ nguyên để tìm nguồn. Những phần bị cắt dòng khi xuất PDF được xử lý bằng nội dung xung quanh và điều kiện đã đọc; không tự điền chi tiết còn thiếu như một sự thật.

## Vì sao không gộp các con số kiểm kê

| Đại lượng | PDF ngày 27/09/2026 | Catalog Docs ngày 08/10/2026 |
|---|---|---|
| Engine được mô tả | UE 5.3, có dấu vết nâng từ UE4 | Custom UE 5.3.2 |
| Source files | 1.507 trong phần tóm tắt; có phạm vi đếm riêng cho `.h/.cpp` ở §2 | 1.512 theo phương pháp kiểm kê của Docs |
| Dòng source | Khoảng 348 nghìn, số làm tròn | 349.495 trong JSON catalog hiện có |
| Asset `.uasset` | Khoảng 135 nghìn | 135.519 |
| File `.umap` | 1.343 | 1.344 |
| Danh tính retail release | Không được xác lập bằng commit/build có thể đối chiếu trong PDF | `retailVersion: null`, source commit không xác định |

Nguồn cột phải: [snapshot.json](../06-Catalogs/snapshot.json). README trước đó có thể giữ con số của lần kiểm kê ban đầu; khi cần kiểm máy đọc được, dùng JSON cùng phương pháp và ngày của nó. Chênh lệch có thể do phạm vi đếm, file lab hoặc snapshot khác; bảng này không khẳng định nguyên nhân cụ thể chưa được kiểm chứng.

## Mức bằng chứng của các nhận định

TDD nói đã dùng review theo nhiều lát cắt và đối chiếu code, rồi gọi danh mục §17 là “verified issues”. Bản tiếng Việt giữ nội dung và phân loại **theo TDD**. Nó không tuyên bố đã đọc lại toàn bộ source hoặc chạy lại 118 tình huống vào ngày 12/10/2026. Đặc biệt, một số dòng trong tài liệu vẫn ghi suy luận hoặc chưa chạy; những điều kiện ấy cần đi cùng kết luận.

Khi muốn dùng một nhận định để sửa project, làm lần lượt:

1. Tìm đúng mục/trang PDF và ghi phạm vi kết luận.
2. Tìm symbol trên snapshot đang làm; kiểm branch, macro, call site và effective config.
3. Kiểm asset/CDO nếu hành vi phụ thuộc Blueprint hoặc dữ liệu.
4. Tạo ca tái lập nhỏ trong đúng build flavor: Editor, standalone, listen server, dedicated server hoặc packaged build.
5. Lưu expected/actual và cập nhật nhãn từ TDD sang SOURCE hoặc RUNTIME khi thực sự có bằng chứng mới.

Đọc một nhánh source không đủ để khẳng định nó được gọi trong mọi mode. Một tên feature, class, macro hoặc entry trong catalog cũng không chứng minh tính năng đó đã bật. Đây là điểm đặc biệt quan trọng với host migration, dịch vụ online, cheat/debug và các tunable được PDF nêu.

## Cách sử dụng danh mục vấn đề

[Chương 17](17-known-issues.md) là backlog điều tra: giữ 118 dòng và mức độ mà PDF gán. Đừng coi nó là hướng dẫn tái tạo lỗi. Với một game mới, chọn bài học tổng quát và viết ca kiểm: validation ở authority, biến đổi state một lần, phép tính có kiểu đúng, action có đường hủy, callback có vòng đời rõ và ma trận build có phạm vi đầy đủ.

Đối với mục liên quan credential hoặc trust boundary, chỉ mô tả vấn đề kiến trúc và cách phòng tránh. Bản công bố không chứa giá trị khóa truy cập hay hướng dẫn khai thác một dịch vụ đang chạy.

## Phạm vi xuất bản

Repo công khai chứa phần tiếng Việt do bộ sách biên soạn cùng metadata đối chiếu. PDF gốc do người dùng cung cấp, source/engine và asset nhị phân không được đính kèm vào phần này. Định vị `file:line` là địa chỉ đọc cục bộ, không phải link phân phối file game. Tài liệu không cấp quyền đối với code hoặc tài nguyên thuộc chủ sở hữu khác.

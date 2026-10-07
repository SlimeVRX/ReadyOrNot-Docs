# Nguồn, phương pháp và cách tái lập

## Nguồn quyết định cho snapshot

| Nguồn | Nó trả lời điều gì? | Giới hạn |
|---|---|---|
| Ready Or Not/Source | Implementation C++, reflection/build rules | Blueprint/config/runtime có thể chọn nhánh và đổi dữ liệu |
| Ready Or Not/Config | Input/platform/game settings và data config | Hierarchy, platform override và generated config cần đọc cùng nhau |
| Ready Or Not/Content qua Editor | Class ancestry, CDO, asset references, map | Existence/CDO extraction không thay runtime/cảm giác |
| Ready Or Not/Plugins | Module và integration đặc thù | Plugin enabled không có nghĩa mọi capability được dùng |
| Engine/Build/Build.version và source custom | Phiên bản Engine và cơ chế nền được dùng | Không suy ra release thương mại của Ready or Not |
| Build/runtime logs và receipts | Ca thực thi đã xảy ra | Chỉ đúng điều kiện và snapshot ghi trong receipt |
| Hai .drawio.png cung cấp | Bản đồ ý tưởng/quan hệ của người nghiên cứu | Không coi nhãn là lệnh hoặc proof implementation |

Source references trong phần gameplay thường bắt đầu bằng Source/, Config/ hoặc Plugins/: root của chúng là thư mục **Ready Or Not**. Prefix Ready Or Not/ bắt đầu từ workspace; prefix Engine/ cũng bắt đầu từ workspace. Khi số dòng đổi, tìm symbol trước, rồi so hash.

## Nguồn phương pháp

- [Wardogs-Docs](https://slimevrx.github.io/Wardogs-Docs/): phân rã gunfeel, kiểm tra animation additive/context và giá trị của nghiệm thu trực tiếp.
- [Fortnite-Docs](https://slimevrx.github.io/Fortnite-Docs/): sách theo vòng gameplay, capability, source/catalog, implementation và acceptance.
- [Paldark-Docs](https://slimevrx.github.io/Paldark-Docs/): owner/lifetime/dependency, quyết định kiến trúc và evidence.

Các bản cục bộ được đọc khi biên soạn. Nhận xét về ReadyOrNot được kiểm tra trên source ReadyOrNot, không suy từ cách Wardogs/Fortnite/Palworld triển khai.

## Phương pháp kiểm kê

Script catalog dùng filesystem walk, bỏ symlink, không theo đường dẫn ra ngoài input root. Source manifest chỉ chứa path/size/line count/hash. Dòng được đếm bằng tách newline UTF-8, gồm comment và dòng trống. C++ header không đồng nghĩa một implementation riêng; số dòng không phải thước đo chất lượng.

Plugin manifest lấy descriptor .uplugin và đối chiếu toggles trong .uproject. EnabledByDefault và projectEnabled được giữ riêng vì trạng thái tải còn phụ thuộc platform, target và dependency.

Catalog weapon dùng AssetRegistry metadata để tìm class trong /Game, kiểm tra ancestry và đọc CDO. Các default tác động đến presentation và gameplay được giữ nguyên dạng với null/unknown khi không đọc được. Nó không giả lập phép bắn để suy ra thông số.

## Phương pháp phân tích source

Với mỗi feature, sách tìm điểm vào, state writes, branch/early return, timer/delegate, implementation authoritative và đường presentation. Bảng citation ưu tiên path + symbol + line. Suy luận thiết kế được đánh dấu riêng với mệnh đề đã thấy trong source.

Không thể bảo đảm một tài liệu tĩnh bao phủ mọi nhánh của 349 nghìn dòng source cùng tất cả Blueprint. Bộ sách là một bản đồ có đường đi tới chi tiết và các câu hỏi mở; nó không tự nhận là formal verification toàn game.

## Mức kiểm chứng

Đọc README và Gun Lab để phân biệt SOURCE, CDO, BUILD, GENERATED, RUNTIME và MANUAL. Một tác vụ đo tự động cũng có thể gọi một API khác với input thật, dùng null RHI hoặc bỏ qua presentation. Receipt phải ghi các điều kiện đó để người đọc không mở rộng kết luận.

“Native pipeline” nghĩa là harness gọi lại class/cơ chế vốn có. Nó làm giảm số lớp tự viết lại; nó chưa chứng minh snapshot không có lỗi migration, asset thiếu hoặc khác bản phát hành bạn nhớ.

## Tái lập nguồn trên máy khác

1. Có cùng project/Engine và các tài nguyên cần thiết qua nguồn bạn được phép sử dụng.
2. So Engine Build.version, manifest/hash source và descriptor.
3. Build đúng target, xác minh Editor có thể load project.
4. Cài harness/generate map theo Gun Lab, giữ original defaults.
5. Chạy lại từng acceptance case, lưu receipt mới thay vì dùng kết quả máy cũ.
6. Khi snapshot khác, cập nhật nguồn và diễn giải, không chỉ chạy lại script catalog.

Không cần đưa source/asset gốc lên website để làm các bước trên.

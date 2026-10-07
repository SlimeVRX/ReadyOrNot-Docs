# Đọc một feature từ hành vi đến source

## Bắt đầu bằng một câu hỏi nhỏ

“Hệ thống súng hoạt động thế nào?” quá rộng cho một phiên học. Hãy đổi thành: **“Giữ cò khi băng đang hết đạn tạo ra những thay đổi state và feedback nào?”** Câu hỏi này có input, điều kiện và output; bạn có thể chạy lại và phản bác mô hình mình vừa vẽ.

Giữ hai cửa sổ: Editor ở một tình huống cố định và code ở đường thực thi. Đừng vừa đọc vừa thay giá trị. Ghi baseline trước khi sửa.

## Năm lượt đọc

| Lượt | Việc cần làm | Điều cần tránh |
|---|---|---|
| 1. Tìm điểm vào | Binding input, menu command, overlap/event hoặc stimulus | Chọn một hàm cùng tên ở lớp không dùng |
| 2. Tìm owner | Chỗ ghi state, điều kiện bắt đầu, timer và cancel | Chỉ đọc getter/header |
| 3. Đi qua ranh giới | C++ → Blueprint, weapon → ammo, server → client | Giả định một hàm hoàn thành toàn bộ feature |
| 4. Tìm feedback | Camera, animation, sound, VFX, HUD, AI stimulus | Lấy state logic thay cho trải nghiệm |
| 5. Đọc nhánh thất bại | Empty, interrupted, denied, missing dependency, travel | Chỉ ghi happy path |

## Một thao tác tìm kiếm có ý nghĩa

Từ một symbol đã xác định, tìm call site và writes của field liên quan. Ví dụ, khi hiểu fire interval, đừng kết luận chỉ từ property tên FireRate. Xem timer dùng nó như khoảng thời gian hay tần số; rồi xem Blueprint CDO và modifier nào tác động. Phần [Gun Gameplay](../03-Gun-Gameplay/README.md) ghi cụ thể khác biệt này trong snapshot.

```powershell
# Chạy từ workspace gốc; đây là thao tác đọc, không sửa project.
rg -n "FireRate|CalculateRecoil|ApplyRecoil" "Ready Or Not/Source/ReadyOrNot"
rg -n "RON_NO_SPRINT|FastWalk|IsSprinting" "Ready Or Not/Source/ReadyOrNot"
```

Kết quả tìm kiếm là **địa chỉ để đọc**, chưa phải phân tích. Đọc khoảng trước/sau branch, các early return, macro và nơi timer được hủy. Nếu đọc một declaration, ghi nhãn declaration; đừng mô tả nó như thân hàm đã chạy.

## Vẽ trace cùng thời gian

Mỗi hàng dưới đây là một quan sát cần thu, không phải trace tự động đã chạy sẵn:

| Mốc | Ghi gì? | Ví dụ câu hỏi |
|---|---|---|
| t0 input | device, action, pressed/released, local frame | Click có tới owning pawn không? |
| t1 accepted | weapon class, state gates, authority | Reload/obstruction có từ chối không? |
| t2 commit | ammo before/after, chamber state, shot ID nếu có | Có trừ hai lần hoặc trừ khi denied không? |
| t3 hit | trace/projectile branch, collision, damage target | Camera line và muzzle line có khác nhau không? |
| t4 presentation | montage/pose, recoil, audio/VFX onset | Feedback đến cùng frame hay trễ? |
| t5 recovery | timer reset, recoil return, action end | Khi nào input tiếp theo được phép? |

Đừng ép mọi mốc vào một frame. Một animation có thể chuẩn bị trước commit; server confirmation đến sau local presentation. Cần ghi relation thật trong snapshot và điều kiện mạng đang thử.

## Tìm Blueprint/CDO đúng lớp

Trong Content Browser, mở Blueprint class thực tế của entry đang cầm. Ghi generated class path, parent, Class Defaults và assets nó tham chiếu. Nếu property không được override ở child, đi lên parent; nếu nó bị thay ở construction hoặc runtime, ghi cả default và nơi thay.

Một tên như “MK18” có thể có bản player, suspect, deprecated hoặc mainline. Hai file có cùng ItemName không tự là cùng cấu hình. [Catalog vũ khí](../06-Catalogs/README.md) giữ package path để phân biệt.

## Giải thích đúng mức chắc chắn

Viết: “hàm X có điều kiện Y trước khi gọi Z” khi đã đọc source. Viết: “CDO của class A trả về B” khi có export. Viết: “ca RUNTIME-07 equip A và nhận active item A” khi có receipt. Đừng rút gọn cả ba thành “A hoạt động giống game gốc”.

Nếu không có symbol hoặc dữ liệu, ghi câu hỏi mở. Không điền giá trị cảm giác bằng thông số súng ngoài đời; đó không phải thông số authored của game.

## Bài tập: một lỗi, hai giả thuyết

Chọn hiện tượng “khi ADS, phát đầu làm súng nhảy khác thường”. Đưa ra ít nhất hai giả thuyết khác tầng: camera recoil khởi tạo sai và additive base/pose blending sai. Thiết kế phép đo tách chúng: khóa camera trong nhánh thực hành riêng, đo transform súng so với camera; sau đó giữ pose và đo camera. Ghi rõ nhánh thực hành đã khác native reference ở đâu.

Đạt khi bạn có thể nói **phép đo nào bác bỏ giả thuyết nào**, không chỉ “đã chỉnh thấy đỡ”.

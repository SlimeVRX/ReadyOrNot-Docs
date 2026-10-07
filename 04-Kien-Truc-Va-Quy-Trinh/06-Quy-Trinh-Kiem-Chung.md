# Quy trình phát triển: biến việc học thành kết quả có thể kiểm tra

Dự án lớn không được quản lý bằng trí nhớ của một người. Cách làm việc có thể học ngay từ một prototype là viết contract nhỏ, xác định quyền sửa, tạo fixture, đo hành vi và lưu kết quả. Khi thêm người hoặc agent, cùng contract đó giúp công việc ghép lại được.

## Một task nên chứa đủ một hành vi

Task “làm hệ thống súng giống Ready or Not” quá lớn để nghiệm thu. Task “cho weapon fixture X dùng đúng inventory equip, bắn một viên, giảm ammo một lần, phát animation/audio, observer thấy phát bắn, và có reset” có giới hạn rõ. Nó vẫn đi xuyên nhiều lớp, nên kiểm tra được tích hợp thực sự.

| Mục trong contract | Ví dụ dạng thông tin cần có |
|---|---|
| Trải nghiệm | Người chơi thử vũ khí X tại lane đã định |
| Input và guard | Fire chỉ khi item sẵn sàng, không bị khóa action |
| State owner | Weapon/inventory/authority nào ghi dữ liệu |
| Data | Blueprint, ammo, attachments, animation/audio |
| Failure/cancel | Hết đạn, đổi item, reload đang dở, chết/reset |
| Network | Owner và observer cần thấy gì |
| Measurement | Thời điểm input, shot, ammo change, impact |
| Acceptance | Scenario và kết quả cụ thể |
| Unknown | Nhánh/asset chưa xác minh |

Đây là mẫu cho project học tập; không phải mô tả mọi contract nội bộ đã có ở source gốc.

## Tách các mức bằng chứng

**Build evidence** cho biết code biên dịch/link được. **Asset evidence** cho biết class/default/reference load đúng. **Runtime evidence** cho biết một scenario đã chạy. **Parity evidence** cho biết kết quả đã được so với baseline theo metric/quan sát. Chúng có quan hệ phụ thuộc nhưng không tương đương.

Ví dụ: có hàm `FakeSurrender` là source evidence; đã mở Blueprint archetype dùng nó là asset evidence; đã kích hoạt trong fixture và lưu log là runtime evidence; tỷ lệ/timing/feedback khớp phiên tham chiếu trong tolerance mới là parity evidence.

## Phiếu chạy thử tối thiểu

```text
Scenario ID:
Ngày và build/source snapshot:
Engine / map / GameMode / net mode:
Pawn, item, attachments, config và save slot:
Seed, frame rate và điều kiện đầu:
Các bước input:
Expected:
Observed:
Log / ảnh / video local:
Pass, fail hoặc chưa đủ bằng chứng:
Sai lệch và lần sửa tiếp theo:
```

Phiếu là tài liệu gốc do người học ghi, không phải output tự sinh để tuyên bố pass. Nếu không có runtime, ghi “chưa chạy”; nếu có log load asset nhưng chưa nhấn cò, ghi “load verified”. Tính trung thực này giúp bạn học nhanh hơn vì biết thực sự còn thiếu gì.

## Chọn phép thử theo rủi ro thay đổi

Thay văn bản docs thì kiểm tra build site và link. Thay một property của fixture thì chạy case ảnh hưởng và reset. Thay item state machine thì thử equip/fire/reload/cancel với owner và observer. Thay GameMode hoặc scoring thì chạy vòng mission thành công/thất bại/retry. Thay code engine hoặc serialization thì phạm vi regression rộng hơn.

Không cần viết hàng trăm unit test lặp lại implementation cho một chỉnh sửa nhỏ. Nhưng trạng thái có nhiều nhánh, giao dịch kéo dài nhiều frame hoặc dữ liệu save cần test có ý nghĩa. Các invariant tốt gồm ammo không âm, evidence chỉ được secure một lần, end mission chỉ chốt một kết quả, cancelled action không còn timer/lock.

## Công cụ có trong source không phải bằng chứng test đã chạy

`Source/ReadyOrNot/Testing/ReadyOrNotGauntletTestController.cpp:11`, `OnInit`; `:24`, `StartProfiling`; `:55`, `OnTick` cho thấy một controller phục vụ bắt đầu phiên test/profiling và CSV capture. `ReadyOrNotSpinTestController.cpp` có thêm logic khảo sát và lưu thông tin hiệu năng, đồng thời có các đoạn profiling được comment. Không được diễn giải sự tồn tại của folder Testing thành độ phủ test toàn game.

Với prototype, nên có một smoke path ngắn từ spawn tới item usable, một test vòng mission và một fixture performance ổn định. Log nên ghi actor/request/scenario id để phân biệt hai lần chạy. Thêm video ở những chỗ số đo không mô tả đủ feel: camera motion, pose, sound layering và timing.

## Học cách phối hợp nhiều phần việc

Chia quyền sửa theo contract cụ thể: một người làm weapon behavior, một người làm animation data, một người làm lane/target, một người làm kết quả. Họ cần thống nhất identity, event payload và acceptance trước khi viết. Nếu hai phần đều có quyền sửa ammo hoặc mission result, phải giải quyết ownership trước khi tích hợp.

Giữ docs công khai chứa giải thích gốc, source paths/symbols và công cụ học tập do mình viết. Không cần đưa toàn bộ source engine/game hoặc binary asset vào repo docs để người đọc hiểu kiến trúc. Asset thật vẫn được tham chiếu trong môi trường local đã có. Điều này cũng làm repo tài liệu nhỏ và dễ kiểm tra thay đổi.

**Bài tập:** lấy một bug “client thấy súng cũ sau khi đổi item”, viết expected/observed, chỉ ra `UInventoryComponent::OnRep_ItemChangeRequest` (`Components/InventoryComponent.cpp:698`) và các điểm local/TP draw cần kiểm tra; tạo fixture tái hiện và chỉ sửa sau khi quan sát. **Tiêu chí đạt:** người khác chạy được cùng steps, thấy cùng failure và xác minh được fix mà không cần đọc toàn bộ cuộc trò chuyện.

**Chưa xác minh:** CI gốc, mọi automation suite, packaged build, performance budget theo phần cứng và parity toàn game. Các chương hướng dẫn quy trình cần thực hiện; chúng không thay cho receipt của một lần chạy thực.

# Gun Lab trong chính Ready or Not

Lab dùng project và custom engine hiện có. Pawn, controller, inventory, gun Blueprint, đạn, animation, âm thanh, ADS, recoil và reload đều đi qua hệ thống native. Plugin bổ sung chỉ chọn súng, cấp magazine cho buổi thử, bố trí khu thử và ghi kết quả kiểm tra.

Map cục bộ: `/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab`. Các file `.umap`, `.uasset`, engine và source thương mại không nằm trong repo tài liệu. Repo chỉ cung cấp mã harness mới viết và script dựng map trên bản project người học có quyền sử dụng.

## Bắt đầu

1. Đọc [cách dựng và kiểm tra](./setup.md).
2. Mở map bằng custom Unreal Editor, chọn Play trong viewport, rồi bấm vào viewport để nhận input. `Shift+F1` trả chuột về editor.
3. Chọn một lane, thử một magazine với cấu hình mặc định rồi ghi kết quả theo [ma trận thử](./test-matrix.md).
4. Tra tên lớp trong [catalog súng](../06-Catalogs/weapons.json). Một Blueprint variant không đồng nghĩa một mẫu súng độc lập hay một vũ khí đã phát hành.

| Điều khiển lab | Tác dụng |
|---|---|
| F5 / F6 | Súng trước / sau trong danh sách lớp native |
| F7 | Cấp lại đạn qua hàm virtual native, chỉ khi người dùng yêu cầu |
| F8 | Trở về firing line, nhìn theo trục lane |
| F9 | Chạy kiểm tra lần lượt spawn → ownership → inventory → equip |
| F10 | Hiện / ẩn overlay |
| `ronlab select 12` | Chọn mục thứ 12, số bắt đầu từ 1 |
| `ronlab next`, `prev`, `refill`, `reset`, `audit` | Lệnh console tương ứng |

Fire, ADS, reload, fire selector, nghiêng người và đi lại giữ binding của project. Config mặc định có chuột trái để bắn, `X` đổi chế độ bắn; tùy chọn cá nhân có thể ghi đè config. Lab không thay sensitivity hay key binding hiện tại.

F5–F7 cũng là các phím chọn tổ AI trong config native; range hiện không spawn tổ AI. Có thể dùng các lệnh `ronlab` khi muốn tránh kích hoạt đồng thời binding đó.

## Những gì baseline giữ lại

Harness gọi `SpawnActor` với lớp Blueprint thật, `AddInventoryItem`, rồi `PutItemInHands` như development Equip của project. Chuyển súng đi qua draw/holster native; đồ cũ do lab tạo chỉ được dọn khi native equip hoàn tất. Lab yêu cầu bốn magazine qua `SetMagazineCount`, dùng loại đạn đầu tiên được asset khai báo; override của từng subclass quyết định kết quả thực tế. Pepperball dùng hopper, Taser và shotgun có logic riêng. Không bật auto refill, infinite ammo hay sửa fire rate/recoil. Attachment mặc định của Blueprint được giữ nguyên, không áp thêm preset do lab tự tạo. Các tùy chọn native của người chơi vẫn có thể tác động trạng thái súng.

Các lane cách firing line 5, 10, 25, 50 và 100 mét theo đơn vị Unreal 100 cm/m. Các bảng ở lane khác nhau, cần đứng đúng vạch để khoảng cách đo theo lane có ý nghĩa. Có cover thấp/cao và một panel geometry mỏng. Panel dùng physical surface mặc định; đây chưa phải thí nghiệm xuyên vật liệu đã hiệu chuẩn.

## Đọc bằng chứng đúng mức

- **Compile:** plugin C++ được UnrealBuildTool biên dịch và link.
- **Generate:** Python tạo/save `.umap` và ghi `generation_receipt.json`.
- **Equip audit:** runtime thực sự có `PlayerCharacter`, súng có owner đúng, inventory giữ súng yêu cầu và animation blocking kết thúc. [Receipt đầy đủ](../lab/manifests/equip_audit_receipt.json) được lưu riêng với action probe.
- **Action probe:** runtime đọc trạng thái ADS và ammo trước/sau lệnh bắn, reload. [Receipt sáu cấu hình](../lab/manifests/action_probe_receipt.json) ghi từng giá trị quan sát được; equip audit không tự chứng minh các thao tác này.
- **Presentation và cảm giác:** camera, audio, hit feedback và thao tác của người chơi cần kiểm tra thêm theo ma trận. [Biên bản quan sát ảnh range](../lab/manifests/visual_receipt.json) chỉ xác nhận những gì nhìn thấy trong một ảnh render.

Snapshot project hiện có cảnh báo migration: một số asset `CameraAnim` cũ không load được, và một số tham chiếu âm thanh/vật liệu không tồn tại. Native pipeline có thể đúng trong khi presentation chưa hoàn chỉnh. Không nên gọi đây là xác nhận giống 100% bản game phát hành.

## Kết quả đã quan sát ngày 07/10/2026

Custom UE 5.3.2 đã compile/link plugin và tạo map cục bộ gồm 46 actor. Asset Registry quét toàn bộ `/Game` với 136.799 asset, xác định 102 Blueprint ứng viên thuộc họ `BaseMagazineWeapon`, bao gồm súng thử nghiệm ngoài thư mục Items và các lớp less-lethal. Knife và sledgehammer thuộc họ melee được ghi riêng, không tính thành súng.

Lần audit `20261007-012533` có đủ **102/102 hàng**: **80 equip thông thường**, **21 equip dùng nhánh instant native có đánh dấu**, và **1 lớp nền M320 bị loại vì cờ Abstract**. Không có timeout equip trong lần chạy này. Ba lớp M320 Bang/Gas/Stinger cụ thể đều equip được. Đây là số Blueprint/variant, không phải 102 mẫu súng thương mại khác nhau.

21 hàng fallback xảy ra khi asset súng đang rời không có `Holster.Body_FP`; harness dùng tham số `bInstant` có sẵn của inventory để hoàn tất chuyển súng. Không xem các hàng này là kiểm chứng thành công cho holster first-person thông thường. [Chi tiết setup và giới hạn](./setup.md) giải thích cách receipt phân biệt hai trường hợp.

Action probe `20261007-115320` thử sáu cấu hình với renderer thật. Cả sáu có owner đúng, vào trạng thái aiming và tiêu đạn. Mỗi probe kéo dài khoảng 12 giây sau equip; yêu cầu reload ở giây thứ 3, đọc kết quả ở giây thứ 12, tức khoảng 9 giây quan sát sau yêu cầu reload.

| Lab # | Cấu hình | Trước bắn | Sau bắn | Cuối probe | Có bổ sung đạn sau reload |
|---|---|---:|---:|---:|---|
| 1 | 870mcs | 8 | 7 | 8 | Có |
| 54 | G19 V2 | 15 | 14 | 16 | Có |
| 67 | Taser V2 | 1 | 0 | 1 | Có |
| 33 | Pepperball MLO | 200 | 199 | 199 | Chưa quan sát được |
| 75 | M320 Flash / Launcher_M320_Bang | 1 | 0 | 1 | Có |
| 40 | SR16 | 30 | 29 | 31 | Có |

Pepperball có `magazines=4`, `native_can_reload_before_request=true` và đã nhận yêu cầu reload, nhưng ammo vẫn là 199 cuối probe. Chưa xác định nguyên nhân; kết quả này không chứng minh reload thành công hoặc gate đã từ chối. Có thể dùng F7 để cấp lại đạn khi thử thủ công, nhưng không tính F7 là reload. G19 và SR16 lên lần lượt 16 và 31 viên là số đo native cần đối chiếu với [chamber/reload](../03-Gun-Gameplay/03-Fire-ammo-chamber.md), không ép về dung lượng danh nghĩa.

Ảnh `Saved/GunLab/Range.png` sau batch cho thấy SR16 trên tay, range và nhãn khoảng cách đúng chiều; shader/asset compilation đã hết trước khi chụp. Ảnh giữ cục bộ trong project. Capture tắt âm thanh. Chưa có xác nhận âm thanh, impact, đường đạn, animation theo thời gian hoặc cảm giác tương đồng bản retail; tiếp tục bằng [ma trận thử thủ công](./test-matrix.md).

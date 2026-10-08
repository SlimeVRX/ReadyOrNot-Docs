# Gun Lab trong chính Ready or Not

Lab dùng project và custom engine hiện có. Pawn, controller, inventory, gun Blueprint, đạn, animation, âm thanh, ADS, recoil và reload đều đi qua hệ thống native. Harness nối loadout, bố trí khu thử, cung cấp hướng dẫn và ghi kết quả; công cụ compatibility phục hồi những liên kết camera và event có bằng chứng trong snapshot.

Map cục bộ: `/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab`. Các file `.umap`, `.uasset`, engine và source thương mại không nằm trong repo tài liệu. Repo chỉ cung cấp mã harness mới viết và script dựng map trên bản project người học có quyền sử dụng.

## Bắt đầu

**Bản nâng cấp native gunplay:** đọc [hệ thống, phím điều khiển và 26 bài thử đầy đủ](./native-systems-and-test-guide.md). F1 mở hướng dẫn ngay trong game; bảng trạng thái cho biết chế độ bắn hiện tại và các mode thực sự có trên súng. Tâm ngắm hỗ trợ thử dùng widget `CrossHairOverlay` và texture `HUD_Reticle` có sẵn, chỉnh layout trong phiên để hiển thị chấm nhỏ ở tâm; tắt/bật bằng `ronlab crosshair`. Nó không đổi hướng ngắm hoặc đường đạn.

Kiểm chứng ngày 08/10: **102 equip, 101 action qua bốn phiên**, **38/38 điều kiện input**, và đối chứng có renderer cho 16 cấu hình. Cả 40 cấu hình loadout người chơi đã ADS/tiêu đạn; sáu shotgun cần đối chứng có renderer để quan sát reload. [Xem kết quả, nguồn từng phiên và giới hạn](./runtime-results.md).

1. Đọc [cách dựng và kiểm tra](./setup.md).
2. Mở map bằng custom Unreal Editor, chọn Play trong viewport, rồi bấm vào viewport để nhận input. `Shift+F1` trả chuột về editor.
3. Chọn một lane, thử một magazine với cấu hình mặc định rồi ghi kết quả theo [ma trận thử](./test-matrix.md).
4. Tra tên lớp trong [catalog súng](../06-Catalogs/weapons.json). Một Blueprint variant không đồng nghĩa một mẫu súng độc lập hay một vũ khí đã phát hành.

| Điều khiển lab | Tác dụng |
|---|---|
| F1 | Hướng dẫn phím đang dùng và checklist |
| F5 / F6 | Súng trước / sau trong danh sách lớp native |
| F7 | Cấp lại đạn qua hàm virtual native, chỉ khi người dùng yêu cầu |
| F8 | Trở về firing line, nhìn theo trục lane |
| F9 | Chạy kiểm tra lần lượt spawn → ownership → inventory → equip |
| F10 | Hiện / ẩn overlay |
| `ronlab select 12` | Chọn mục thứ 12, số bắt đầu từ 1 |
| `ronlab next`, `prev`, `refill`, `reset`, `audit` | Lệnh console tương ứng |
| `ronlab loadout` | Mở UI trang bị native tại station; UI lưu lựa chọn như game |
| `ronlab attachments optics` / `ronlab attachment optics 1` | Xem / áp attachment từ danh sách tương thích native |
| `ronlab ammo` / `ronlab ammo 1` | Xem / chọn loại đạn native cho cấu hình hiện tại |
| `ronlab_targets reset` | Dựng lại mục tiêu qua AI spawner native |

Fire, ADS, reload, fire selector, nghiêng người và đi lại giữ binding của project. Config mặc định có chuột trái để bắn, `X` đổi chế độ bắn; tùy chọn cá nhân có thể ghi đè config. Lab không thay sensitivity hay key binding hiện tại.

F5–F7 cũng là các phím chọn tổ AI trong config native; range hiện không spawn tổ AI. Có thể dùng các lệnh `ronlab` khi muốn tránh kích hoạt đồng thời binding đó.

## Những hệ thống native được nối vào lab

Súng primary/secondary có trong loadout đi qua `ReadyOrNotLoadoutManager`, `DestroyAllEquippedItems` và `EquipLoadoutOnPlayer`, theo cùng trình tự của bàn trang bị native. Slot súng, ammo đã chọn, attachment, armor và gear được áp cùng nhau. Chọn cấu hình khác bằng F5/F6 là thay loadout; đổi bằng 1/2 giữ hai súng đang có và số đạn của chúng. Lựa chọn nhanh trong lab chỉ đồng bộ PlayerState trong bộ nhớ; thao tác trong UI loadout gốc lưu preset như game.

Lớp suspect/test/thiết bị ngoài loadout vẫn dùng đường xem asset riêng và được ghi `catalog_asset`. Chúng không đại diện cho một loadout SWAT hoàn chỉnh. Lab giữ fire rate, recoil, ADS, reload, chamber, animation và âm thanh của game; không có auto refill hoặc ammo vô hạn. Pepperball, Taser và shotgun tiếp tục dùng override native của từng subclass.

Các lane cách firing line 5, 10, 25, 50 và 100 mét theo đơn vị Unreal 100 cm/m. Đứng đúng vạch tương ứng. Khu phía sau vạch bắn bổ sung loadout/ammo station, mục tiêu AI có damage/armor native, panel vật liệu dùng physical material của project, cửa native và phòng CQB tối. Fixture cô lập từng trải nghiệm để đối chiếu; mục tiêu debug đứng yên không thay thế bài kiểm tra combat AI đầy đủ.

## Đọc bằng chứng đúng mức

- **Compile:** plugin C++ được UnrealBuildTool biên dịch và link.
- **Generate:** Python tạo/save `.umap` và ghi `generation_receipt.json`.
- **Equip audit:** runtime thực sự có `PlayerCharacter`, súng có owner đúng, inventory giữ súng yêu cầu và animation blocking kết thúc. [Receipt đầy đủ](../lab/manifests/equip_audit_receipt.json) được lưu riêng với action probe.
- **Action probe:** runtime đọc trạng thái ADS và ammo trước/sau lệnh bắn, reload. [Receipt đo thao tác](../lab/manifests/action_probe_receipt.json) ghi từng giá trị quan sát được; equip audit không tự chứng minh các thao tác này.
- **Theo từng vũ khí:** [bảng kết quả runtime](./runtime-results.md) ghép đúng class/index, tách đường loadout native và asset catalog, đồng thời giữ nguyên những quan sát chưa đạt.
- **Presentation và cảm giác:** camera, audio, hit feedback và thao tác của người chơi cần kiểm tra thêm theo ma trận. [Biên bản ảnh baseline ngày 07/10](../lab/manifests/history/20261007/visual_receipt.json) chỉ xác nhận những gì nhìn thấy trong ảnh của lượt cũ. Kết quả nâng cấp nằm trong [hướng dẫn kiểm chứng](./native-systems-and-test-guide.md#ket-qua-kiem-tra-hien-tai).

Lab đã chuyển các camera curve hợp lệ từ định dạng cũ sang UE5 và nối lại vào camera shake gốc. [Audit trình diễn](./native-presentation-audit.md) ghi phương pháp, sai số và camera bị loại. Snapshot vẫn có tham chiếu âm thanh/vật liệu không tồn tại; native pipeline có thể đúng trong khi một phần presentation còn thiếu. Không coi đây là xác nhận giống 100% bản game phát hành.

## Kết quả lịch sử của bản đầu ngày 07/10/2026

Các số dưới đây thuộc harness trước khi tích hợp loadout đầy đủ. Không dùng chúng để kết luận bản nâng cấp đã vượt qua mọi bài thử. Xem [hướng dẫn và mức kiểm chứng của bản nâng cấp](./native-systems-and-test-guide.md).

Custom UE 5.3.2 đã compile/link plugin và tạo map cục bộ gồm 46 actor. Asset Registry quét toàn bộ `/Game` với 136.799 asset, xác định 102 Blueprint ứng viên thuộc họ `BaseMagazineWeapon`, bao gồm súng thử nghiệm ngoài thư mục Items và các lớp less-lethal. Knife và sledgehammer thuộc họ melee được ghi riêng, không tính thành súng.

[Lần audit lịch sử `20261007-012533`](../lab/manifests/history/20261007/equip_audit_receipt.json) có đủ **102/102 hàng**: **80 equip thông thường**, **21 equip dùng nhánh instant native có đánh dấu**, và **1 lớp nền M320 bị loại vì cờ Abstract**. Không có timeout equip trong lần chạy này. Ba lớp M320 Bang/Gas/Stinger cụ thể đều equip được. Đây là số Blueprint/variant, không phải 102 mẫu súng thương mại khác nhau.

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

Pepperball có `magazines=4`, `native_can_reload_before_request=true` và đã nhận yêu cầu reload, nhưng ammo vẫn là 199 cuối probe. Phân tích asset bổ sung phát hiện MLO chưa được gán montage reload; [hướng dẫn MLO và TAC700](./native-systems-and-test-guide.md) giải thích vì sao CanReload không chứng minh reload hoàn tất. Có thể dùng F7 để cấp lại đạn khi thử thủ công, nhưng không tính F7 là reload. G19 và SR16 lên lần lượt 16 và 31 viên là số đo native cần đối chiếu với [chamber/reload](../03-Gun-Gameplay/03-Fire-ammo-chamber.md), không ép về dung lượng danh nghĩa.

Ảnh `Saved/GunLab/Range.png` sau batch cho thấy SR16 trên tay, range và nhãn khoảng cách đúng chiều; shader/asset compilation đã hết trước khi chụp. Ảnh giữ cục bộ trong project. Capture tắt âm thanh. Chưa có xác nhận âm thanh, impact, đường đạn, animation theo thời gian hoặc cảm giác tương đồng bản retail; tiếp tục bằng [ma trận thử thủ công](./test-matrix.md).

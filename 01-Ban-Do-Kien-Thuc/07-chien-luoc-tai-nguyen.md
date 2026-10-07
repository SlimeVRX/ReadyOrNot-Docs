# Dùng cả kho tài nguyên mà không copy mù

Kho hiện có gồm source game, custom Engine, Blueprint, mesh, animation, material, VFX, audio bank/raw audio, data table, config, level và công cụ nội bộ. “Dùng tất cả để học” nên hiểu là **biết từng loại tài nguyên phục vụ câu hỏi nào và cách nối bằng chứng**, không phải nhập toàn bộ vào một project rỗng ngay lập tức.

## Bốn loại dữ liệu cần phân biệt

| Loại | Ví dụ | Cách nghiên cứu |
|---|---|---|
| Luật/implementation | C++, BP graph, script | Đọc branch, state writes, calls, authority và lifetime |
| Dữ liệu authored | CDO, data table, animation curves, mesh sockets | Export hoặc đọc trong Editor; lưu class/package và inheritance |
| Trình diễn | animation, sound event/bank, VFX, material, UI | Đối chiếu lúc và nơi nó được phát trong gameplay |
| Dữ liệu dẫn xuất | DDC, shader cache, compressed animation, asset registry cache | Hiểu quá trình tạo; không coi là nguồn chuẩn để thiết kế |

Config có thể nằm ở ranh giới: vừa chọn implementation (GameMode, pawn, platform), vừa cung cấp tuning. Luôn ghi hierarchy và override; một dòng DefaultEngine.ini không đủ giải thích mọi giá trị runtime.

## Sổ phụ thuộc của một khẩu

Cho mỗi weapon Blueprint, ghi:

1. Package, generated class, native parent, Blueprint parent và leaf/non-leaf.
2. ItemName/ItemClass, ammo types/data row, magazine defaults, fire interval/fire modes.
3. Animation data, mesh/skeleton, sockets, equipped/ADS state, montage/notify và graph.
4. Camera/recoil/spread/obstruction settings và nơi runtime có thể thay đổi.
5. SoundData/FMOD event, muzzle/impact/casing VFX, material và decal.
6. Attachment defaults và cách attachment có thể sửa stats/presentation.
7. Tình trạng load/equip/fire/reload đã thử; lỗi và dependencies còn thiếu.

Catalog của sách mới trả lời được phần đã xuất từ metadata/CDO và các ca runtime được ghi nhận. Nó không tự giải mã toàn bộ AnimGraph hoặc chứng minh mọi dependency trọn vẹn.

## Khi chuyển sang project học tập riêng

Dùng AssetTools/Migrate có kiểm soát hoặc công cụ tương đương để giữ dependency, nhưng coi migration thành công là **copy asset graph**, chưa phải port feature. Candidate còn cần class native tương thích, enum/struct/data layout, module/plugin, config, collision channel, physical material, input, animation instance và gameplay context.

Thử theo một chuỗi ngắn: một rig → một idle/equip → một ADS/fire → một reload → một ammo type → một target. Mỗi lần chạy trong project riêng, lưu lại dependency nào đã thay thế và vì sao. Không dùng hàng loạt dummy class để làm asset load rồi kết luận gameplay đã phục hồi.

## Blueprint chứa gì chưa thấy qua CDO?

CDO cho biết default values và object references. Nó không thay cho graph thực thi, construction behavior, timeline, notify logic hoặc runtime mutation. Nếu một animation chạy khác, cần đọc graph/slots/layer/curve và nơi native cập nhật anim inputs.

Ở snapshot này có cảnh báo migration/asset dependencies trong quá trình mở/inspect. Bảng lỗi cần chỉ rõ ảnh hưởng quan sát được; đừng suy ra mọi warning là fatal, cũng đừng bỏ qua một CameraAnim không load rồi khẳng định camera parity đầy đủ.

## Học level và AI từ dữ liệu không gian

Map không chỉ là static mesh. Nó có player start, game mode/world settings, navigation, volumes, doors, cover, spawn rules, objectives, audio zones, lighting và streaming. Một AI class đúng nhưng world thiếu nav/door topology vẫn có thể đứng yên hoặc ra quyết định sai.

Native Gun Lab chủ động cô lập nhiều lớp nhiệm vụ để thử súng. Khi chuyển sang học tactics/AI, tạo fixture riêng cho cửa, perception, morale và đội hình; đừng dùng kết quả ở sân bắn trống làm chứng nhận hành vi trong phòng hẹp.

## Cách giữ provenance

Với mỗi dữ liệu đã rút ra, ghi file/package và phép đọc: filesystem inventory, AssetRegistry metadata, loaded CDO, source inspection hay runtime receipt. Hash nguồn giúp phát hiện snapshot đổi, nhưng không tự chứng minh nội dung đã được hiểu đúng.

Script catalog trong repo chỉ ghi metadata. Website không cần chứa 135 nghìn asset để bạn tra được đúng asset trên máy. Thiết kế tốt là **tài liệu gọn, địa chỉ chính xác, bài thực hành tái lập được**.

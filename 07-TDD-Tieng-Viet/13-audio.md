# 13. Âm thanh: khoảng cách nghe theo đường đi

> **Phạm vi nguồn:** diễn giải TDD R&D độc lập ngày **27/09/2026**, §13, tr.79–82. TDD dựa trên reverse engineering; không phải tài liệu chính thức VOID, không xác minh bản bán lẻ mới nhất. Mọi `file:line` là **trích dẫn do TDD cung cấp**, không phải kết quả kiểm chứng mới. Đường dẫn tương đối mặc định dưới `Source/ReadyOrNot/`.

## Âm thanh là thông tin chiến thuật

Trong không gian CQB, âm thanh giúp người chơi suy luận có người sau tường, phía trên hay quanh góc. Khoảng cách thẳng từ nguồn đến tai không đủ: một phát súng ở phòng sát cạnh có thể phải đi qua hành lang và cửa mới tới người nghe. TDD mô tả hệ thống vừa tính quãng đường lan truyền vừa làm âm thanh nghẹt hơn ở các lần rẽ, cửa đóng và vật cản. Phụ đề cũng chịu điều kiện nghe được để không vô tình cung cấp tri thức mà âm thanh không truyền tới người chơi.

Checkout được nghiên cứu dùng FMOD Studio **2.00.08**, với thống kê từ project: 3.036 event, 36 bank, 71 mixer group, 17 snapshot, 513 parameter và 36 effect preset. Đây là ảnh chụp nội dung trong nguồn, không phải danh mục của mọi bản phát hành. Bank chia theo màn chơi và miền chức năng như SWAT, Weapons, CivSus, Interactions, UI, Global Amb, Dialogue, Modding, Legacy, Master; mục tiêu là mỗi map chỉ nạp phần cần thiết. Nguồn: §13, tr.79.

## Những thành phần và dòng dữ liệu

`USoundManager` là world subsystem xây đồ thị phòng/cổng, trả lời truy vấn đường đi và quản lý pool `USoundSource`. Mỗi source bọc một FMOD event instance, cập nhật `PropagateDistance` và `Occlusion` theo chu kỳ riêng. Source còn hỗ trợ âm thanh cha/con và programmer sound dùng cho VO. Âm con như các lớp đuôi tiếng súng có thể nhận giá trị lan truyền từ cha thay vì tính lại.

`ARoomVolume` và `APortalVolume` mô tả topology âm học do designer đặt. Phòng có `RoomGroupID`; cổng đại diện cửa, cửa sổ, kính có thể vỡ, lối mở ngang/dọc, trong/ngoài nhà. `AReadyOrNotAudioVolume` xử lý reverb và room tone tại vị trí người nghe. `UMusicManager` đặt tham số nhạc tương tác; hệ thống voice, conversation và subtitle tiếp nhận lời thoại và ngữ cảnh.

Mixer tách SFX, giọng nói góc nhìn thứ nhất/thứ ba, radio, command, ambience theo level, Music, UI và reverb. Các VCA Master/Music/SFX/UI phục vụ thanh âm lượng. Snapshot gồm loại phòng Hall, Small, Mid, Corridor, Bathroom, Street, Cave, Forest; vật liệu WOOD, CONCRETE, FABRIC, METAL; cùng StunClose/StunFar và Damage. Không nên đánh đồng snapshot với trạng thái gameplay có thẩm quyền: đó là cách phát lại thông tin trên máy người nghe.

## Reverb theo phòng và đồ thị lan truyền

Theo `Actors/Environment/ReadyOrNotAudioVolume.cpp:47-139`, mỗi **0,5 giây** volume kiểm tra người chơi local trong AABB. Khi vào, nó bật event reverb 2D và ambient gắn kèm; khi ra thì dừng. Nếu volume lân cận đang dùng cùng event, instance được bàn giao thay vì khởi động lại, tránh tiếng bật/tắt đột ngột. TDD cảnh báo AABB có thể không khớp brush xoay hoặc hình không phải hộp (`:86`); hình hiển thị của volume không bảo đảm đúng miền kiểm tra.

Đồ thị truyền âm coi **portal là node**. Mọi portal thuộc cùng nhóm phòng nối với nhau thành clique; các portal ngoài trời cũng nối nhau. Với mỗi source cần cập nhật, hệ thống thêm nguồn và người nghe làm node tạm. Cùng nhóm phòng thì dùng đường thẳng; khác nhóm thì tìm ba đường:

1. Đường ngắn nhất.
2. Đường thay thế, loại portal đầu của đường ngắn nhất.
3. Đường “không bị chặn”, tránh cửa đóng và kính chưa vỡ.

`PropagateDistance` là chiều dài đường ngắn nhất, gửi cho FMOD bằng **mét**. Những âm thanh ở góc nhìn thứ nhất được bỏ qua bước truyền theo graph này. Nguồn: §13, tr.80; `Info/SoundManager.cpp:119-330`; `Actors/Sound/SoundSource.cpp:292-615`.

Trong chế độ angular, mỗi lần rẽ đóng góp:

`(1 − (cosθ + 1)/2) × AngleOcclusionMultiplier`.

Góc rẽ càng gắt thì phần nghẹt càng lớn. Portal có toàn bộ cửa đóng cộng `DoorOcclusionAdditive`; đoạn ngoài trời tăng occlusion theo quãng đường. Ba đường được pha bằng trọng số `1/d²`; một async line trace nguồn–tai thêm thành phần obstruction. Đừng tự suy ra đây là mô phỏng sóng vật lý đầy đủ: nó là mô hình đồ thị và tham số hóa nhằm giữ thông tin không gian với chi phí có thể kiểm soát.

Chế độ khác, `OT_Depth`, trace xuôi và ngược để ước lượng độ dày từng component chắn. Đóng góp là `depth / FullOcclusionDepth(material)`, nhân hệ số của cửa. Tag `NoOcclusion` bỏ component khỏi phép che; `NoSound` làm câm âm. Implementation truyền theo nav-path trong `FMODAudioPropagationComponent.cpp:50-105` được TDD ghi là **đã comment out**, không phải tuyến runtime bổ sung còn hoạt động.

## Tiếng súng, vật liệu và nhạc

`FWeaponSoundData` giữ event riêng cho 1P/3P, trong/ngoài nhà, nạp đạn, kiểm tra, bolt, fire-select, dry-fire, viên cuối, whizz, hit/kill marker. Khi bắn, code đặt `IsOutside`, `IsSupressed` và `FireMode` (`Actors/BaseMagazineWeapon.cpp:3037-3143`). Việc xác định ngoài nhà dựa trên nội dung được gắn tag `IsOutside` ở mesh sàn, được character, grenade và portal đọc. Đây không phải phép phân loại hình học tự động.

Footstep, impact và vỏ đạn dùng tham số `Surface` từ physical material; `FootstepFoleyVolume` có thể override theo vùng. TDD tìm thấy `gunshotNumbers.txt` liệt kê loại đạn và loại phòng nhưng **không tìm thấy C++ đặt tham số “Rooms”**; Blueprint có đặt hay không vẫn chưa xác minh. Không nên dùng sự tồn tại của tên tham số làm bằng chứng tính năng đã nối vào runtime.

Nhạc có một event theo level từ `LevelScript->MusicData`. Mỗi giây, `MusicManager.cpp:61-240` đặt Combat/Combat Tension thành 1 nếu có bất kỳ AI nào bắn trong **15 giây** vừa qua; Ambient Tension thành 1 nếu SWAT AI breach trong **10 giây**; Entry_DistanceFromDoor thành 1 nếu người chơi cách cửa tối đa **10 m**. Health và Win/Lose cũng được cập nhật. Các tham số riêng từng map có thể do Blueprint điều khiển, nhưng đây là **suy luận của TDD**. Cờ combat toàn cục không xét nơi giao tranh hay khả năng người chơi nghe thấy.

## Pipeline lời thoại và phụ đề

TDD ghi 158 speaker, với WAV rời trong `Content/VO/<Speaker>/`, được stage kiểu non-UFS. Tên file theo nhóm `[BARK]`, `[TELL]`, `[REPLY]`, `[CALL]` và loại câu/số biến thể. `UReadyOrNotVoiceConfig` ánh xạ speaker và loại câu; `FSequencedVOLookup` là shuffle bag: chỉ lặp lại sau khi hết các biến thể trong lượt. Cooldown trong ini gồm `[CALL]Pain` 8 giây, `[BARK]Pain` 8 giây, `[BARK]Death` 4 giây.

VO chạy qua programmer sound của `USoundSource`, do đó chịu cùng propagation/occlusion. Phụ đề được bật khi **occlusion ≤ 0,8 và khoảng cách truyền ≤ 15 m**, ngoại trừ lời qua radio (`ReadyOrNotCharacter.cpp:5663-5674`). Cặp TELL/REPLY phục vụ hội thoại nền; `ConversationManager` có quy tắc tag người nói, khoảng cách, LOS, speaker tùy chọn và hoạt động trước/sau hội thoại.

Script trong `Assets/build-scripts/` dùng Whisper large-v2 để tạo `sub_en.csv`, xử lý dịch, chuyển định dạng console qua fsbank và build bank bằng FMOD CLI. Đây là mô tả công cụ của nguồn; chương này không chạy script, không tái xuất bản VO hay nội dung dịch.

## Authority, giới hạn và bài thực hành tái tạo

Âm thanh được tính **phía client theo người nghe của client**. Gameplay như bắn, cửa, VO được replicate hoặc multicast để kích hoạt phát cục bộ; tham số nhạc đọc GameState đã replicate. Kết quả occlusion của một tai nghe không nên trở thành nguồn quyết định gameplay toàn cục. TDD cũng lưu ý topology âm học đặt tay chưa được sinh từ room data của `WorldDataGenerator`; việc dùng chung dữ liệu là cơ hội cải tiến được suy luận, không phải khả năng đã tồn tại.

**Proposed — phụ thuộc:** ba phòng nối bằng hai cửa, một cửa sổ vỡ được, dữ liệu phòng, một emitter và hai listener; dùng FMOD hoặc bộ phát âm do dự án riêng lựa chọn.

**Bài thực hành tái tạo:** dựng graph portal, ghi log đường đi và đơn vị mét; triển khai angular occlusion, rồi thêm trạng thái cửa/kính. Tách lịch cập nhật graph, source và reverb. Cho subtitle dùng đúng cùng kết quả audibility với VO. Sinh portal từ cùng dữ liệu phòng đã bake và thay AABB bằng phép containment đúng hình volume.

**Tiêu chí đạt:** đóng cửa làm thay đổi đường hoặc occlusion; phá kính mở lại đường; cùng phòng ưu tiên đường trực tiếp; âm con theo cha; hai client ở phòng khác nhau nghe/phụ đề khác nhau một cách hợp lý; chuyển hai vùng cùng reverb không restart event; kiểm tra riêng biên 0,8 và 15 m. Nhạc đề xuất dùng mức nguy cơ và audibility tại người chơi thay cờ bắn toàn cục. Các bài này là thiết kế mới để học, chưa phải xác nhận hành vi retail.

Đọc tiếp: [Hậu kỳ, hoạt ảnh và phụ đề](./14-presentation-feel.md), [pipeline màn chơi](./12-level-design-pipeline.md).

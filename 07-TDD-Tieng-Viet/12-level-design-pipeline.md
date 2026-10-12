# 12. Quy trình xây dựng màn chơi

> **Phạm vi nguồn:** bản diễn giải học tập từ TDD R&D độc lập, reverse-engineered, đề ngày **27/09/2026**, §12, tr.76–78. Đây không phải tài liệu chính thức của VOID và không xác nhận hành vi của bản bán lẻ mới nhất. Các địa chỉ `file:line` dưới đây được **TDD cung cấp**, chưa được chương này kiểm chứng lại trong source. Đường dẫn tương đối thuộc `Source/ReadyOrNot/`, trừ khi ghi `Content/`, `Docs/` hoặc `Plugins/`.

## Màn chơi là một không gian có nhiều cấu hình

TDD mô tả nhiệm vụ như một bài toán chiến thuật trong không gian hẹp: người chơi học cấu trúc tòa nhà nhưng không biết chắc lần tiếp theo ai sẽ xuất hiện, cửa nào khóa, cửa nào có bẫy hay tuyến nào bị đồ vật chặn. Giá trị tái chơi đến từ sự kết hợp giữa kiến trúc ổn định và biến thể được tác giả màn chơi kiểm soát. Điều này khác với việc sinh ngẫu nhiên toàn bộ tòa nhà: các lựa chọn vẫn thuộc danh sách đã được thiết kế và kiểm tra trước.

Một bản đồ được chia thành các sublevel theo chuyên môn: `Blockout`, `Art`, `Arch`, `Props`, `Lighting`, `Audio`, `AI`, `Cine`, `Core`, thường nằm dưới `Content/ReadyOrNot/Level/RoN_<Map>/Levels/Editable/`. TDD **suy luận** rằng cách chia này giảm tranh chấp khóa độc quyền khi nhiều người dùng Perforce. Lớp nhiệm vụ nằm riêng dưới `Missions/RoN_<Map>_<Mode>_Core.umap`, chứa các actor phục vụ chế độ như BarricadedSuspects, HostageRescue, Raid, ActiveShooter hoặc BombThreat. Đây là package thực sự được yêu cầu tải.

Ví dụ trong TDD, lớp HostageRescue của Hospital tham chiếu `RoN_Hospital_Core`, `_Art`, `_Audio` theo kiểu `AlwaysLoaded`, bên cạnh các lớp streaming động. Vì vậy cần phân biệt “bản đồ đang mở” với cả tập lớp nội dung mà bản đồ đó kéo theo. Nguồn: §12, tr.76; `Actors/Environment/MissionPortal.cpp:209-295,482-486`.

## Danh mục dữ liệu điều phối các lớp

`FLevelDataLookupTable` là hàng DataTable gom thông tin của một màn chơi; TDD dẫn `Data/LevelData.h:119-760`. Hàng này kết nối dữ liệu mỹ thuật, briefing, kế hoạch triển khai, quần thể AI và điều kiện nhiệm vụ:

| Nhóm | Dữ liệu đáng chú ý | Ý nghĩa khi tác giả hóa |
|---|---|---|
| Trình bày | Tên thân thiện, nickname, designation, thời điểm, mô tả, loading screen, bản đồ chọn nhiệm vụ | Người chơi nhận diện đúng địa điểm và bối cảnh |
| Kế hoạch | Tầng, ảnh bản đồ, điểm vào, `Spawn_1..4`, depot, điểm nhân sự | Đồng bộ không gian thật với bản đồ tablet |
| Ngân sách | Deployable đề xuất, chi phí điểm, `BaseSquadPts = 10` | Ràng buộc lựa chọn triển khai |
| Quần thể | `AISpawnRosters`, `TrapRosters`, `DynamicLevelSpawnRosters` | Tạo biến thể có giới hạn |
| Nhiệm vụ | Objectives, Challenges, briefing TOC, âm thanh, tài liệu, ảnh, hồ sơ, timeline | Gắn hành vi với mục tiêu và thông tin |
| Quy tắc/hình ảnh | Yêu cầu mở khóa theo URL và điểm, override hậu kỳ NVG | Nối progression với nội dung |

`COOPModesLevelMap` ánh xạ chế độ co-op sang sublevel; `COOPModesLevelAdditionalMaps` bổ sung các lớp khác (`Data/LevelData.h:173-179`). Điểm bắt đầu dùng quy ước tag `SPAWN_1..4`, `LobbyView`; các tag `SERT_RED`, `SERT_BLUE`, `VIP_*` được TDD ghi cho PvP. `UModLevelData`, một primary data asset, đưa danh mục tương tự ra cho nội dung mod. Đây là khả năng của schema trong checkout được nghiên cứu, không phải bảo đảm mọi chế độ còn được phát hành.

## Sinh quần thể và biến thể theo roster

Actor `AISpawn` mang `SpawnGroup`. Một `FAIRoster` chỉ định số lượng tối thiểu/tối đa và danh sách `FAIDataPick { Chance, AILookupTag }`; tag tra sang dữ liệu AI và archetype. Cờ `bAffectedByNegotiator` cho phép giảm số AI 10% khi thuê negotiator. Không nên diễn giải `Chance` thành xác suất độc lập cho mọi actor nếu chưa đọc bộ chọn cụ thể: TDD chỉ xác nhận cấu trúc dữ liệu và cách gắn roster với nhóm.

Mỗi điểm spawn có thể bổ sung đường hoạt động đời thường, `TimeDoingActivity` mặc định 15 giây, spawn trong `CoverLandmark`, `SpawnChance`, tag bắt buộc, ép không cầm súng, override giáp hoặc cho phép áo nổ. Đây là hai tầng quyết định: roster trả lời có bao nhiêu và loại gì; actor spawn trả lời xuất hiện ở đâu, trong trạng thái nào. Nguồn: §12, tr.76–77; `Actors/Gameplay/AISpawn.h`.

`FTrapRoster` dùng cùng mô hình nhóm và khoảng min/max cho bẫy. Biến thể bố trí dùng `FDynamicLevelRoster`: `OverallSpawnPercent`, `MinimumPicks`, `MaximumPicks` cùng danh sách `{Label, Chance}` có trọng số. Khi trận bắt đầu, GameMode sửa khoảng min/max sai, bốc số lựa chọn và tỷ lệ tổng thể, rồi chọn label theo trọng số. Kết quả nằm trong `GameState->WhitelistedLabels`; mỗi `ADynamicWorldActor` chỉ bật khi `DynamicLabel` của nó nằm trong danh sách. Nguồn: `ReadyOrNotGameMode.cpp:150-210`; `Actors/DynamicWorldActor.cpp:56-58`.

Thiết kế này có footprint replication nhỏ: máy chủ truyền quyết định label, không cần gửi lại toàn bộ nội dung của mọi biến thể. Đồng thời, người thiết kế giữ quyền kiểm soát các chướng ngại, đường bị khóa và đồ trang trí thay thế. Tuy nhiên, tính hợp lệ về gameplay vẫn phải được kiểm tra: một tổ hợp label có thể chặn hết đường tới mục tiêu dù từng actor riêng lẻ đều hợp lệ.

## Cửa, tri thức không gian và dữ liệu bake

Theo TDD, cửa là các lớp con của `BP_Door_New`. Khả năng khóa, xác suất khóa, khả năng gắn bẫy và hệ số che âm là thuộc tính từng instance. Điều đó khiến cửa trở thành nội dung chiến thuật, không chỉ một mesh có hoạt ảnh. Các volume môi trường và trigger còn mô tả vùng tránh flank, ép low-ready, ngoài biên, hoạt động, phản ứng và foley.

`AWorldDataGenerator` cung cấp nút `CallInEditor` cho `GenerateRooms`, `GenerateCoverPoints`, `GenerateWorldThreatAwarenessActors`, sinh threat ở cửa, clear points, vị trí cửa và kiểm tra. Kết quả được lưu trong `UWorldGenSave`, qua các cấu trúc phòng, cửa, cover, threat, exit và route. Phòng có cỡ Small/Medium/Large/Corridor và đồ thị kết nối; AI dùng tri thức đó để kiểm tra cover theo phòng, suy luận nguy cơ và gọi thông tin cho SWAT. Nguồn: §12, tr.77; `Actors/WorldDataGenerator.h:283-368`.

Tài liệu nội bộ năm 2018 còn hướng dẫn đặt stack-up/clear point bằng tay. TDD **suy luận** rằng bước thủ công đó đã lỗi thời khi generator hiện tại tồn tại; chưa đủ cơ sở để nói mọi map đều đã chuyển hoàn toàn sang dữ liệu sinh tự động. Tương tự, chỉ dẫn đọc tọa độ, shift và zoom từ widget bản đồ là mô tả công cụ tác giả hóa, không bảo đảm tablet và hình học luôn khớp.

## Authority và những rủi ro cần giữ nguyên

GameMode trên máy chủ bốc roster và label; GameState replicate danh sách cho client. Mọi máy phải có nội dung và cấu hình streaming tương thích để cùng label bật cùng biến thể. Đây là điều kiện của cơ chế, không phải một hệ thống phát nội dung qua mạng.

TDD nêu bốn điểm cần chú ý: hướng dẫn viết từ 2018 bị tụt sau code; thay đổi hình học đòi hỏi bake lại; sửa min/max lúc chạy che giấu dữ liệu sai; URL ghép bằng `_Core` và `_<Mode>_Core` có thể hỏng âm thầm khi đổi tên package. Runtime cover regeneration được mô tả là bị vô hiệu hóa ở §9.7, nên không được trông đợi runtime tự sửa bake cũ. Nguồn: §12, tr.78; `ReadyOrNotGameMode.cpp:162-168`; `Actors/Environment/MissionPortal.cpp:482-486`.

## Proposed — bài thực hành tái tạo

**Phụ thuộc:** một map nhỏ có hai tầng, ba cửa, ít nhất hai lối đi; hệ thống spawn; GameMode/GameState; dữ liệu phòng/đồ thị và một UI kế hoạch tối giản. Tự viết code và dùng asset riêng.

1. Chia lớp theo chuyên môn và chế độ. Tạo hai nhiệm vụ dùng chung kiến trúc nhưng khác mục tiêu và điểm spawn.
2. Tạo roster có min/max, trọng số và seed có thể ghi log. Tạo ba nhóm label, trong đó hai nhóm xung đột được validator phát hiện.
3. Bake phòng, cửa và điểm cover thành asset; lưu hash hình học hoặc phiên bản bake. Chặn bước kiểm tra nội dung khi hash không khớp.
4. Thay chuỗi URL tự ghép bằng soft object reference; kiểm tra package tồn tại, tag spawn đúng và mọi mục tiêu còn tiếp cận được.

**Tiêu chí đạt:** cùng seed cho cùng roster trong kiểm thử xác định; client và host bật cùng label; đổi tên package không làm mất nhiệm vụ; sửa tường khiến kiểm tra bake báo cũ; dữ liệu min/max sai bị phát hiện trước khi vào trận; mọi biến thể thử nghiệm còn đường tới mục tiêu. Đây là đề xuất học tập, không phải quy trình đã được xác nhận của VOID.

Đọc tiếp: [Âm thanh và topology phòng/cổng](./13-audio.md), [quy trình kiểm chứng của bộ Docs](../04-Kien-Truc-Va-Quy-Trinh/06-Quy-Trinh-Kiem-Chung.md).

# 18. Playbook tái tạo ý tưởng cho game khác

> **Phạm vi nguồn:** diễn giải §18, tr.114–116 của TDD R&D reverse-engineered độc lập ngày **27/09/2026**. Không phải hướng dẫn chính thức VOID, không xác nhận trạng thái retail mới nhất. Các đề xuất dành cho dự án Unreal riêng; không cần và không cho phép suy ra phải sao chép source, asset, âm thanh, animation hoặc tài liệu nội bộ RoN. Các mốc công sức là **ước lượng của tác giả TDD cho cá nhân**, không phải cam kết tiến độ.

## Tái tạo một nguyên lý trước khi tái tạo cả hệ thống

Mục tiêu là hiểu vì sao một nguyên lý tạo ra trải nghiệm mong muốn, dữ liệu nào cần cho nó, ai có quyền đổi trạng thái và phép đo nào chứng minh nó hoạt động. Reaction buffer có thể làm AI công bằng hơn trước khi cần utility evaluator; roster label tạo biến thể trước khi cần sinh level tổng quát.

TDD xếp pattern theo giá trị so với công sức và đưa ra kiến trúc gợi ý cho game UE5.x mới. Đây là lời khuyên trong nguồn ngày 27/09/2026, không phải đối chiếu tài liệu engine hiện hành. Khi triển khai thực tế, đội dự án phải kiểm tra API, plugin và build target đang dùng.

## Bộ khung kiến trúc nên bắt đầu nhỏ

| Miền | Cách TDD mô tả RoN | Hướng được TDD đề xuất cho dự án mới |
|---|---|---|
| Truy cập toàn cục | Nhiều static Get và registry trong GameState; character registry được suy luận là server-only qua `PossessedBy`, còn item/door đăng ký qua `BeginPlay` trên từng máy (§2, tr.9–12 làm rõ cách viết tắt của §18) | World subsystem với accessor có kiểu; client tự có registry hợp lệ hoặc dùng danh sách replicate tường minh |
| Cấu hình | Ini GameplayConfig, AI config table và DataTable | Một UDeveloperSettings theo miền cùng DataAsset; editor validation |
| Dữ liệu hành vi | Archetype DataAsset chứa cả state runtime | Asset bất biến; state riêng mỗi agent trong component/struct của controller |
| Màn chơi | Sublevel theo chuyên môn/mode, roster label | Giữ nguyên phân tách ý tưởng; có thể dùng data layer của World Partition nếu phù hợp |
| Âm thanh | FMOD, portal graph, bank theo map | FMOD hoặc MetaSounds cộng graph riêng; sinh portal từ room data |
| Mạng | Listen/dedicated nhưng một số RPC tin client | State và validation ở server; kiểm thử mọi build flavor cần hỗ trợ |

Nguồn: §18, tr.114. Việc chọn data layer thay sublevel không tự giải quyết ownership, streaming hay dữ liệu stale. Pattern quan trọng là chuyên môn và chế độ được tách để thay đổi độc lập; công nghệ cụ thể phụ thuộc phạm vi dự án.

## Mười bốn pattern và điều cần cải thiện

| # | Pattern từ TDD | Giá trị và phụ thuộc thực tế | Công sức trong TDD | Cải thiện được đề xuất |
|---:|---|---|---|---|
| 1 | Buffer từ perception sang knowledge, §9.2 | AI nhìn thấy chưa đồng nghĩa lập tức biết và bắn; cần clock và sự kiện nhận thức | Thấp, 1–2 ngày | Scale theo khoảng cách, awareness và ánh sáng |
| 2 | Giới hạn aim theo cơ thể, §9.6 | Turn rate bị chặn và firing cone giúp aim đọc như chuyển động người | Thấp | Curve riêng theo archetype |
| 3 | Think LOD, §9.1 | Awareness điều chỉnh think rate, cap số xử lý/frame | Thấp | Tích lũy thời gian thật trôi qua, không chỉ thời gian mỗi lượt được ghé |
| 4 | Utility chọn, activity thực thi, §9.4–9.5 | Designer tune hành vi bằng dữ liệu; cần consideration, gate, lifecycle | Vừa, 1–2 tuần | Pool không phụ thuộc thứ tự, cache gate, state ngoài asset, break đúng trong FSM |
| 5 | Biến thể label và weighted roster, §12 | Tái chơi với lượng state replicate nhỏ | Thấp–vừa | Kiểm tra roster/label trong editor |
| 6 | Bake tri thức không gian, §9.7, §9.9, §12 | Room, cover từ navmesh edge, clear point cửa thành mảng runtime | Vừa–cao | Hash geometry để phát hiện stale; dùng room data sinh audio portal |
| 7 | Truyền âm portal và angular occlusion, §13 | Người chơi định vị nguy cơ theo tòa nhà | Vừa, 1–2 tuần | Sinh topology tự động; containment đúng hình volume thay AABB |
| 8 | Phụ đề theo khả năng nghe, §13 | Không làm lộ lời thoại ngoài phạm vi tai nghe | Thấp | Dùng chung kết quả audibility của VO |
| 9 | Shuffle-bag VO, quy ước tên, file rời và transcription, §13, §16 | Quản lý hàng nghìn câu và hạn chế lặp | Thấp–vừa | Kiểm tra tên, locale, staging; giữ schema rõ |
| 10 | Điểm theo quy trình, §10 | Ledger ROE/report/evidence/grade thưởng sự kiềm chế | Vừa | Win check không phụ thuộc thứ tự, guard chia 0, penalty từ data |
| 11 | Hành động có thể bị từ chối, §3 | Animation gate và action lock tạo nhịp có chủ ý | Thấp | Authority nhất quán cho thay đổi tốc độ |
| 12 | Cửa nhiều trạng thái chiến thuật, §7 | Mở hé, khóa/bẫy ngẫu nhiên và nhiều cách breach tạo quyết định | Cao | FSM cửa nhỏ hơn, data-driven; tránh chia nguyên trong xác suất |
| 13 | Lệnh vòng tròn theo ngữ cảnh, §8 | Một input truy cập nhiều lệnh | Vừa | Một resolver chung cho mọi input device |
| 14 | Hậu kỳ theo asset và requirement, §14 | Thêm status effect không cần C++ riêng cho từng effect | Thấp–vừa | Tránh bypass Super::Super; dùng blend volume hoặc ownership rõ |

Hàng 8–9 bổ sung tiêu chí thực hành; bảng gốc không đặt cải tiến riêng. Thời lượng hàng 1, 4, 7 là ước lượng nguồn; các hàng khác chỉ có mức tương đối. Không cộng chúng thành lịch chính xác: còn phụ thuộc chung, content, polish, network và QA.

## Mô hình dữ liệu tối thiểu

`LevelRow` cần map/lớp theo mode, entry point, roster AI/bẫy/label, objective và briefing. Không bắt đầu bằng mọi trường của RoN: chọn đúng dữ liệu đủ để một mission chạy từ đầu đến chấm điểm. Soft reference và validator sẽ giảm lỗi do đổi tên nội dung.

`ArchetypeAsset` giữ danh sách action; mỗi action có consideration curve, gate, weight, threshold, commit time, cooldown, interrupt. **Dữ liệu mô tả không giữ state riêng agent**. Điểm số đang tính, target hiện tại, timer và lịch sử interrupt thuộc controller/component. TDD cảnh báo khoảng 60 consideration của RoN nằm trong Blueprint, nên ngân sách phải bao gồm tác giả hóa đầu vào hữu ích, không chỉ viết phép cộng/nhân utility.

`ItemData` và `AmmoData` chứa damage, penetration, hit-location multiplier. `ScoringTable` chứa event ROE, bonus/penalty và ngưỡng grade. `AudioTopology` chứa room/portal được sinh hoặc xác minh; VO có quy ước thư mục và cooldown table. Mỗi schema cần range, unit, giá trị default và trường hợp thiếu dữ liệu. Nguồn mô hình: §18, tr.115; các yêu cầu validator ở đây là **Proposed** để thực hành.

## Authority và khả năng bảo trì là điều kiện nền

TDD yêu cầu mọi Server RPC đối chiếu giá trị với state server, không nhận tốc độ, health hoặc checksum client như sự thật. Một RPC có tên Server và keyword Validate chưa đủ; nội dung phép kiểm tra mới quyết định quyền. Registry dùng cho gameplay phải có trên client nếu client cần đọc, hoặc caller client không được phụ thuộc vào nó.

Ma trận build gồm editor, standalone, listen server, dedicated server và Shipping. Hai chiều build configuration và net mode không hoàn toàn loại trừ nhau; khi lập kế hoạch thực tế phải ghi rõ tổ hợp nào hỗ trợ. Những lỗi chỉ lộ ngoài editor hoặc trên dedicated không được phát hiện bởi một lượt single-process PIE.

Feature flag nên có một nơi khai báo rõ. Mod cần đường đóng gói từ đầu, chẳng hạn UGC plugin, và kiểm tra CI. `UModLevelData`, parameter Mod_* hay wrapper mod.io chưa chứng minh chuỗi tải/nạp/dỡ/version hoạt động; xem [mục mạng và mod](./15-network-platform-mods.md).

## Thứ tự xây dựng và các cổng hoàn thành

TDD đề nghị thứ tự ở tr.116: core loop/pacing → AI base → level pipeline → audio → scoring/rules → polish cửa/lệnh/hậu kỳ. Dưới đây chuyển thành **Bài thực hành tái tạo — Proposed**, có phụ thuộc và tiêu chí đạt; đây không phải lịch nội bộ RoN.

1. **Core loop:** dùng character và item riêng; dựng movement multiplier, action lock/refusal và aim cone. Đạt khi input bị từ chối có lý do nhìn thấy được, tốc độ không vượt giới hạn server, các lock được giải phóng khi cancel/death.
2. **AI base:** thêm perception/reaction buffer trước utility. Đạt khi delay được đo, target không thành knowledge trước deadline, scheduler không bỏ mất thời gian khi LOD đổi, mọi agent có state độc lập.
3. **Level pipeline:** một building, hai mode, vài roster/label và bake room/cover. Đạt khi client/host cùng biến thể, objective luôn tiếp cận được, geometry đổi làm validator báo bake cũ.
4. **Audio:** bank theo miền/map, graph và VO/phụ đề. Đạt khi đóng cửa đổi âm, subtitle không lộ lời không nghe được, shuffle bag không lặp sớm, topology khớp phòng.
5. **Rules:** ledger sự kiện và grade. Đạt khi đảo thứ tự objective không đổi verdict, score pool 0 có kết quả quy định, cùng event không bị ghi hai lần.
6. **Polish:** cửa giàu trạng thái, resolver lệnh chung, effect asset. Đạt khi chuột/gamepad chọn cùng lệnh cho cùng context, effect thiếu dữ liệu thất bại có kiểm soát, ragdoll/blending chỉ bật sau kiểm tra bằng content riêng.

## Rủi ro khi áp dụng

Data-driven không đồng nghĩa có thể tune được mọi thứ: reader có thể đã bị comment out. Bake nhanh ở runtime nhưng có thể lỗi thời. Portal graph rẻ chỉ khi topology đúng. Ngẫu nhiên tạo replayability nhưng những lần tung đồng xu ẩn trong code làm designer khó hiểu kết quả; TDD khuyên đưa xác suất thành tunable. Khi công bố một kết quả học tập, ghi rõ phần nào đo được, phần nào chỉ là ý tưởng, và không lấy một prototype thành công làm bằng chứng toàn hệ thống đã đúng.

Nguồn §18 không đưa locator `file:line` mới mà trỏ về các mục trước. Các ví dụ locator được TDD cung cấp có thể xem tại [level](./12-level-design-pipeline.md), [audio](./13-audio.md), [presentation](./14-presentation-feel.md), [network](./15-network-platform-mods.md), [tooling](./16-tooling-build-tests.md) và [118 vấn đề](./17-known-issues.md). Không tái bản source code, media hoặc PDF gốc.

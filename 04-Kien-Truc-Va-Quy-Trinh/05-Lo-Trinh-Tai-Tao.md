# Lộ trình kiến thức: dựng lại theo các vòng chơi nhỏ

Mục tiêu học tập không phải đọc hết source rồi mới viết. Hãy đi theo vòng **quan sát → lần source → dựng fixture → đo → giải thích lại**. Mỗi mốc phải tạo một trải nghiệm có đầu và cuối; kiến thức được học đúng lúc nó giải quyết một vấn đề cụ thể.

Lộ trình dưới đây là đề xuất cho người chưa từng tham gia dự án lớn. Nó không phải kế hoạch hoàn thành toàn game trong một số tuần cố định và không tuyên bố các mốc đã được thực hiện chỉ vì tài liệu có chương tương ứng.

## Các tầng kiến thức và đầu ra

| Tầng | Cần hiểu | Sản phẩm thực hành | Điều kiện đi tiếp |
|---|---|---|---|
| 0. Nền tảng công cụ | Git, build target, log, breakpoint, asset browser | Mở baseline đúng engine, ghi phiên bản | Tìm lại được lỗi và log của chính lần chạy |
| 1. C++ và Unreal object | class, pointer/reference, UObject/Actor/Component, reflection, GC | Actor có component và event, spawn/destroy sạch | Giải thích được owner và lifetime |
| 2. Không gian và input | vector/rotation, local/world, collision, pawn/controller, input | Player di chuyển và nhìn trong phòng đo | Không nhầm camera direction với mesh transform |
| 3. State và luật | enum/state machine, timer, delegate, authority | Một mission Start → Active → Result → Retry | Outcome chốt một lần, reset sạch |
| 4. Item và presentation | inventory, attachment, skeletal mesh, montage/notify | Một khẩu súng equip/fire/reload có feedback | Ammo/state/animation khớp nhau |
| 5. Damage và tương tác | health/limb, target, report/evidence, cancel | Một target có thể bị xử lý và thu dọn | Hành vi ảnh hưởng objective/score đúng |
| 6. Cửa và không gian AI | collision/nav, door states, marker, async path | Hai phòng và một cửa dùng được bởi player/AI | Mesh/collision/nav thống nhất |
| 7. AI cá nhân | perception, memory, morale, action score, interrupt | Một suspect và một civilian giải thích được hành vi | Mọi action có reason trace |
| 8. AI đội | command context, phân công, shared stages | Hai SWAT move/hold/stack/cancel | Failure của một người không treo đội |
| 9. Network và persistence | RPC/replication, owner/observer, save schema | Hai client cùng mission và một slot test | State cuối thống nhất và sống đúng qua travel |
| 10. Sản xuất nội dung | data-driven variants, profiling, packaging, UX | Thêm súng/map nhỏ bằng quy trình lặp | Mỗi nội dung mới có dependency và receipt |

Network authority cần được nghĩ từ tầng 3 và thử sớm ở tầng 4–5, dù tầng 9 mới mở rộng travel/persistence. Đừng viết toàn bộ game dựa trên state local rồi mong thêm multiplayer bằng cách bật `Replicates` cuối dự án.

## Bảy mốc sản phẩm gợi ý

**M0 — Baseline có thể lặp.** Dùng engine đi kèm, vào đúng map, ghi config/save/loadout. Đạt khi hai lần chạy có điều kiện giống nhau; compile thành công chưa đủ nếu PIE chưa vào được world.

**M1 — Gun lab một khẩu.** Dùng player, inventory, animation/audio/camera thật của snapshot để giảm sai lệch. Bắn vào target đo ở nhiều cự ly trong game, reload đủ các case có trong weapon. Đạt khi giải thích được đường input tới ammo/damage/feedback và có phép so sánh; chưa cần AI.

**M2 — Mission hai phòng.** Một cửa, một suspect fixture đầu hàng, một civilian, một evidence, results và retry. Đạt khi thu dọn ảnh hưởng kết quả và mọi đường hủy hành động đã kiểm tra.

**M3 — Người có hành vi.** Thay fixture bằng AI perception/morale/action, giữ room ít biến số. Đạt khi log giải thích khác biệt giữa nghe, thấy, surrender, chống đối và bị bắt; chưa cần mọi archetype.

**M4 — Đội hỗ trợ.** Hai SWAT với move/hold/stack và một chuỗi vào phòng. Đạt khi có path failure, cancellation, team selection đúng. Sau đó mở rộng tool và số người.

**M5 — Hai người chơi.** Hai client làm chung M2–M4. Đạt khi authority, observer presentation, reset và travel có receipt. Không lấy “không crash” làm chuẩn đồng bộ.

**M6 — Nội dung theo hệ thống.** Mở rộng catalog súng/attachments, các room/mission data, archetype và progression. Đạt theo từng hàng manifest; một súng thiếu audio/animation vẫn là coverage gap, dù đã spawn được mesh.

## Bám source ở mỗi mốc

M1 lần qua inventory/weapon/animation trong quyển 03. M2 đọc `ACoopGM::CheckWinConditions` (`GameModes/CoopGM.cpp:664`), `AObjective` và `ADoor`. M3 đọc `AIActionDecisionEvaluator` (`AI/AIActionData.cpp:156`) và `ACyberneticController`. M4 đọc `USWATManager::GiveStackUpCommand` (`Info/SWATManager.cpp:3801`). M5 dùng replicated properties của `ReadyOrNotGameState.cpp:56` và `InventoryComponent.cpp:36` làm bản đồ. M6 nối manifest asset với profile/config trong chương trước.

## Bài tập để biết mình đã hiểu

Sau mỗi mốc, tự giải thích một hành động bằng năm câu: input gì; state trước là gì; ai quyết định; khi nào kết quả được chốt; người chơi thấy/nghe gì. Sau đó thêm ba câu: có thể thất bại thế nào; reset làm gì; client khác thấy gì. Nếu chưa trả lời được một câu, đó là tầng kiến thức tiếp theo cần học.

**Tiêu chí toàn chương:** có backlog nhỏ theo dependency, mỗi task có fixture và acceptance; không dùng số file/asset import làm tiến độ gameplay. **Khoảng trống chủ động:** công nghệ online platform, localization, accessibility, console certification, pipeline đội nghệ thuật và engine internals có thể học sâu sau khi vòng chơi nhỏ đã ổn; chúng vẫn là phần của sản phẩm lớn, không bị coi là không quan trọng.

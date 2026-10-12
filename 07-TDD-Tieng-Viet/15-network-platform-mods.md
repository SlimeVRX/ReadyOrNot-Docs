# 15. Mạng, phiên chơi, nền tảng và mod

> **Phạm vi nguồn:** diễn giải TDD R&D độc lập ngày **27/09/2026**, §15, tr.90–96. Nguồn là reverse engineering một checkout, không phải tài liệu VOID chính thức hoặc kiểm tra bản retail mới nhất. Mọi `file:line` dưới đây **do TDD cung cấp**; đường dẫn mặc định thuộc `Source/ReadyOrNot/`. Đặc biệt, “có lớp/thuật toán” không đồng nghĩa tính năng đang được compile hoặc vận hành.

## Mô hình mạng và ranh giới trách nhiệm

Theo TDD, game dùng client–server tiêu chuẩn Unreal qua `AGameSession` và `UNetDriver`, không có transport riêng. Co-op dùng listen server; target dedicated server cũng tồn tại. Máy chủ quyết định trạng thái gameplay, actor/property replication chuyển kết quả sang client, còn multicast thường phục vụ thông báo một lần. Nền tảng nằm sau `IOnlineSubsystem`: Steam trên PC; GDK+EOS cho Xbox và Sony+EOS cho PlayStation được TDD xác định từ cấu hình link (`ReadyOrNot.Build.cs:86-296`). Điều này mô tả dependency của checkout, chưa chứng minh mọi đường code được dùng trong mọi bản phát hành.

`UReadyOrNotGameInstance` sống suốt process, quản lý tạo/tìm/join session, version, checksum PAK, lỗi mạng và trigger migration. `AReadyOrNotGameSession` giữ chính sách dedicated server: password, admin, ban, giới hạn team-kill/AFK, map rotation. `UConsoleMultiplayerSubsystem` tách lobby/party social khỏi phiên chơi và callback matchmaking Sony. `UReadyOrNotBackend` là HTTP client cho login Steam ticket, heartbeat, liên kết Discord, DLC/ban, token Vivox và telemetry; heartbeat 300 giây, retry login 3 giây. Chương này không đưa endpoint, credential hoặc hướng dẫn gọi backend.

## Quảng bá phiên và kiểm tra tương thích

`RegisterServer` chỉ compile dưới `UE_SERVER || UE_EDITOR`. Session quảng bá map, mode, tên server, version, checksum/cờ checksum, số người không tính replay spectator và cờ PvP. Cờ PvP được đặt khi một mission portal có `ModeURL` không chứa `COOP`. `NumPublicConnections` là `MaxConnections` clamp theo giới hạn mode; `bIsDedicated = true`. `UpdateServerDetails` chỉ compile dưới `WITH_STEAM`. Browser bỏ qua session mang `MIGRATION_GUID` và đọc version/checksum trước join. Nguồn: `ReadyOrNotGameSession.cpp:61-171,423-476`; `GameModes/MainMenuGM.cpp:392-403`.

Version mạng được tạo bằng cách bỏ dấu chấm khỏi project version rồi đổi sang số nguyên. `IsNetworkCompatible` luôn true ngoài Shipping, còn Shipping cần bằng tuyệt đối (`ReadyOrNotGameInstance.cpp:2055-2070`). Như vậy một lần đổi version có thể tách nhóm client; test development thành công chưa chứng minh Shipping chấp nhận kết nối.

“Checksum” PAK trong TDD thực ra là **tổng kích thước các PAK + số PAK × 76849**, không phải hash nội dung mật mã. PAK ngoài `VALID_GAME_PAK_FILES` đánh dấu modded và được quét: `.umap` thêm map mod; `AILevelData.ini` đặt no-scoring; file được đăng ký asset registry. Controller local báo giá trị bằng `Server_SetChecksum`. Listen host kick khi setting checksum bật, giá trị báo khác 0 và khác giá trị local; khối kiểm tra **bị tắt dưới `UE_SERVER`**. Validate của RPC luôn true, nên máy chủ tin giá trị client tự báo. Nguồn: `ReadyOrNotGameInstance.cpp:1936-2053`; `Characters/ReadyOrNotPlayerController.cpp:209-215,457-475,689-697`.

Đây là cổng tương thích nội dung yếu, không nên biến thành kết luận “đã chống sửa client”. Đề xuất của TDD là kiểm tra phía máy chủ/challenge; khi thiết kế mới, cần xác định threat model và giới hạn của mọi kết quả do client cung cấp.

## Host migration: có snapshot nhưng tuyến phục hồi không hoạt động

`UHostMigrationManager::SaveState` ghi một danh mục khá đầy đủ: người chơi gồm ID, transform, control rotation, health, equipped class, inventory và trạng thái magazine/index; AI gồm transform, mesh, team, health, arrested/surrendered/reported và tag; cửa gồm broken/open amount, mesh transform, tối đa 9 chunk transform, trap còn sống; bomb timer/state; evidence active/bagged/dropped; objective status (`Info/HostMigrationManager.cpp:26-172`). Đây là mẫu tham khảo tốt để nghĩ về trạng thái phải sống qua reconnect.

Thiết kế migration là lưu map/mode, hiện loading, phá session cũ; host tạo session 5 slot với GUID rồi mở map listen, client tìm session cùng version/GUID và join. Destroy session thành công hay thất bại đều dẫn tới tạo phiên mới. Khi tiếp tục chơi, LobbyGM gỡ GUID. Đó là **ý định của luồng**, không phải chức năng chạy được đã xác nhận.

TDD kết luận `HOST_MIGRATION_ENABLED` không được định nghĩa trong Build/Target đã kiểm tra, nên tự chuyển host và đường nạp lại state bị compile out. Debug command vẫn có thể gọi migration nhưng `SetNextHost` không được C++ gọi và không là UFUNCTION; `NextHost` giữ null, dẫn tới main menu. Search client đặt `bFoundSession` nhưng không dùng nó để dừng; điều kiện `FindSessionAttempt > 30` cho phép **31 lượt tìm liên tiếp không delay**, rồi về menu dù join từng thành công. Session migration còn bỏ checksum. Nguồn: `ReadyOrNotGameInstance.cpp:2500-2520,2555-2574`; `HostMigrationManager.cpp:174-186,222-356`; `.h:330`.

Các dereference thiếu null-check cũng được nêu, nhưng TDD lưu ý manager hiện được `Init()` tạo nên một đường là an toàn theo thứ tự khởi tạo hiện tại. Không nên nâng mọi thiếu guard thành crash đã tái hiện. Bài học là snapshot chỉ là một phần: bầu host, session discovery, restore, timeout và điều kiện kết thúc phải được thiết kế cùng nhau.

## Biểu quyết và quản trị server

Actor `AReferendum` replicate, always relevant. Authority chụp tập controller đủ quyền lúc bắt đầu; thời gian mặc định **30 giây**. Yes hoặc No vượt nửa tổng eligible voter sẽ kết thúc sớm; người chơi được đổi phía. Hết giờ, chỉ cần Yes > No để thông qua, hòa thất bại và người không bỏ phiếu không tính. Sau khi thông qua, chờ thêm **3 giây** rồi gọi `OnReferendumPassed`; native body rỗng, dự kiến subclass/Blueprint cung cấp tác động. Cooldown mỗi controller là **5 giây** (`Actors/Voting/Referendum.cpp:12-155`; `.h:25,29`; `ReadyOrNotPlayerController.cpp:5670-5712`).

Tuy nhiên, RPC khởi tạo bỏ qua class và chỉ mở popup `BeginVote` cũ. TDD không tìm được C++ spawn actor hoặc gán `CurrentReferendum` ngoài reset null; property lại BlueprintReadOnly. Map/player referendum và Yes/No RPC do đó no-op trong tuyến native được đọc. Blueprint có spawn ở nơi khác hay không **chưa xác minh**, nhưng TDD chỉ ra pointer vẫn không được gán qua property đó. Các multicast thông báo Yes/No/Started có implementation nhưng không có caller C++ (`ReadyOrNotPlayerController.cpp:5614-5712`; `Referendum.cpp:157-185`).

Server policy là config: MaxConnections 16; auto-kick sau 2 team-kill, auto-ban sau 4; AFK 0 nghĩa tắt; round start 90 giây, giữa map 30, reinforcement 30, respawn 8, timelimit 1200, scorelimit 150, 2 round/map, ClientNetSpeed 250000, về lobby sau 1 mission. Default của `SetServerSettings` Blueprint khác ở round start 480 và timelimit 900 (`ReadyOrNotGameSession.h:117-118,143-216`). Team-kill để ban lưu theo unique-net-ID qua SaveGame; ban/admin ghi config. AFK bỏ qua admin và pawn spectating.

Admin RPC kiểm tra `IsAdmin()` trong Validate, nhưng hàm trả true nếu không có GameState. Login admin chỉ cho dedicated server, password không rỗng và khớp; comment nói listen host luôn admin nhưng cách cấp quyền **chưa xác minh**. Những chi tiết này cần giữ riêng khỏi các RPC tin client như checksum.

## Ngân sách tick, dịch vụ và trạng thái compile

Significance manager là tối ưu tick/animation, **không phải network relevancy**. Mỗi 0,2 giây nó kiểm tra một character: trong 1400 unit luôn relevant; xa hơn và sau lưng là irrelevant; phía trước dùng 2 trace, thêm 5 trace nếu dưới 3500 unit. AI relevant tick 1/30 giây khi xa hơn 3500, 1/60 giây khi xa hơn 2000, còn lại mỗi frame; actor irrelevant và component/movement tick 1 giây. TDD giữ đơn vị là Unreal unit. Nguồn: `Info/ReadyOrNotSignificanceManager.cpp:28,38-147,262-339,552-618`.

Chỉ viewpoint local được xét: trên listen host có thể hạ tần suất AI gần remote player; dedicated không có local controller trả transform identity. **Tác động này là suy luận của TDD**, chưa có đo runtime trong chương. Khi tái tạo, lấy mức ưu tiên cao nhất từ mọi viewpoint có liên quan.

TDD thống kê 604 `DOREPLIFETIME*`, 255 Server UFUNCTION (240 Reliable, 25 không WithValidation), 152 NetMulticast (9 unreliable), 66 Client. Đây là thống kê source, không phải packet rate/bandwidth. Vivox được link nhưng `WITH_VIVOX` chỉ sống trong private definition PlayStation; PC/Steam và Xbox compile thành stub. VOIP setting ghi AudibleDistance 8100, ConversationalDistance 270, inverse-distance fade; không dùng chúng để suy ra voice hiện hoạt động trên mọi platform. `WITH_MODIO` comment out khắp nơi được kiểm tra; raw PAK drop-in vẫn có tuyến scan. `SteamParty` chỉ còn header, không có plugin/build hoàn chỉnh. Zeuz cũng chỉ còn dấu vết và flag đổi quảng bá server.

## Proposed — bài thực hành tái tạo

**Phụ thuộc:** session subsystem, server-authoritative character/inventory, map nhỏ, hai client, target dedicated và schema snapshot có phiên bản. Bắt đầu với create/find/join/leave và thông báo lỗi; chỉ thêm migration khi cần rõ ràng.

**Bài thực hành:** một object replicate duy nhất làm nguồn cho UI vote và kết quả; server tự tính quyền, tốc độ và tài nguyên; catalog mod có phiên bản; tick LOD tổng hợp mọi người chơi. Với migration, test riêng bầu host, serialize/restore, ID actor, retry có backoff, dừng ngay khi join thành công và timeout duy nhất.

**Tiêu chí đạt:** development và Shipping được thử riêng; client sửa giá trị không đổi state trái phép; vote hòa/thay phiếu/người rời phiên có chính sách rõ; AI gần remote không bị giảm tick do xa host; disconnect không tạo vòng join vô hạn; schema snapshot giữ magazine, cửa, evidence, objective sau restore. Đây là tiêu chí cho dự án mới, chưa phải bằng chứng RoN đã đạt chúng.

Liên hệ: [authority trong bộ Docs](../04-Kien-Truc-Va-Quy-Trinh/03-Multiplayer-State-Ownership.md), [tooling và kiểm thử](./16-tooling-build-tests.md).

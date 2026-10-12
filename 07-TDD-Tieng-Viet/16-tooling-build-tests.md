# 16. Công cụ, build, kiểm thử và đo hiệu năng

> **Phạm vi nguồn:** diễn giải TDD R&D reverse-engineered độc lập ngày **27/09/2026**, §16, tr.97–103. Không phải tài liệu VOID chính thức, không xác nhận trạng thái retail mới nhất. Các `file:line` là **trích dẫn do TDD cung cấp**, chưa được kiểm chứng mới ở đây. Chương này mô tả công cụ; không chạy build, upload, cheat hoặc pipeline nội bộ.

## Công cụ tồn tại chưa có nghĩa kiểm thử là cổng phát hành

TDD mô tả ba nhóm: debug menu và cheat state cho QA; automation/Gauntlet cho AI và hiệu năng; TeamCity+BuildGraph để build, đóng gói, upload và promote. Kết luận đáng chú ý là automation và Gauntlet **không nằm trong promotion gate được tìm thấy**: không TeamCity build type, bước `Build.ps1` hay BuildGraph node nào chạy chúng. Promote chỉ phụ thuộc Steam build thành công. Do đó không nên đồng nhất “repo có test” với “mỗi build đã được test”. Nguồn: §16, tr.97,101.

Frontend DevMenu có module type `DeveloperTool` và guard `!UE_BUILD_SHIPPING`, nên TDD cho rằng menu trên màn hình không tồn tại trong Shipping. Nhưng `UReadyOrNotDebugSubsystem`, nơi giữ state thật, không có guard tương tự. TDD đếm khoảng 58 hàm BlueprintCallable cho health, ammo, damage/melee multiplier, door state, RTX, cover/threat/navigation và audio. Đây là ranh giới quan trọng: ẩn cửa vào UI không loại bỏ implementation khỏi binary.

## Menu dựa trên ini và state tập trung

Module nghe `OnWorldBeginPlay`, tạo menu cho **local player controller đầu tiên**, bind phím theo controller và poll combo gamepad trong Tick. `UDevMasterMenuWidget_INI` tạo trang theo `[Section]`; section có thể thay bằng `CustomWidgetClass`. Một dòng `ConsoleCommand=` trở thành lệnh console, còn `DebugCommand=` tải Blueprint kế thừa `UDevMenuScript`. Script cung cấp ExecuteScript, giá trị trạng thái và màu chữ. TDD **chưa xác minh** rằng phần lớn Blueprint này đều chuyển tiếp sang debug subsystem.

`Config/DevMenuCommands.ini` trong nguồn có 843 dòng, 30 section, 88 ConsoleCommand, 216 DebugCommand và 15 section dùng custom widget; không có dòng Array đang dùng. Trang Stats chứa `stat fps/unit/unitmax/unitgraph/AI/game`, dumpticks, ProfileGPU và giới hạn FPS 10/20/30/60/120/240; giá trị “unlimited” là 9.999.999. Menu mặc định bật, mở bằng Backslash/NumPad1 hoặc thumbstick trái + shoulder phải; phím điều hướng gồm mũi tên/D-pad, Enter và Backspace. Nguồn: §16, tr.98,100; `Plugins/DevMenu/.../DevMenuWidget.cpp:570-760`; `Config/DevMenuCommands.ini:595-637`.

Debug subsystem là `UGameInstanceSubsystem`, một instance mỗi game instance và không replicate. Toggle health vô hạn gọi cả `Server_SetUnlimitedResource` lẫn setter local. Listen host vì thế đi qua hai đường; hiệu lực của setter local trên client phụ thuộc role check trong health component và **chưa được mục này xác minh** (`ReadyOrNotDebugSubsystem.cpp:88-97`). Menu không thêm authority check; lệnh chạy qua `APlayerController::ConsoleCommand` thông thường. Khi tái tạo, quyền phải được kiểm tra tại thao tác có thẩm quyền, không chỉ tại nút UI.

## QA ghi nhận AI và Gameplay Debugger

`ABadAIAction` lưu ghi chú dưới section map trong `Config/BadAIActions.ini`, dạng ID, vị trí, summary, description. Report mới cùng chuỗi vị trí thay thế report cũ (`Debug/BadAIAction.cpp:83-134`). TDD chỉ ra RemoveReport dùng `RemoveAt(ID)` dù ID là kích thước mảng khi tạo; xóa trước đó làm ID cũ trỏ sang ghi chú khác. Sau đó xóa theo toàn chuỗi có thể xóa thêm bản ghi đúng; các phép tìm chỉ bằng location lại không khớp toàn chuỗi (`:148-156`). Bài học là cần ID ổn định tách khỏi index hiển thị.

Gameplay Debugger có category `ReadyOrNot`, slot 5, hiện dữ liệu `ACyberneticCharacter`, dưới `WITH_GAMEPLAY_DEBUGGER`. Các thư viện automation hỗ trợ input action/axis, spawn/destroy SWAT tại cửa, ghi kết quả và hỏi activity đã xong chưa. Đây là hạ tầng giúp tạo scenario lặp lại, không tự bảo đảm scenario có assertion chính xác.

## Kiểm thử AI: phạm vi và sai lệch báo cáo

Stack-up test liệt kê `.umap` chứa `RoN_` và kết thúc `_Core`, loại lớp mode cụ thể; cũng chấp nhận tên chứa `Ali_`, và thêm case từng cửa trong world editor. Nó mở map, khởi động PIE, chạy các lệnh latent. Phòng center/hallway dùng **5 trong 6** hoán vị Split/Left/Right ở mỗi phía, thiếu Split→Right→Left; corner/hallway-left/right chỉ một kiểu. Time dilation là **2×** cho một cửa, **20×** cho tất cả cửa; timeout theo uptime activity là **20 giây**. Nguồn: `Plugins/ReadyOrNotTests/.../SwatStackUpTest.cpp:99-352`.

Breach test thử Open/Kick/Shotgun/C2/Ram/Move và item tiếp theo như flashbang hoặc M320. Stinger và CS gas đã comment out. Mỗi update ép infinite SWAT items/ammo/health, timeout **40 giây**: đây là kiểm tra logic breach, không bao phủ hao hụt tài nguyên. Cả hai chạy single-process PIE nên không kiểm thử replication (`SwatBreachAndClearTest.cpp:186-258,430-465`).

TDD ghi lỗi stack-up thành công cũng gọi `AddError`, summary thành công/thất bại đều vậy; harness vì thế coi test thất bại. Counter là file-static dùng chung, chỉ đếm kết quả đầu tiên mỗi cửa, nên tỷ lệ không phản ánh mọi ordering (`SwatStackUpTest.cpp:64-66,377-431`). Cần sửa kênh report và mô hình mẫu trước khi dùng số liệu để đánh giá AI.

## Map-load test và heatmap hiệu năng

`FMapLoadTest` có danh sách hard-code 19 map. Chuỗi latent: load và ghi load time; bật stat unit, chờ **10 giây**; ghi frame/thread time, memory, texture memory, draw/primitive count theo `TestResult;<Map>;<Metric>;<Value>`; stat capture **5 giây chạy + 5 giây pause**; rồi memory report. TDD không tìm thấy consumer log trong BuildTools, nên đường phân tích tự động **chưa xác minh**. Danh sách không có tutorial và không theo danh mục đóng gói (`Testing/ConsoleAutomation.cpp:139-169,228,243-302`).

Gauntlet base chờ warm-up **3 giây**, CSV capture **16 giây**. Spin controller đổi pawn thành spectator, ready player state, tạo timer **1/60 giây**. Mỗi callback quay yaw **6°**, tốc độ danh nghĩa 360°/giây, ghi vị trí/hướng và `GetDeltaSeconds()`. Header ghi FPS nhưng cột thực là **frame delta**, không phải FPS; reader đổi `1/delta` và bỏ dòng header.

Sau hơn một giây ở vị trí, camera bước X bằng **10% half-extent** của volume; hết X thì quay về đầu hàng và bước Y cùng quy tắc, giữ Z của actor. Reader đọc tối đa 62 hàng mỗi điểm, lấy FPS thấp nhất: dưới 30 đỏ, dưới 60 cam, dưới 120 vàng, còn lại xanh. Timer có thể gọi nhiều lần trong một frame chậm, dẫn tới các dòng delta lặp; TDD thấy hiện tượng này trong ba capture đã lưu, không nên xem mỗi dòng là một frame độc lập. Nguồn: `Testing/ReadyOrNotSpinTestController.cpp:81-190`; `SpinTestHeatmapVolume.cpp:13-128`.

Các rủi ro gồm writer ghi `ProjectDir/../../../../PerfTests` nhưng reader đọc `ProjectDir/PerfTests`; reader so sánh FVector chưa khởi tạo; sau hết sweep có thể gọi StopProfiling lặp mỗi giây, ghi lại file và tạo nhiều async wait (**suy luận chưa chạy**). StartProfiling của spin bị comment out nhưng Stop vẫn gọi EndCapture. Những điểm này khiến hình heatmap đẹp chưa đủ để tin phép đo.

## Build và cấu hình phần cứng

TeamCity gọi một `BuildTools/Build.ps1` với `-buildStep`. Steam đi SetupGame → Engine → BuildGame → UploadToSteam → SaveSymbols. Engine dùng **BuildGraph riêng của project**, `BuildTools/BuildGraph/BuildEditorAndTools.xml`, không phải example Epic; target mặc định Tag Output Files, chỉ đổi sang Submit To Perforce for UGS khi có code change và bật push binary. Game dùng BuildCookRun. Promote chạy CopyToStaging từ experimental sang main sau Steam build thành công (`Build.ps1:118-143,281,538-595`).

`BuildParameters.ini` có default và bốn section stream; default đóng gói 20 map: 18 BarricadedSuspects, Station, TutorialV3. Nó cũng giữ feature defines như premisson planning, FSR, Reflex, carry arrested, mod/replay. TDD không công bố tên stream/branch nội bộ và credential; chương này cũng không chép chúng. Jenkins/ScriptsOld/ScriptsV2 còn trong cây là pipeline cũ, không nên chọn chỉ vì gặp trước khi tìm kiếm.

Console tuning nhận SKU và áp baseline dynamic resolution off, FSR FP16 on, screen percentage 50, SSR quality 1. PS4/Pro dùng 1280×720, pool 500, shadow/AA 2 nếu cast user settings thành công; Xbox One/S pool 1000, One X 2000; Series S 1200 và tắt subsurface; Series X/PS5 pool 2000. TDD không ghi đơn vị riêng của pool. `MapName` chỉ log, `QualityOverFrameRate` không được đọc, nên tên LevelSpecific gây hiểu nhầm. Hàm dereference local controller thiếu guard (`lib/ReadyOrNotConsoleFunctionLibrary.cpp:43-171`).

## Proposed — bài thực hành tái tạo

**Phụ thuộc:** CI riêng, test map riêng, automation harness, log có cấu trúc và profile build. Giữ menu ini đơn giản; đặt cheat sau guard Shipping hoặc authority-gated cheat manager. Dùng một catalog map cho cook lẫn test. Cho writer/reader dùng chung đường dẫn và schema `frame_delta_seconds`; lấy mẫu mỗi frame thật hoặc mô tả rõ timer sampling.

**Tiêu chí đạt:** test thành công không ghi error; counter reset mỗi run; mọi ordering có ID/kết quả riêng; capture mới được đọc ngay; sweep kết thúc đúng một lần; số liệu ghi rõ unit và warm-up; build fail nếu test bắt buộc fail; chạy ít nhất standalone/listen/dedicated/Shipping theo phạm vi tính năng. Kiểm tra RTX master phản ánh trạng thái từng effect, không chỉ bool cũ; lint phát hiện `.bak`, optimization pragma và dữ liệu CI nhạy cảm. Đây là quy trình đề xuất, không phải kết quả test đã chạy trên RoN.

Liên hệ: [ma trận vấn đề từ TDD](./17-known-issues.md), [quy trình kiểm chứng](../04-Kien-Truc-Va-Quy-Trinh/06-Quy-Trinh-Kiem-Chung.md).

# 08. Giao diện lệnh tổ đội và hàng đợi thực thi

> Bản diễn giải tiếng Việt của §8, trang in 49–55, tài liệu R&D phân tích ngược ngày **27/09/2026**. Không phải TDD chính thức của VOID hoặc xác nhận phiên bản bán lẻ hiện tại. Các `file:line` là **TDD locator**, chưa được kiểm chứng độc lập ở đây. TDD chưa đọc lại một số thân hàm thư viện gamepad; những nhận định tương đồng ở đó chỉ là suy luận.

## Một menu phụ thuộc thứ đang nhìn

Người chơi không chuyển sang camera chỉ huy hay tạm dừng thế giới. Họ tiếp tục di chuyển, ngắm và nhìn vào cửa, người, vật chứng, bẫy, điểm exfil hoặc khoảng trống; hệ thống chọn menu tương ứng. Gold là cả squad; mỗi officer thuộc Red hoặc Blue. Bàn phím/chuột dùng danh sách đánh số `USwatCommandWidget`; gamepad dùng bánh xe hai vòng `UCommandWheel`. Hai giao diện này có **hai đường dựng menu và dispatch riêng**, không chỉ hai cách vẽ cùng cây lệnh. [TDD §8, tr.49–50; TDD locator: `HUD/Widgets/SwatCommandWidget.h/.cpp`, `HUD/Widgets/Console/CommandWheel.cpp:16-22`, `lib/ReadyOrNotCommandFunctionLibrary.h/.cpp`.]

Lệnh đi từ kết quả trace `FHitResult` → `ContextualData` → trang `FSwatCommand` → chọn leaf → queue hoặc execute → lời thoại → timer → `DoCommand` → `USWATManager::Give*Command`. Một số case gọi thẳng controller/activity hoặc dùng RPC nhân vật. `USWATManager` là world subsystem có tick, không phải replicated actor; nó giữ map queue theo đội và cooldown prefix. Đừng đặt authority vào menu chỉ vì menu gọi được manager. [TDD §8, tr.50,53–54; TDD locator: `Info/SWATManager.h:41,77,171,241-316`, `HUD/Widgets/SwatCommandWidget.cpp:2801-2852`.]

## Trace không đơn giản là “vật gần tâm màn hình”

Trace nhiều hit bắt đầu từ camera FP, dài 10,000 units, bật complex collision, tìm WorldStatic/PhysicsBody/Pawn/Doorway/Item và loại volume. Người chết/gục đã còng, SWAT không active và trailer bị bỏ qua. Khi đang ở submenu, mọi character đều bị bỏ để người đi ngang không giật menu sang trang khác. Hit tường hoặc actor không triển khai `UCanIssueCommandOn` có thể chặn việc chọn đích phía sau. [TDD §8, tr.50; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:3231-3383`.]

Cửa cách dưới 100 units, là doorway-only hoặc mở ít nhất 75%, và không có đội stack sẽ được bỏ khỏi kết quả để chọn thứ phía sau. Khi hit doorway component của khung không cánh/cửa mở thấy được, một cửa commandable khác tại `HitResults[1]` có thể thành `ContextualData2`. Với actor không phải cửa, hệ chạy thêm single trace theo cùng tia, thêm WorldDynamic và Door nhưng bỏ Doorway; nó kiểm loại blocker khác, không phải “tia hẹp hơn”. [TDD §8, tr.50; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:3326-3450`.]

Trang hiển thị xét những ngoại lệ trước: đội có queue thì default thành Execute; solo chết cả squad thì tắt team orders; half-team chết thì chuyển đội; người chơi là known enemy thì trang chỉ còn `SC_KillMe`. Sau đó mới tạo world/location page và chuyên biệt theo actor. AI target có trang người; đối tượng đã còng hoặc down chưa báo cáo mặc định Report, đối tượng có thể bắt mặc định Arrest. Với evidence, trap, exfil và một số loại human, chỉ default đổi, trang vẫn là default page. Training override áp dụng cuối. [TDD §8, tr.50; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:1766-2171`.]

## Default command phải theo đúng thứ tự

Đối với cửa, chuỗi ưu tiên là:

1. Disarm nếu bẫy live có thể gỡ từ phía đang đứng.
2. Move and Clear nếu chỉ có khung cửa.
3. Default cửa mở nếu cánh đang mở.
4. Default kiểm cửa nếu chưa biết khóa và đội đã stack.
5. Default cửa chưa biết khóa nếu chưa stack.
6. Default cửa locked hoặc unlocked tương ứng.

Mặc định header: cửa mở `SC_MoveAndClear`, chưa biết `SC_StackUp`, kiểm khóa `SC_StackUp`, locked `SC_PickLock`, unlocked `SC_OpenAndClear`. `LoadDefaultCommands` ghi đè bốn mặc định cửa nhưng **không** ghi `DefaultCheckDoorCommand`. General default còn bị nạp vào biến local trùng tên member. Khả năng các default là setting người chơi là suy luận từ helper save/load. [TDD §8, tr.51–54; TDD locator: `HUD/Widgets/SwatCommandWidget.h:90-103`, `.cpp:1962-1963,2060,2234`, `lib/BpGameplayHelperLib.h:653`.]

Tap default không đơn thuần dùng lựa chọn đã hiển thị: nó trace lại rồi chạy một Cast chain khác. Không squad thì không làm gì; training dùng global default; known enemy dùng KillMe; queue active thì execute. Chain mới xử lý riêng trap gắn cửa và cho SWAT officer/người gục đã báo về global default. Vì hint và action được phân loại ở hai nơi, thay đổi một nơi có thể làm nút bấm không đúng nhãn. Đây là rủi ro cấu trúc được TDD xác định, không phải khẳng định mọi lần bấm đều sai. [TDD §8, tr.51,54; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:3624-3759`.]

## Queue lưu cả đích lúc chọn

Bàn phím giữ `HoldGoCode` khi chọn leaf để queue. Không queue khi default KillMe hoặc `bOverrideActiveTeamType`; cờ sau bật khi không SWAT và khi target là character, nên lệnh nhắm người không queue theo đường này. Gamepad dùng toggle `ShouldQueue`, prompt D-pad Down; mở wheel mới reset false. [TDD §8, tr.51; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:1689-1701,1952,2019`, `HUD/Widgets/Console/CommandWheel.cpp:96,279-294`.]

Mỗi `ETeamType` có một `FQueuedSwatCommand`, gồm command và hit result **tại thời điểm queue**. Khi Red/Blue queue trong lúc Gold đã có order, order Gold được chuyển cho half-team còn lại. Queue lệnh breach-and-clear còn phát Stack Up cùng target; nếu đã stack thì chỉ nói. Nhờ lưu context, khi người chơi quay đi rồi Execute, đội vẫn làm việc trên cửa đã chọn. [TDD §8, tr.51; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:1703-1750`.]

| Đội chọn khi Execute/Cancel | Quy tắc |
|---|---|
| Gold có order riêng | Chạy order Gold, xóa cả ba slot |
| Gold không có order riêng | Chỉ chạy nếu **cả Blue và Red** đều có queue; gọi Blue rồi Red |
| Red hoặc Blue | Chỉ chạy slot của mình |
| Cancel Gold | Xóa cả ba slot |
| Cancel Red/Blue | Chỉ xóa slot tương ứng |

`HasQueuedCommandForTeam` coi Gold có queue khi cả hai half-team có queue, phù hợp rule execute. Điều này không đồng nghĩa mọi lệnh bắt đầu đúng cùng một frame: mỗi lệnh vẫn đi qua phần thoại và timer riêng. [TDD §8, tr.51; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:1519-1568,1670-1671`.]

## Lời thoại là một phần thời gian gameplay

`ExecuteCommand` không gọi ngay `DoCommand`. Delay mặc định 1.0 s; nếu leader là người chơi và câu vừa rồi là prefix đội, delay 1.5 s; các trường hợp khác lấy độ dài clip hiện tại, giá trị không dương lại về 1.0 s. Mỗi lời gọi tạo timer riêng, tránh việc order sau đè order trước. Khi dispatch location, hit position dịch ra ngoài theo normal 50 units để LOS của AI hoạt động. [TDD §8, tr.51; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:1622-1644,2460`.]

Chu kỳ team Red → Blue → Gold bỏ qua đội chết. Mở menu và cycle có thể phát prefix nếu hết cooldown 0.6 s. Khi dựng trang cửa, hệ có thể tự chuyển sang half-team đang stack tại đó. Sau đóng menu, input inventory mở lại sau 0.3 s. Những khoảng ngắn này giải thích cảm giác lệnh có nhịp, không chỉ là độ trễ AI. [TDD §8, tr.52–53; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:521-526,3224,3524-3622`.]

Submenu đẩy parent vào stack, lưu key combo, actor hiện tại và `bHoldPageUntilExecute`. Khi rebuild, trang được giữ nếu actor không đổi hoặc hold flag bật; Fall-In formation và Focus officer dùng hold. Back lên một cấp và xóa hold. `PreviousActiveCommandPage` chỉ chứa một level, trong khi parent stack chứa lịch sử, cần phân biệt hai vai trò. [TDD §8, tr.51–52; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:180-183,726-729,1216,1837,3062-3134`.]

## Gamepad và dữ liệu trình bày

Khi gamepad active, widget bàn phím return khỏi tick. Wheel dựng trang mỗi tick, trong khi bàn phím mỗi 0.15 s. Stick magnitude dưới 0.1 xóa preselection; quay về tâm sau khi chỉ parent mở outer ring. Ánh xạ góc stick sang index được suy luận là Blueprint vì `CurrentIndex` cho Blueprint ghi và setter là event. Hai lệnh di chuyển cá nhân có free-view: chọn lệnh, ngắm vị trí rồi confirm. [TDD §8, tr.52–53; TDD locator: `HUD/Widgets/Console/CommandWheel.cpp:64-89,165-194,223-256`, `HUD/Widgets/SwatCommandWidget.cpp:63-91`.]

`ESwatCommand` khoảng 130 giá trị, bao gồm khoảng 20 container `PC_*`. `FSwatCommand` mang text, key, type, enabled, target, subcommands, lựa chọn context thứ hai và các flag giữ trang/lấy lại context. Wheel icon lấy data table. `URadialWidgetThemeData` điều khiển **weapon wheel**, không dùng cho command wheel. Team palette được khai bốn lần ở menu, entry, status và wheel, khác cả màu Red/Blue; không nên xem chúng là nguồn màu chuẩn duy nhất. Mapping TT_SUSPECT đỏ/TT_CIVILIAN xanh chỉ được xác nhận trong menu. [TDD §8, tr.52–54; TDD locator: `Enums.h:355-490`, `Structs.h:8-101`, `Data/RadialWidgetThemeData.h:35-95`, `HUD/Widgets/Console/CommandWheel.cpp:26-30`.]

Status HUD có năm row Alpha/Beta/Charlie/Delta/Lead, phản ánh vị trí, health và activity. Overlay `UCommandInterface` có icon hướng và Move-In/Fall-In/Cover/Deploy, nhưng nơi dùng chưa traced. Tách “lớp tồn tại” khỏi “tính năng đang hiện” giúp đọc UI thận trọng. [TDD §8, tr.49; TDD locator: `HUD/Widgets/SwatTeamStatusWidget.h:19-45`, `HUD/Widgets/CommandInterface.h:30-56`.]

## Authority, mở rộng và lỗi thiết kế

UI chạy trên client sở hữu. Team command gọi local subsystem; năm lệnh cá nhân dùng RPC qua player: MoveTo và MyPosition → `Server_GiveAIMoveTo`, Stop → `Server_StopAIMoveTo`, Exit → `Server_GiveAIMoveToExit`, TurnAround → `Server_GiveAITurnAroundOrder`. Lý do có thể là lệnh compliant NPC cần hoạt động ở multiplayer còn SWAT AI chỉ có nơi phù hợp; TDD đánh dấu đây là suy luận. Không SWAT thì widget ép TT_NONE và chặn team order. [TDD §8, tr.53–54; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:81-85,1952,2810-2852`.]

Thêm leaf cần enum, entry ở **hai** builder kể cả helper gamepad, voice case, dispatch ở hai nơi và icon row. Actor mới cần `UCanIssueCommandOn`, rồi sửa cả chain trang và chain default tap. Hook Blueprint gồm `OnPostUpdateSwatCommands`, `OnSwatCommandQueued`, `OnSwatCommandIssued`. TDD khuyên dự án mới dùng một cây lệnh chung, hai renderer, một kết quả default duy nhất và một palette asset. [TDD §8, tr.54–55; TDD locator: `HUD/Widgets/SwatCommandWidget.cpp:2165,2857`.]

Ngoài duplication, container của library có `PC_*` nhưng widget dùng `SC_None`, nên leaf test dùng chung dễ sai. `CommandCombo` library chỉ thấy ghi/xóa, không thấy C++ reader; Blueprint chưa kiểm. TDD không đọc thân `GrabContextData`, `UpdateCommandPageData`, `DoCommand` của library trong lượt này, nên không được gán toàn bộ chi tiết bàn phím thành kết luận đã đọc trực tiếp trên gamepad. [TDD §8, tr.54–55; TDD locator: `lib/ReadyOrNotCommandFunctionLibrary.cpp:124,1258,1985,3014,3741,3810,3844`.]

## Bài thực hành tái tạo

**Kịch bản tự thiết kế:** Red stack cửa A, Blue stack cửa B, người chơi quay sang evidence rồi Execute Gold. Owner chọn context là UI; server sở hữu queue được chấp nhận và giao activity. States: browsing → submenu → queued → dispatch pending → executing/cancelled. Phụ thuộc: trace classifier, command tree chung, target ID/context snapshot, đội, voice duration và dispatcher.

**Tiêu chí chấp nhận:** hai đội vẫn đi đúng cửa lúc queue; Gold chỉ chạy split order khi đủ hai slot; tap thực hiện đúng hint đã tính; đổi từ chuột sang gamepad không đổi eligibility; target mất trước timer được xử lý an toàn; đóng menu khôi phục input đúng hạn. Log thời điểm chọn, kết thúc thoại và bắt đầu activity để phân biệt UI delay với AI delay. Đây là thiết kế bài tập, không bổ sung claim về source.

Liên hệ [SWAT command và điều hướng](../02-Gameplay-Va-AI/08-SWAT-Command-Navigation.md), [tương tác với người/vật chứng](../02-Gameplay-Va-AI/03-Tuong-Tac-Bat-Giu-Vat-Chung.md), [state ownership](../04-Kien-Truc-Va-Quy-Trinh/03-Multiplayer-State-Ownership.md).

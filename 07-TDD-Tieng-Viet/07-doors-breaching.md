# 07. Cửa, khóa, bẫy và phá cửa

> Chương học này diễn giải §7, trang in 42–48 của bản R&D phân tích ngược ngày **27/09/2026**, không phải TDD chính thức của VOID hay tài liệu xác nhận bản thương mại hiện tại. `file:line` là **TDD locator**, chưa đối chiếu độc lập. Các điểm “suy luận”, “chưa kiểm chứng”, “không đọc lại” được giữ vì chúng giới hạn kết luận.

## Một cửa là cả hệ tương tác

`ADoor` quản lý cánh cửa, khóa, bẫy, tấm phá hủy, nav link, âm thanh và kiến thức AI. Cửa đôi là hai instance liên kết `DriveSubDoor`; lá `bMainSubDoor` sở hữu bẫy. Ngay cả khung không có cánh cũng dùng lớp này: `IsDoorwayOnly()` đúng khi row không có chunks và `DoorMesh`. Khung đó giảm tick xuống 1 s, vô hiệu tương tác và loại các component trừ blocker khỏi navigation. “Cửa đang mở” và “khung không cánh” vì thế khác nhau về dữ liệu dù cùng tham gia chọn lệnh. [TDD §7, tr.42; TDD locator: `Actors/Door.cpp:1015-1022,1037-1043,3572-3575`.]

`TypeOfDoor` chọn row `FDoorData`; `SetupDoor` chép sang `DoorData` replicate. `OnRep_DoorDataUpdated` áp mesh, chunks, góc tối đa và tham số đá trên mỗi máy. `UDoorwayComponent` đánh dấu thể tích cửa; mỗi `UDestructibleDoorChunkComponent` giữ health và hỗ trợ vật lý. Ram thực hiện trace ngắn 50 units rồi gửi point damage; breaching shotgun là marker, cửa quyết định phản ứng. Wedge gỡ được bằng multitool, còn ram có thể loại wedge trực tiếp. [TDD §7, tr.42–43; TDD locator: `Actors/Door.cpp:3070-3241`, `Actors/Items/DoorRam.cpp:75-88`, `Actors/Items/DoorJam.cpp:62-78`.]

## Từ thao tác tới một góc quay

Cửa có 11 timeline và 12 slot curve. Mở/đóng dùng chung timeline, chọn curve theo dấu góc đích. Callback nội suy `LastStartAngle → LastTargetAngle` rồi ghi `OpenCloseAmount`; ba callback khác ghi góc tay nắm, rattle thêm offset từ curve. Mặc định chạy timeline mới dừng timeline khác, nhưng caller có thể giữ chuyển động tay nắm song song cánh cửa. Client làm mượt mesh/chunks chưa physics tới scalar replicate bằng `RInterpTo`, speed 15. [TDD §7, tr.43,47; TDD locator: `Actors/Door.h:1805-1838,1923-1924,1967-1977`, `.cpp:1278-1289,5024-5037,6802-6911`.]

Ba ngưỡng dễ nhầm: `IsOpen` khi `|yaw| ≥5°`; `IsClosed` khi yaw trong 1° quanh 0 hoặc `OpenCloseAmount==0`; `CloseThreshold=75°` chỉ phục vụ phép “mở vượt ngưỡng đóng”, không định nghĩa trạng thái đóng. Khung không cánh, mesh đã tách hoặc major chunks đã mất đều có xử lý riêng. `MaxOpenClose` là trị tuyệt đối góc row, mặc định 90°. [TDD §7, tr.43; TDD locator: `Actors/Door.cpp:3072,3672-3691`, `.h:140,1742-1753`.]

Peek là toggle 15°: nếu góc đích đã đạt increment thì quay về 0; ngược lại mở ra xa người soi, đảo dấu cho cửa một chiều khi cần. Nếu đã mở quá increment, lời gọi không làm gì. Peek hoặc open đặt bộ đếm số kick cần thiết về 0. Chạm push lên cửa đóng, có thể khóa (`lockable`), đội người dùng chưa biết lock state bắt đầu hold; điều này không đòi hỏi cửa thực sự đang khóa. Thanh tiến độ đi từ 0.2 đến 0.45 s, nhả trước 0.2 s chuyển sang peek. Lockpick mất 4 s. [TDD §7, tr.42–43; TDD locator: `Actors/Door.cpp:1504,1514,2240-2253,2347-2360,5837-5927,6021,7435-7438`.]

## Khóa, kiến thức và bẫy

Push, peek, open đều tiết lộ lock state cho **cả SWAT và nghi phạm**, bất kể ai chạm. Mở bằng multitool chỉ thông báo đội người dùng. Các bit knowledge là tri thức của đội, không phải trạng thái khóa vật lý. `bAlwaysLocked` ép khóa; nếu không, chỉ roll `FRand() <= LockedChance` khi `bOverrideLockChance` bật, mặc định 0.2. Cửa không override do hệ khác chọn khóa, hệ đó chưa xác định trong phần này. [TDD §7, tr.43; TDD locator: `Actors/Door.cpp:2913-2934,5736-5737,5855-5856,5957-5958,7452`.]

Cửa điện kiểm keycard đội để mở. Thử cửa điện còn khóa kích alarm một lần, phát hearing event có max range 0; nghĩa “không giới hạn range riêng của event, để listener quyết định” là suy luận từ ngữ nghĩa engine. Không thể hiểu 0 là không ai nghe. [TDD §7, tr.43; TDD locator: `Actors/Door.cpp:3049-3068`.]

`SetupTrap` chạy server: bỏ qua khi `bNoSpawnTrap`; lá phụ lấy bẫy lá chính; lấy class từ `FTrapData`, lật transform nếu phía Back. Có hai trace visibility: một trước spawn, một sau spawn để đặt dây; đường khởi tạo dùng `SpawnActorDeferred` và `TrapInit`. Hàm này không roll ngẫu nhiên. Việc chọn `TypeOfTrap/bNoSpawnTrap` diễn ra nơi khác, chưa kiểm chứng. Đừng thêm xác suất vào bước đặt chỉ vì game có biến thể nhiệm vụ. [TDD §7, tr.43–44; TDD locator: `Actors/Door.cpp:3359-3529`.]

## Đá, ram và shotgun

Xác suất kick được TDD ghi là:

`FinalKickChance = clamp(DoorKickSuccessChance − (locked ? LockedChance/10 : 0) − KickChanceOffset, 0, 1)`.

Row mặc định 0.7, clamp `[0.1,1]`, thay default actor 0.9. Mỗi lần thử tăng 0.05 dù thành/bại; roll thành công giảm bộ đếm kick, mặc định row 1 và clamp 0–100; tới 0 mới mở. `bKickAlwaysFails` ép thất bại; trait `Kicker` có cơ hội bỏ qua roll. Chân phải đã bị thương mất thêm một ticket trước khi thử. Cửa một chiều chặn kick sai phía; nhánh được phép có thể làm cánh rời. [TDD §7, tr.44; TDD locator: `Actors/Door.cpp:3074-3075,3592-3618,6536-6668`, `.h:204,1774`.]

Cửa đôi không roll riêng cho lá phụ: kick thành công gọi `KickSubDoor`, counter lá phụ đã 0, nên nó mở nếu predicate/one-way cho phép; không tốn thêm ticket chân hay phát alarm riêng. Bẫy và độ bền cần được kiểm ở từng nhánh, không suy ra “một hit = cùng kết quả cho cả hai cánh”. [TDD §7, tr.44; TDD locator: `Actors/Door.cpp:6605,6670-6743`.]

Ram từ chối nếu hết hinge, timeline ram đang chạy hoặc `bCanUseRam=false`; còn lại mở khóa, tháo wedge, lộ bẫy cho hai đội và swing tới ±max. Nếu bật âm ram, morale damage lấy `BatteringRamMorale.*`, nhân `max(Breacher,1) × RepeatBreachDamageFactor^TimesBreached`; fallback factor 0.25, từ lần breach thứ 3 trở đi multiplier 0. `BodyRamDoor` cũng swing/unlock/tháo wedge nhưng không morale và không kiểm `bCanUseRam`. Ragdoll nhanh hơn 50 va vào cửa unlocked, chưa vượt close threshold cũng gọi ram. [TDD §7, tr.44; TDD locator: `Actors/Door.cpp:4912-4918,6174-6311`.]

Shotgun phá cửa ánh xạ khoảng pellet 100–1000 sang `BreachPushAngle` 45°–15°, clamp; bắn gần đẩy rộng hơn. Với cửa destructible, phá chunk bị trúng, xét support và breach nếu chunk không là hinge. Đạn khác chỉ gây damage chunk, bằng 1.0 nếu causer là character, còn lại dùng full damage. `BreachDoor` kiểm `bCanUseBSG`; nhánh from-point alarm, unlock, snap góc theo increment và giới hạn one-way `[0,Max]`, loại khác `[−Max,Max]`. Morale shotgun dùng cùng dạng lũy thừa nhưng thiếu fallback 0.25 và 3 như ram. [TDD §7, tr.44–45,47; TDD locator: `Actors/Door.cpp:4854-4899,6322-6386`.]

## Nổ và cấu trúc phá hủy

C2 gọi `ExplodeDoor(..., bKeepHinges=true)` rồi return. Cửa có chunk phá mọi phần trừ hinge với impulse 2500; cửa nguyên tấm tách mesh 100 kg, impulse 15,500, scale 0.92. Trap loại phá hết chunks dùng đường tương tự nhưng impulse chunk 300. Grenade projectile không phải stun gọi collapse rồi explode với `false`, không giữ hinge; collapse tấm dùng 50 kg, impulse 1000 tại hit point, scale 0.98. Stun có thể đẩy `DoorPushScale ×MaxOpenClose` hoặc phá hết chunks tùy damage type. [TDD §7, tr.45; TDD locator: `Actors/Door.cpp:4728-4850,6397-6534`.]

Cuối `TakeDamage`, nếu chunk nào đã hỏng thì `BreakDoor(false)`. C2/trap/grenade/ram return trước đoạn này; không được giả định chúng đi qua nó. `BreakDoor` chỉ chạy một lần, phát alarm, trigger attached trap và đặt `bDoorBroken`. Cửa có lưới 3×3: 0–2 hàng dưới, 0/3/6 cột hinge. Support graph được dựng bằng chỉ số kề; field support tự khai trong data đã comment. `Restore` đặt chunk health 1500, mass 75 kg. Major chunks là 1,4,5,7,8. [TDD §7, tr.45; TDD locator: `Actors/Door.cpp:3122-3144,4920-4923,6745-6780,7084-7091`, `Components/DestructibleDoorChunkComponent.cpp:64-103,143-152`.]

| Dữ liệu mặc định | Giá trị / ý nghĩa |
|---|---|
| `PhysicalPushAmount` | 30° |
| `ElectronicLockChance` | 0.0; consumer chưa đọc |
| `bCanUseC2/BSG/Wedge/Ram/Optiwand` | Đều true |
| `FTrapData::InvertOffset` | 5.0 |
| Kill/stun distance: blasting | 300 / 500 |
| Kill/stun distance: shotgunning | 100 / 200 |
| Kill/stun distance: ramming, kicking | 0 / 100 |

Các cặp kill/stun được constructor thiết lập và editor vẽ; TDD chỉ nêu cặp blasting có consumer runtime, không chứng minh cả bảng luôn là damage radius cho mọi dụng cụ. Dòng giải thích consumer ở trang 46 bị cắt trong bản trích xuất, nên không phục dựng phần mất. [TDD §7, tr.46,48; TDD locator: `Actors/Door.cpp:496-506`, `.h:295,1012-1028,1742-1802`, `Characters/PlayerCharacter.cpp:2650-2672`.]

## Liên kết AI và mạng

Door nav link kiểm async path giữa hai điểm ±50 quanh cửa, lặp mỗi 0.25 s. Path dài hơn 500 hoặc có custom link bật nav link; đường ngắn trực tiếp tắt; query thất bại bật. AI nhìn cửa SWAT vừa dùng, trong 500 units và lệch cao dưới 150, lập tức biết SWAT user; sau đó mới xét quyền investigate. Door noise được nhớ 5 s nếu nổ, 30 s loại khác. Radius move/open 500 hoặc 1250; kick lấy config fallback 1000; ram 1300; nổ 1500. [TDD §7, tr.45–46; TDD locator: `Actors/Door.cpp:5302-5427,7467-7522,4252,4267,4431`.]

Các mutator đã đọc đều return dưới `ROLE_Authority`. Replication gồm dữ liệu cửa/bẫy, lock/broken/jammed/knowledge, scalar góc, chunks, trap/wedge, last user/damage. Phần danh sách replicate không được TDD đọc lại; việc Tick timeline có guard role cũng chưa kiểm. Đa số RPC là multicast hình/âm; hai server RPC ghi knowledge khóa/bẫy. [TDD §7, tr.47; TDD locator: `Actors/Door.cpp:944-978,1210-1214`, `.h:919-926,1876-1880`.]

## Lỗi cần hiểu trước khi tái sử dụng

- Tỷ số ticket chân dùng chia `int32`: chân thương một phần làm `1 − remaining/max` thành 1, ép roll kick về 0; trait ép thành công vẫn có thể qua. Ticket còn bị trừ trước roll.
- Điều kiện một chunk đồng thời bằng 0 **và** 3 **và** 6 không bao giờ đúng: hinge được giữ khỏi phá nhưng không đổi collision sang ignore pawn.
- One-way BSG đã clamp góc không âm rồi kiểm `<0`, làm nhánh break không tới được; kick một chiều phía bị chặn cũng chỉ tới nhánh break bằng forced kick.
- Support check bị gọi qua multicast rồi gọi implementation trực tiếp: server chạy hai lần, tính lặp được TDD suy luận là thừa.
- Lá chính cửa đôi đã comment nhánh detach thường, lá phụ còn chạy nên chỉ partner có thể bung. `SetLocked` khi cửa đang mở/chuyển động lại gọi unlock rồi bỏ yêu cầu khóa.

TDD cũng cảnh báo constants detach lặp ở nhiều nhánh và morale BSG khác ram khi thiếu config. [TDD §7, tr.47–48; TDD locator: `Actors/Door.cpp:4457-4467,4880-4881,4991-5006,6347-6353,6561-6563,6576-6578,6622-6639,6725-6728`.]

Mở rộng door type bằng row, trap bằng subclass và row, cảm giác chuyển động bằng curve. Dụng cụ mới cần nhánh damage, enum, âm và morale, đồng thời chọn rõ có gọi `BreakDoor` hay không. Nên gom constants tách cánh vào helper, dùng graph support rõ ràng và phép chia float cho xác suất. [TDD §7, tr.47–48.]

## Bài thực hành tái tạo

**Kịch bản tự thiết kế:** một cửa đơn khóa, có wedge và bẫy giả, hai đội sở hữu kiến thức riêng. Server sở hữu lock, jam, trap state, yaw target; client làm mượt yaw. Trạng thái: closed → peeking/opening → open → closing; damaged/detached là trạng thái cấu trúc độc lập. Phụ thuộc: curve, trace, knowledge flags, tool interface, support graph đơn giản và event âm thanh.

**Tiêu chí chấp nhận:** giữ kiểm khóa và nhả nhanh tạo hai hành vi khác nhau; trap trigger đúng một lần; ram tháo wedge theo quy tắc đã định; request khóa khi đang swing có kết quả công khai, không âm thầm đổi nghĩa. Trong công thức float đề xuất cho bài tập, chân còn 3/4 vé cho tỷ lệ 0.75 và offset `1 − 0.75 = 0.25`; xác suất cuối vẫn phải trừ offset này cùng phạt khóa khỏi xác suất cơ sở rồi clamp. Client trễ mạng phải nhận đúng góc cuối. Bài tập có thể chỉ dùng một panel, rồi mới thêm phá hủy từng chunk.

Đọc kèm [cửa và thiết kế căn phòng](../02-Gameplay-Va-AI/04-Cua-Va-Breaching.md), [SWAT command/navigation](../02-Gameplay-Va-AI/08-SWAT-Command-Navigation.md), [asset, Blueprint và level](../04-Kien-Truc-Va-Quy-Trinh/02-Asset-Blueprint-Level.md).

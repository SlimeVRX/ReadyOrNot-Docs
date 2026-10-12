# 06. Trang bị, inventory, lựu đạn và vật đặt trong thế giới

> Diễn giải học tập từ §6, trang in 35–41, tài liệu R&D phân tích ngược ngày **27/09/2026**; không phải TDD chính thức của VOID hay xác nhận bản thương mại hiện tại. Mọi `file:line` là **TDD locator**, chưa kiểm chứng độc lập trong chương này. Các mặc định header có thể được Blueprint ghi đè. Nội dung chỉ mô tả ý tưởng và cơ chế, không cung cấp source hay asset gốc.

## Tách vật cầm khỏi vật đã ném

Inventory runtime trả lời nhân vật sở hữu gì, đang cầm gì, đang cất hay rút gì. Màn hình loadout trước nhiệm vụ thuộc hệ quản lý riêng. Sau khi ném hoặc đặt, vật có actor đồng hành quản lý vòng đời: grenade dùng `AThrownGrenade`, chemlight dùng `AThrownChemlight`, C2 dùng `APlacedC2Explosive`. Tách hai vai trò giúp vật tiếp tục tồn tại khi người ném đổi vũ khí. TDD đếm 23 cặp lớp trong `Actors/Items/`, gồm cả base chung, không nên diễn giải thành 23 món trang bị độc lập. [TDD §6, tr.35–36; TDD locator: `Components/InventoryComponent.h/.cpp`, `Actors/Gameplay/ThrownGrenade.h`, `Actors/Gameplay/PlacedC2Explosive.h/.cpp`.]

`UInventoryComponent` giữ danh sách và yêu cầu equip; `UItemVisualizationComponent` là skeletal mesh chỉ dành người chơi local, phản chiếu mesh trang bị lên cơ thể; nó không phải camera Optiwand. `ATool` là base thao tác lên target, replicate `bOperating`, thời gian hiện tại/tối đa và `TargetActor`. `AMultitool` chọn Lockpick/Knife/Wirecutter, có thể lấy thời gian từ event FMOD. `ALockpickGun` và `ABreachingShotgun` là subclass rỗng, hành vi quan trọng nằm trong base hoặc target. [TDD §6, tr.35–36; TDD locator: `Components/ItemVisualizationComponent.cpp:106`, `Actors/Items/Tool.cpp:9-12`, `Actors/Items/Multitool.h:33-39`.]

## Đổi vật bằng một yêu cầu có danh tính

Không có một con trỏ equipped đơn giản làm nguồn sự thật. `FItemChangeRequest` chứa `ChangeId`, `FromItem`, `ToItem`, `bInstant`, `bNoDraw`, `bIsComplete`; so sánh hai request chỉ dựa vào GUID. Client sở hữu áp dụng ngay để phản hồi nhanh, chạy `OnRep_ItemChangeRequest`, rồi gửi reliable `Server_ChangeEquippedItem`. Server lưu request và chạy cùng OnRep. Khi bản replicate quay lại client, GUID đã thấy giúp không phát animation lần hai. [TDD §6, tr.37; TDD locator: `Components/InventoryComponent.h:13-54`, `.cpp:698-720,809-918`.]

Trong lúc đổi, `GetEquippedItem()` vẫn trả `FromItem`; khi complete trả `ToItem`, và trả null nếu vật không còn trong inventory. Timer cất đồ dùng độ dài montage trừ `0.1 s` ở các đường FP/TP, kể cả TP crouch. Vì thế chỉnh animation có thể đổi thời điểm gameplay coi vật đã được cất. `ReattachAllGear` gắn vật không cầm vào socket và kiểm quy tắc hiển thị. Các danh sách category, class hiện và class ẩn là mảng C++ trong header; ưu tiên chính xác giữa chúng và ngoại lệ khiên/giáp chưa được TDD kiểm lại. [TDD §6, tr.37; TDD locator: `Components/InventoryComponent.cpp:440,452,527,1096-1241,1288-1292`, `.h:237-239`.]

## Lựu đạn là đường bay dựng trước

`ABaseGrenade` dùng dữ liệu cho nhiều loại grenade. Mặc định `ThrowImpulse=220`, `UpImpulse=52.5`. Khi ném, hệ dự đoán đường projectile bằng trace cho cung đầu và tối đa hai đoạn nảy; vận tốc khởi đầu mỗi nảy là `ThrowDirection ×150 ×GrenadeBounciness`. Danh sách `CompletePath` dùng `FVector_NetQuantize` cùng ba bounce index được gửi lên server. Vật bay tắt physics, chạy dọc điểm bằng `VInterpTo_Constant`; tới điểm cuối mới bật profile `PhysicsItem` và physics, lấy hai điểm cuối gợi ý vận tốc. Đây là chuyển động theo kịch bản cho tới lúc bàn giao physics: cửa hoặc cơ thể di chuyển sau khi ném không làm đường cũ tự đổi. [TDD §6, tr.37,40; TDD locator: `Actors/BaseGrenade.h:136,288-292`, `.cpp:1034-1131`, `Actors/Gameplay/ThrownItem.cpp:246-333`.]

`Detonate()` tăng `CurrentDetonations`, đặt lại timer khi `CurrentDetonations <= RedotonateCount`. Tên field trong nguồn viết đúng như vậy; nó là float mặc định 0. Sau tiếng nổ đầu dùng `ReDetonationTime=0.5 s`. Khi bật tăng bán kính theo thời gian, bán kính từng entry chia cho số lần nổ còn lại. Morale chỉ áp dụng ở lần đầu và đọc `GrenadeDetonateMorale.*` từ AI config, không đọc các UPROPERTY tương tự. [TDD §6, tr.37; TDD locator: `Actors/BaseGrenade.h:342,351`, `.cpp:1258-1275,1378-1438`.]

Sát thương vùng có “second trace”: sphere overlap outer radius, trace từ tâm tới từng component; nếu bị chặn và mục tiêu trong `OuterRadius × SecondTraceRadiusFactor`, thử năm gốc cao hơn: thẳng lên và bốn hướng chéo lên tại `SecondTraceDistance`. Cách này cho vụ nổ vòng qua vật chắn thấp, không phải hình nón sát thương. Âm thanh ngoài trời được xét bằng trace 5000 units dọc trục up; không hit hoặc hit actor tag `IsOutside` thì đặt tham số FMOD ngoài trời. [TDD §6, tr.37; TDD locator: `Actors/BaseGrenade.cpp:1278-1300,1598-1760`.]

## Công cụ đặc biệt và các mặc định

| Vật / dữ liệu | Mặc định được TDD ghi nhận |
|---|---|
| Detonator | Chỉ nổ charge cách chủ sở hữu dưới `MaxDetonationDistance=2000` |
| Taser | `MaxCableLength=70`, `CableGainRate=70`, `ProbeMaxDistance=2000`, `PingStunDuration=8` |
| Pepper spray | `DurationFrontFace=16`, `DurationFrontTorso=12`, `DurationBackFace=8`; abuse debounce 2 |
| Pepperball | 5 headshot hoặc 10 stun shot tới ngưỡng abuse |
| Zipcuffs | Khoảng bắt 200, thời gian bắt `2.0 s` |
| Door ram | Trace tối đa 50, `StrikePlayerDamage=200` |
| Ladder | `MaxRollDegreesBeforeUnwalkable=30°` |

Đây là header defaults, không bảo đảm đúng mọi Blueprint. Đặc biệt thời lượng stun thực có thể do character override quyết định; xem [chương sát thương](05-damage-health-arrest.md). Việc hai vị trí frame của ladder dùng làm điều kiện “đã ổn định”, chi tiết khiên phối hợp pistol/kính/tư thế và viewfinder drone đều chưa kiểm chứng trong lượt đọc nguồn. [TDD §6, tr.35–38; TDD locator: `Actors/Items/Detonator.h:30`, `.cpp:73-80`, `Taser.h:58-85`, `Pepperspray.h:91-107`, `PepperballGun.h:32-38`, `Zipcuffs.h:72,84`, `DoorRam.h:43,79`, `TelescopicLadder.h:58,68-69`.]

Chemlight cần ammo để ném. Vật đã ném tăng sáng với `InitialGlowSpeed=2`, bắt đầu giảm sau `StartDimTime=1800 s` với `LightDimSpeed=30`, hủy sau `TotalLifeTime=2000 s`. Damage trên 5 làm thử trace xuống 10 units để tạo decal; dù có hit hay không, mesh bị xóa và đèn tắt dần. Đây là lifetime riêng, không dựa vào grenade fuse. [TDD §6, tr.37; TDD locator: `Actors/Gameplay/ThrownChemlight.h:37-53`, `.cpp:17-102`.]

Optiwand dùng `USceneCaptureComponent2D` ghi vào render target, gắn socket `tag_camera`, FOV 90°. Khi soi cửa, lấy góc từ mirror point phía đối diện cộng offset; pitch giới hạn −60°..60°. `CollisionTraceDistance=25` có tồn tại nhưng cách dùng chưa traced. Chỉ `bRepMirroring` replicate; người khác thấy trạng thái soi, không nhận render target. NVG dùng scalar trên material parameter collection toàn cục và thay LUT hậu kỳ. [TDD §6, tr.36,38; TDD locator: `Actors/Items/Optiwand.cpp:39-52,173-176`, `.h:40,120-121`, `NightvisionGoggles.cpp:61-111`.]

Door wedge, trap, door và bomb cùng triển khai `ICanUseMultitoolOn`: target cung cấp loại thao tác, thời gian, điều kiện và xử lý hoàn tất trên server. Trap trả 3.0 s; tick interval co giãn từ `0.0167` tới `0.25 s` khi khoảng cách local player tăng 500–5000 units, rồi thành 1 s khi không thể gỡ. Depot tách `UDeployableData` cho tên/mô tả/ảnh/label và component đặt trong level chứa label cùng `ItemClass`. Header nói kit có label phù hợp được spawn khi bắt đầu; vòng gọi spawn nằm ngoài phần đã đọc nên chưa kiểm chứng toàn luồng. [TDD §6, tr.38; TDD locator: `Interfaces/CanUseMultitoolOn.h`, `Actors/Gameplay/TrapActor.cpp:463-466,627-645`, `Components/DeployableSpawnComponent.h:15-21`.]

`EItemCategory` trộn slot, nhóm phím và loại vật trong 51 giá trị, khác `EItemClass` và `EItemVisualizationType`. Với dự án mới nên tách ba ý nghĩa thành dữ liệu riêng. `FGrenadeDamage[]` là struct inline của grenade; tuning mỗi item chủ yếu là UPROPERTY/CDO, không phải một data table chung. [TDD §6, tr.38; TDD locator: `Enums.h:107-116,581-603,773-826`, `Actors/BaseGrenade.h:9-46`.]

## Authority và những chỗ không nên tái tạo nguyên xi

Inventory replicate gear, danh sách, selected device, loadout, removed items và request mới nhất. Nhưng validator equip luôn true, không kiểm `ToItem` thuộc inventory. `GetEquippedItem` lọc sau không thay thế được validation khi nhận. `CanEquip` trên component cũng luôn true; các nơi thực dùng gọi `Item->CanEquip`. `AddInventoryItem`/`RemoveInventoryItem` không tự chặn non-authority, removal chỉ OnRep local; đúng sai phụ thuộc caller. Chỉ destroy có role check rõ. [TDD §6, tr.38–40; TDD locator: `Components/InventoryComponent.cpp:39-44,906-928,1294-1380`.]

Grenade tin client cả đường bay và stream physics. Chu kỳ gửi `0.025 s`, dừng dưới 50 units/s; server nhận nguyên velocity và chỉ teleport mesh khi lệch hơn `MaxDistanceDifference=150`. Validator luôn true, nên TDD cảnh báo client sửa đổi có thể tự dời grenade. Bản tái tạo cần server tính hoặc kiểm giới hạn quỹ đạo và hiệu chỉnh. [TDD §6, tr.39–40; TDD locator: `Actors/BaseGrenade.h:235-254,509-516`, `.cpp:959-1032,1874-1892`.]

C2 damage target nằm trong server RPC, nhưng stun phía sau cửa nằm trong multicast: hard-code damage 100, radii 100/300. Field `DamageInnerRadius=200`/`OuterRadius=600` không còn dùng vì lời gọi đã comment. Khả năng receiver client bỏ qua damage chưa kiểm chứng ở phần này. Gọi multicast rồi gọi thẳng implementation tạo lượt lặp server; `bDetonated` chặn lặp. Gỡ charge quét mọi detonator bằng actor iterator, tốn O(số detonator) và xóa liên kết xuyên chủ sở hữu. [TDD §6, tr.39–40; TDD locator: `Actors/Gameplay/PlacedC2Explosive.cpp:123-172,231,313-328`, `.h:67-70`.]

Trap chỉ replicate `TrapStatus`. Mỗi máy kiểm foot bone với hộp tripwire; client RPC trên actor level-owned dự kiến bị UE từ chối theo ownership, nhưng engine path chưa traced. C++ không đặt `TS_Activated`; tắt collision không chặn phép kiểm hình học, nên nếu Blueprint không đổi trạng thái thì có thể trigger mỗi tick. Server còn gọi trigger hai lượt trong một phát hiện. Penalty được bảo vệ bởi authority và `bGiven`, nhưng âm thanh/event chưa có cùng bảo đảm. [TDD §6, tr.39–40; TDD locator: `Actors/Gameplay/TrapActor.cpp:191-196,280-315,606-615,718,760`, `Info/ScoringManager.cpp:623-638`.]

Mở rộng item bằng subclass thích hợp và virtual dùng đồ/rút đồ/điều kiện equip. Grenade mới thường chỉ cần damage entries, số lần nổ và config morale; gas cần actor/manager riêng. Target multitool mới triển khai interface; function mới thêm enum và ánh xạ âm thanh/thời gian. Quy tắc hiển thị cơ thể nên chuyển sang dữ liệu, sát thương phải ra khỏi multicast. [TDD §6, tr.39–40.]

## Bài thực hành tái tạo

**Kịch bản tự thiết kế:** đổi pistol sang công cụ gỡ khóa trong lúc một quả bóng đánh dấu đang bay. Owner inventory và kết quả thao tác là server; client dự đoán animation. Trạng thái inventory: idle → holstering → drawing → complete; vật ném: prepared → scripted flight → physics → expired. Phụ thuộc: request ID, animation duration, target interface, path predictor, replicated state.

**Tiêu chí chấp nhận:** echo của cùng ID không phát animation hai lần; target ngoài inventory bị từ chối; mất target giữa thao tác hủy an toàn; thêm target multitool không sửa inventory; quả ném không tự nhận path vượt giới hạn từ client; trap chỉ phát một event mỗi lần armed. So sánh hai thử nghiệm cửa đứng yên và cửa đóng giữa đường bay để thấy giới hạn của đường dựng trước. Các quy tắc kiểm này thuộc bài tập.

Liên hệ [input, inventory và equip](../03-Gun-Gameplay/02-Input-inventory-equip.md), [player health và inventory](../02-Gameplay-Va-AI/05-Player-Movement-Health-Inventory.md), [network authority](../03-Gun-Gameplay/09-Network-authority.md).

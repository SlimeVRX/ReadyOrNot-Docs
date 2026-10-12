# 03 — Player: di chuyển, tư thế, tương tác và điều khiển

Chương học này dựa trên **PDF §3, trang in 15–21**, bản R&D reverse-engineered độc lập ngày 27/09/2026; không phải TDD chính thức của VOID Interactive. Hành vi được mô tả thuộc snapshot PDF khảo sát, không khẳng định bản retail mới nhất hoặc source hiện tại giống hệt. Các `file:line` là **định vị do TDD ghi**, chưa được chương này kiểm tra độc lập. Các kết luận suy luận, chưa xác minh và đường code bị compile out được giữ riêng.

## 3.1. Cảm giác chậm đến từ nhiều lớp luật — PDF §3, tr. 15–16

Snapshot không dùng một state machine duy nhất để quyết định toàn bộ movement. Nhiều hệ số nhỏ được kết hợp mỗi tick thành `MaxWalkSpeed`, còn animation, khóa hành động và focus tương tác có thể từ chối input. Do đó “đã nhấn phím” chưa đồng nghĩa “hành động đã bắt đầu”. Đây là cách nhịp chơi có cân nhắc trở thành cơ chế thực tế.

Ba macro `RON_NO_JUMP`, `RON_NO_SPRINT`, `RON_NO_INCREMENTAL_SPEED` được định nghĩa không điều kiện. Jump binding và thân `PlayerJump/Jump` bị loại bởi `#ifndef RON_NO_JUMP`; `Sprint()` đi sang `FastWalk()`, `Walk()` sang `EndFastWalk()`; `IsSprinting()` luôn false; `IncrementalUse` trả về ngay. Input tên `Walk` lại bind vào `Sprint` khi nhấn. Kết quả người chơi có nhịp mặc định và slow walk giữ/toggle, dù tên hàm gợi điều ngược lại.

**Định vị do TDD ghi:** `ReadyOrNot.h:290-292`; `Characters/PlayerCharacter.cpp:5,375-380,439-441,7060,7089,7127-7128,7238-7240,7274-7276,7419-7421`. Đây là cấu hình trong snapshot; không nên dùng tên biến hoặc code ngủ làm bằng chứng rằng người chơi có sprint hay vault hoạt động.

Mỗi công cụ có action riêng: C2, breaching shotgun, pepper spray, door jam, mirror gun, flashbang, CS gas, stinger, zip-cuffs, multitool. Cấu trúc không phải một dãy quick-slot số dùng chung. Low-ready, ADS, free-look và lean tạo nhiều tư thế liên quan nhưng không đồng nhất.

## 3.2. Các lớp phối hợp input và cơ thể — PDF §3, tr. 15–16

| Lớp | Trách nhiệm cần nhận diện |
|---|---|
| `AReadyOrNotCharacter` | Base ở gốc module; sở hữu stun, `IsStunned`, `bRepStunned`; PDF suy luận lean/yell handler nằm ở đây |
| `APlayerCharacter` | Binding, intent flag, tốc độ/gia tốc, low-ready trace, ADS, camera và weapon meshspace |
| `UReadyOrNotCharMovementComp` | CMC tùy biến: crouch, aim offset AI, acceleration proxy và chặn velocity |
| `UInteractableComponent` | Cổng phía object; quyết định có dùng được không, nhận `ActionSlot1..4` nào |
| `AReadyOrNotPlayerCameraManager` | Thay modifier mặc định bằng `URONCameraModifier_CameraShake`; hỗ trợ shake cục bộ/vùng |
| `AReadyOrNotPlayerController` | Tám asset aim-assist: High/Medium/Low/Off cho hip và ADS; hệ số pitch/yaw |
| `AForceLowReadyVolume`, `ASlowDownVolume` | Hook designer đặt trong level để tác động tư thế/tốc độ |

CMC đặt `CrouchedHalfHeight = 75`, cho crouch, tắt `bUpdateOnlyIfRendered`. `PerformMovement` gọi `LookAt/UpdateAimOffset` cho AI; `SimulateMovement` giữ acceleration replicated khi `bHasReplicatedAcceleration`; `CalcVelocity` đưa velocity về 0 khi owner bị movement-lock hoặc đang được mang. InputRemapping được TDD mô tả là helper đổi phím trên action/axis mapping thông thường, nhưng **file plugin chưa được mở ở lượt khảo sát đó**.

`SetupPlayerInputComponent` phối `BindAction/BindAxis` và `BindingNoConsume`. Cái sau tắt `bConsumeInput`, bật `bExecuteWhenPaused`, dùng cho Use nhấn/thả/double-click. Handler kiểm tra gate rồi đổi intent như `bHoldingFastWalk`, `bAiming`, `bUserLowReady`, `bWantsToCrouch`. `PlayerControlledOnlyTick` chỉ tính block tốc độ ở authority và owning client; presentation chạy qua camera/bob/recoil/FOV. `LastInteractableComponent` lưu focus và được replicate.

**Định vị do TDD ghi:** `Components/ReadyOrNotCharMovementComp.cpp:10-71`; `Characters/PlayerCharacter.cpp:353,360-430,822-861,1195,1621-1622`; `ReadyOrNotCharacter.h:1828,1920`; `Characters/ReadyOrNotPlayerController.h:1022-1043,1219-1222`; `ReadyOrNotPlayerCameraManager.cpp:9-10`.

## 3.3. Tính tốc độ: tách trần tốc độ khỏi input hướng — PDF §3, tr. 16–18

Phép tính theo TDD có dạng:

`MaxWalkSpeed = GetRunSpeed() × SprintRamp × HitSpeedMultiplier × TempCurrentRunSpeedPercent × SlowDownSpeedMultiplier × StunRepFactor × BleedMultiplier × PVPSpeedMultiplier`

Tốc độ crouch dùng cùng tích rồi nhân `SpeedModifier_Crouch = 0.5`. `StunRepFactor` bằng 0,75 khi `bRepStunned`, ngược lại bằng 1. Khi kết quả đổi, `SetWalkSpeed` gửi giá trị qua mạng. Để debug, log từng thừa số sẽ rõ hơn chỉ log tốc độ cuối.

`GetRunSpeed()` có thứ tự nhánh quan trọng: bị bắt dùng `RunSpeed × 0.25`; low-ready dùng `RunSpeed × 1.2`, bỏ qua multiplier item và armor; bình thường dùng `RunSpeed × item MovementSpeedMultiplier × armour GetArmourSpeedMultiplier()`, thêm `StunMovementSpeedMultiplier = 0.75` khi `IsStunned()`. Vì nhánh low-ready riêng, không thể giả định armor luôn ảnh hưởng tốc độ như nhau ở mọi tư thế.

| Yếu tố | Cách hoạt động trong snapshot |
|---|---|
| `TempCurrentRunSpeedPercent` | 0,4 khi `bHoldingFastWalk`, không thì 1; rồi nhân 0,75 nếu không sprint. Cấu hình này cho default 0,75 và slow walk 0,3 |
| `SprintRamp` | Đường ngủ: bắt đầu 1,2, tới 1,3 ở 5/s, khoảng 0,02 s; ở build khảo sát reset về 1 mỗi tick |
| `HitSpeedMultiplier` | Clamp `[0.3,1]`, hồi về 1 ở 0,5/s; từ sàn cần 1,4 s. Đường làm giảm nó nằm ngoài block này |
| `BleedMultiplier` | `clamp(1 − 0.1 × BleedTime, 0.25, 1)`; bleed còn kích Injury post-process và shake, cooldown 3,75 s |
| `PVPSpeedMultiplier` | 1,3 nếu GameState là `ADefusalGS` |
| Gia tốc | `MaxAcceleration = 500 × armour GetArmourAccelerationMultiplier()`; hệ số armor khác hệ số tốc độ |

Input hướng được tính riêng trước `AddMovementInput`:

`InputScale = clamp(StrafeScale − LegLoss − CarryLoss, 0.25, 1.0)`.

`StrafeScale` là 1 khi tiến, 0,85 khi lùi/ngang; lean nhân thêm 0,85. `LegLoss = (max leg tickets − min remaining leg tickets) × 0.10`; comment source nói 5% nhưng default được TDD ghi là 10%. `CarryLoss = 0.10` khi mang. Đây là phép **trừ** tổn thất khỏi input scale, không nhân tất cả hệ số. Input bị bỏ hẳn nếu `bMovementLocked`, `bFreeLeaning` hoặc bị bắt; từng move còn tạo shake theo velocity.

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:1580-1627,3979-4025,6532-6600`; `Characters/PlayerCharacter.h:960,1706-1721,1841-1865`. Đơn vị chuyển động trong các trace/bob giữ theo UE unit mà PDF ghi; nơi nguồn ghi `cm` hoặc `s` thì giữ nguyên.

## 3.4. Low-ready: hình học, khoảng đệm và animation gate — PDF §3, tr. 17–19

`DoLowReadyTrace` chỉ chạy khi có item và bỏ qua nếu tablet đang focus. Sphere trace bắt đầu lệch phải camera 5 unit, xét WorldStatic, WorldDynamic, Door, Pawn, Destructible, PhysicsBody; bỏ blocking/trigger/audio volume của level. Bán kính 9, tăng thành 11 khi đã low-ready: hai ngưỡng khác nhau tạo hysteresis để giảm bật/tắt liên tục sát biên.

Chiều dài là `LowReadyTraceDistance (10000) + (1 − LastNonADSRunSpeedPercent) × −30`; chậm hơn chỉ rút tối đa 30 unit, nhỏ so với 10.000. Khi chạm character, khoảng cách dùng để quyết định được nhân 1,75; đối tượng arrested/surrendered nhân 5. Với nhóm sau, hit chỉ được giữ khi line–sphere với sphere 50 unit ở `LowReadyRange` xác nhận, **hoặc** impact thấp hơn Z mục tiêu cộng 35; nếu cả hai điều kiện đều không thỏa thì bỏ hit. Hit AI deactivated kết thúc toàn bộ update. Các điều kiện này quan trọng hơn một quy tắc đơn giản “mũi súng chạm bất cứ người nào thì hạ”.

Kết quả so với `GetLowReadyRange()` của item; pitch camera và `LowReadyPitchThreshold` chọn point-up/point-down. Khi `bUserLowReady` bật, setting `LoadLowReadyStyle` quyết định high-ready/low-ready. `SetLowReady` ép yêu cầu true nếu `bUserLowReady || bForceLowReady`, **nhưng vẫn không vượt qua animation-blocking**. Chỉ khi trạng thái point-up/down đổi mới gửi `Server_SetLowReady`.

Một async line multi-trace khác, `DoLowReadyVolumeTrace`, cập nhật `bIsLowReadyFromVolume` khi hit đầu phù hợp dọc view ray là `AForceLowReadyVolume`. Volume có `bForceLowReadyWhileAimingAt` và `bForceLowReadyWhileInside`, cả hai default true. Súng còn lùi trước khi hạ: khi `LowReadyDistance < LowReadyRange + 30`, meshspace có offset `BackMeshspace × BackRatio × 4`.

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:4261-4405,4473-4492,4917-4966,7883-7890`; `Characters/PlayerCharacter.h:1083`; `Actors/Environment/ForceLowReadyVolume.h:16-20`.

## 3.5. Use và Melee là thương lượng giữa player với object — PDF §3, tr. 17

Player hỏi “tôi có thể hành động lúc này không?” qua `CanUse`; object hỏi “người này có thể dùng tôi lúc này không?” qua `CanInteract`. Hai gate bổ sung nhau. Nếu chỉ kiểm tra khoảng cách ở object, người đang reload hoặc được mang vẫn có thể vô tình gửi hành động không hợp lệ.

`CanUse` từ chối khi paired interaction, actions-lock, tablet focus, animation-blocking, dead/unconscious/arrested/ragdolled, devices menu, reload, đang mang hoặc được mang. `Use()` khi ADS chuyển thành yell. `ShouldExecuteUseOnKeyDown()` yêu cầu `CanUse() && !BleedComponent->CanHeal()`; nếu có thể heal, Use chờ key-up, còn giữ quá 0,25 s gọi `Server_PrepareForHeal`. Một phím có nhiều ý nghĩa theo trạng thái, nên thời điểm nhấn/thả là phần của hợp đồng input.

`Execute_Use` dùng focused component nếu `CanInteract` đạt và có slot `"Use"`; nếu không thì yell, trừ khi lời gọi đến từ `UseOnly`. `bExecuteOnServer` và `bClientInteract` chọn RPC hay thực thi local. Tên slot là một giao thức nhỏ giữa character và object.

`UInteractableComponent::CanInteract` kiểm tra theo thứ tự: owner/world/use-actor có mặt; `bEnabled`; look-dot cache đạt `RequiredLookAtPercentage` (default 0,9) nếu yêu cầu nhìn; khoảng cách trong `[MinShowPromptAtDistance = 0, ShowPromptAtDistance = 160]` nếu bật kiểm tra; overlap tùy chọn; ít nhất một slot hợp lệ; line of sight; `bDisallowedItemEquipped`; cuối cùng `bIsFocusedComponent`.

Melee bị chặn khi mang/được mang hoặc animation-blocking. Nó ưu tiên slot `"Melee"` của focused interactable, cho phép cửa nhận cú kick. Fallback trace dài 300 unit trên `ECC_WorldDynamic`; hit cửa mở cửa sổ double-tap 0,2 s qua `bWaitingForDoubleTapMelee`, thay vì vung ngay.

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:997-1007,4035-4188,4836-4879`; `Components/InteractableComponent.h:204-213`, `.cpp:271-384`.

## 3.6. ADS, free-look, lean và presentation — PDF §3, tr. 17–19

`DoAimDownSights` từ chối nếu actions-lock; xóa `bUserLowReady`; lời gọi `Walk()` liên quan sprint bị compile out theo macro. Toggle bị từ chối khi animation-blocking, tab menu hoặc thu evidence; tablet và grenade không aim. Client gửi `Server_UpdateADS`; `ToggleADS` thêm latch `bToggled` trên `SecondaryUse/EndSecondaryUse`.

`ApplyFoV` dùng `PlayerCameraFOVMultiplier` của scope, hoặc 0,75 nếu bật zoom ADS, nội suy với `GetADSZoomInSpeed()`. FOV trở lại default ở tốc độ 10. Nhánh sprint nhân 1,1 và tốc độ nội suy 8 không chạy vì `IsSprinting()` false. Không nên dùng `SpeedModifier_Aim = 0.5` để suy ra ADS làm giảm tốc độ: TDD thấy nó chỉ dùng trong `IncrementalUse` đã bị vô hiệu; `SpeedModifier_AimFocus = 0.25` chỉ khai báo, không dùng trong `.cpp`.

Meshspace lấy transform Default/Aiming/Back từ item, kết hợp inertia khi aim/strafe. Bob chỉ chạy local: phase sin/cos tăng khi có input, tỷ lệ theo velocity so với max speed hiện tại; walk scale ở ≤120 u/s, sprint scale ở ≥350 u/s, không bob dưới 30 u/s. “Injured” là health dưới 50. Item quyết định crouch/ADS modifier; `FInterpTo` dùng 14 tới target và 12 về nghỉ. Nhãn “sprint scale” trong dữ liệu bob không chứng minh sprint hoạt động.

Free-look clamp pitch `[-20°,45°]`, yaw `[-45°,45°]`, rồi giữ pitch tổng `[-72°,80°]`. Khi free-lean, yaw nhân `1 − clamp(|LeanYaw|/100, 0, 0.75)` và dừng ở ±70°. `AddYaw` luôn nhân `SprintMaxTurnRateLeft/Right`, nhưng nhánh giảm hệ số chỉ chạy khi sprint; phục hồi 1/s giữ chúng ở 1 trong cấu hình này. Gamepad look giảm còn một nửa khi aim. `CanYell()` từ chối nếu pitch dưới −40° và có WorldStatic trong 300 unit theo hướng nhìn, hoặc đang low-ready do tường.

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:3531-3560,3588-3610,3721-3763,5983-6053,6611-6706,7843-7890,10317-10454`; `Characters/PlayerCharacter.h:67-77,1536-1544,1845-1849`. Tuning per-item gồm `CameraBobScaleH/V`, `CameraBobSpeedScaleH/V`, họ `WeaponBob*`, `InertiaDrag*`, `bDisableMeshspaceMovement`, low-ready và ADS; yell còn có `YellOutEffectLength = 2.0`, `YelloutEffectRadius = 3500` (`PlayerCharacter.h:853-856`).

## 3.7. Mạng: dự đoán intent và quyền quyết định tốc độ — PDF §3, tr. 19–20

Khoảng 36 property được đăng ký replicate. Nhóm tới mọi client gồm các giá trị tốc độ, `SlowDownSpeedMultiplier`, `bWalking`, `bHoldingFastWalk`, `bDisableSprinting`, `bStunAimLocked`, trạng thái third-person/stopping, `CurrentMeshRot`, `Server_BaseAimRotation`, focus tương tác và revive. `COND_OwnerOnly` gồm `HitSpeedMultiplier`, `WalkSpeedRampMultiplier`, `SprintSpeedRampUpMultiplier`, `bForceLowReady`, `bServerIsBlockingAnimationPlaying`. `COND_SkipOwner` gồm `FreeLookCache`, `bFireLoop`.

Gait handler áp dụng local rồi gửi reliable RPC với `Validate true`; `FastWalk/EndFastWalk` gọi `_Implementation` local trước khi gửi. Riêng `SetWalkSpeed` gửi cả `Server_SetWalkSpeed` và `Client_SetWalkSpeed`; hai bên gán trực tiếp `MaxWalkSpeed`. RPC server là Unreliable, validate luôn true; timer handle debounce được khai báo nhưng không dùng. Free-look cache và camera rotation rate cũng dùng Unreliable. Authority cập nhật base aim và đẩy AI đang overlap mỗi 0,05 s; correction vị trí dựa CMC chuẩn.

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:294-358,972-988,3804-3820,7316-7326`; `Characters/PlayerCharacter.h:842,912,1908-1920,1943-1975`.

TDD cảnh báo server tin tốc độ client gửi. Tuy nhiên server tick cũng ghi lại tốc độ mỗi frame, nên cửa sổ can thiệp nằm giữa RPC và tick tiếp theo; kích thước ảnh hưởng thực tế **chưa đo, là suy luận**. Bài học tái tạo là client gửi intent, server tự tính tốc độ; không lấy giá trị tốc độ kết quả từ client làm sự thật.

## 3.8. Điểm mở rộng và lỗi cần giữ đúng phạm vi — PDF §3, tr. 19–21

Item có thể consume input qua `ConsumeMovementForward/Right`, `ConsumeMouseMovement`, `ConsumeSprintInput`, `ConsumeCrouchInput`, `ConsumeJumpInput`, `ConsumeIncrementalUse`. Công cụ mới thêm `BindAction` và wrapper `Equip*` gọi `EquipItemOfType(EItemCategory::…)` khi menu SWAT command không mở. Interactable mới chỉ cần component, `ActionSlot1..4` và tuning gate; không phải sửa player cho từng cửa. Feel vũ khí lấy từ item; aim-assist mở rộng bằng asset và `GetAimAssistConfigByIntensity`.

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:421-430,5743-5862,6541,6578,6652,7241,7384`; `Characters/ReadyOrNotPlayerController.h:1294`.

| Điểm nguồn ghi nhận | Hậu quả và giới hạn |
|---|---|
| `ToggleWalk` chỉ đổi `bHoldingFastWalk` local | Không gọi `Server_ToggleWalk`; client tính 0,3 trong khi server tính 0,75. Correction/rubber-banding là kết quả dự kiến, chưa phải đo runtime |
| `ToggleSprint` có thể khóa slow walk | `bWalking` default true, code đổi nó bị compile out; toggle tiếp tục gọi `Sprint → FastWalk`. Thả Walk là lối thoát được suy ra; caller Blueprint chưa kiểm tra |
| Stun phạt hai lần | Nhánh bình thường `GetRunSpeed` nhân 0,75, tick thêm 0,75 khi replicated stun; thành 0,5625. PDF không xác nhận đây là ngoài ý đồ |
| Volume Inside-only không hoạt động độc lập | Kiểm tra vẫn rơi xuống `WhileAimingAt`; default cả hai true che vấn đề |
| Slow-down volume không cộng dồn | Ra khỏi bất kỳ volume nào reset 1,0 dù còn trong volume khác; cả delegate actor và component đều bind nên enter/exit chạy hai lần |
| Sprint/jump/vault ngủ | `DoVaultTraces` còn bị comment; `bLedgeFound` không được đặt. Nếu bật jump lại, `PlayerJump` không làm gì khi không equip item |
| `ApplyCameraLocation` rỗng | Vẫn được gọi mỗi tick |
| `SetRunSpeed` clamp rồi ghi đè | `CurrentRunSpeedPercent` sau clamp bị gán lại `newRunSpeed` chưa clamp |

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:7257-7260,7379-7397,3997-4007,1621,4948-4952,6928-7061,3765-3768,1631,3795-3802`; `Characters/PlayerCharacter.h:1948-1951,2006`; `ReadyOrNotCharacter.cpp:1223`; `Actors/Environment/SlowDownVolume.cpp:14-21,25-60`. `SlowDownMultiplier` default 1,0 (`SlowDownVolume.h:36`). Các speed-dial bound `MinWalkSpeedPercent = 0.15`, `MaxCrouchRunSpeedPercent = 0.5` cũng thuộc đường đã vô hiệu.

PDF đã loại hai nhận xét khỏi bản nháp: “fast-walk cộng sprint chậm hơn walk” không tới được vì sprint bị compile out; “sprint turn-rate không replicate” là local state có chủ đích, không được tính là defect. Gỡ macro không tự hoàn thiện tính năng: các nhánh ngủ và lỗi của chúng sẽ trở lại.

## Bài thực hành tái tạo — hành lang có cửa và hai vùng chậm

**Bài tập đề xuất:** tạo movement default/slow/crouch, một item có low-ready, một cửa interactable và hai volume chồng nhau.

| Phần hợp đồng | Yêu cầu |
|---|---|
| Owner | Server giữ intent hợp lệ và tính tốc độ; owning client dự đoán cùng công thức; camera/meshspace là presentation local |
| State | Intent slow/crouch/ADS; action-lock; focus; bộ modifier tốc độ theo nguồn; low-ready có hai ngưỡng |
| Dependencies | CMC, item feel data, trace collision, interactable gate, animation gate và RPC intent |
| Acceptance | Toggle hai lần trả về bình thường trên server/client; ra một volume vẫn giữ effect volume còn lại; animation-blocking từ chối Use; focus sai góc/LOS từ chối; súng không rung trạng thái ở biên low-ready |

Log từng thừa số tốc độ và lý do từ chối input. Thử cùng thao tác ở host, owning client và người quan sát. Dùng một transform sway đơn giản trước khi thêm toàn bộ bob/inertia; mục tiêu là giải thích được chuyển trạng thái và authority, không phải sao chép số lượng biến của snapshot.

Đọc cùng [player, health và inventory](../02-Gameplay-Va-AI/05-Player-Movement-Health-Inventory.md), [tương tác và bắt giữ](../02-Gameplay-Va-AI/03-Tuong-Tac-Bat-Giu-Vat-Chung.md), [input, inventory, equip](../03-Gun-Gameplay/02-Input-inventory-equip.md), [recoil, ADS, camera](../03-Gun-Gameplay/06-Recoil-ADS-camera.md) và [state ownership](../04-Kien-Truc-Va-Quy-Trinh/03-Multiplayer-State-Ownership.md). Tiếp theo là [vũ khí, ballistics và hit registration](04-weapons-ballistics-hitreg.md).

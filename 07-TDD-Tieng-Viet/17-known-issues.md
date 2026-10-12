# 17. Danh mục 118 vấn đề do TDD nêu

> **Ranh giới bằng chứng:** đây là bản dịch/diễn giải danh mục của TDD R&D reverse-engineered độc lập ngày **27/09/2026**, §17, tr.104–113. Các dòng là **nhận định của tác giả PDF**, không phải lỗi vừa được bộ Docs này tái hiện hoặc kiểm chứng lại. Không phải tài liệu chính thức VOID và không đại diện mặc nhiên cho bản retail mới nhất. Giữ nguyên nhãn severity, mục nguồn và locator để người học quay lại ngữ cảnh.

## Cách đọc và kết quả đối chiếu số lượng

PDF mở đầu bằng tuyên bố các lỗi đã qua bước kiểm tra code của tác giả; đồng thời nhiều mô tả cụ thể vẫn mang điều kiện “suy luận”, “có thể”, “phụ thuộc dữ liệu”, “chưa chạy” hoặc “Blueprint chưa xác minh”. Hai lớp thông tin này phải được đọc cùng nhau. Từ **High** trong PDF được định nghĩa là lỗi người chơi nhìn thấy, phơi lộ bảo mật hoặc tính năng âm thầm không hoạt động. Medium/Low không có thang điểm định lượng chi tiết; không tự nâng/hạ nhãn bằng suy đoán.

Đối chiếu trực tiếp các hàng bắt đầu bằng high/medium/low trong văn bản tr.104–112 cho kết quả **118 = 12 High + 38 Medium + 68 Low**, khớp số tổng PDF công bố. Không phát hiện bất nhất về tổng severity. Việc cùng một nhóm rủi ro có mức độ khác nhau là phân loại của tác giả, không phải xác nhận tác động đã đo. Tr.113 không thêm hàng, chỉ ghi chú hướng tái sử dụng.

| Trang PDF | High | Medium | Low | Tổng |
|---|---:|---:|---:|---:|
| 104 | 12 | 1 | 0 | 13 |
| 105 | 0 | 14 | 0 | 14 |
| 106 | 0 | 12 | 0 | 12 |
| 107 | 0 | 11 | 5 | 16 |
| 108 | 0 | 0 | 15 | 15 |
| 109 | 0 | 0 | 15 | 15 |
| 110 | 0 | 0 | 14 | 14 |
| 111 | 0 | 0 | 15 | 15 |
| 112 | 0 | 0 | 4 | 4 |
| **Tổng** | **12** | **38** | **68** | **118** |

Các locator giữ cách ghi của TDD. Đường dẫn không có prefix thường tương đối với `Source/ReadyOrNot/`; `Plugins/...` và locator rút gọn `...` được giữ theo nguồn, không giả vờ là đường dẫn đã resolve. Số dòng thuộc snapshot TDD, không bảo đảm đúng ở checkout khác. Không công bố giá trị API key hay tài khoản mà PDF đã giấu.

## High — 12 hàng

| ID | Severity | Mục nguồn | Nhận định trong PDF, diễn giải tiếng Việt | Locator do TDD cung cấp | Trang |
|---|---|---|---|---|---:|
| 001 | High | §11 Meta | `GetSquadTraitValue` bỏ qua officer có trait đã mở khóa; phép kiểm tra có vẻ đảo ngược, ảnh hưởng SBAGS, MoralOfficer và Paramedic. | `Source/ReadyOrNot/Commander/RosterManager.cpp:461` | 104 |
| 002 | High | §15 Network | RPC biểu quyết không spawn/gán `AReferendum`; `CurrentReferendum` luôn null trong tuyến được đọc, nên map/player referendum và vote RPC không làm gì. | `Characters/ReadyOrNotPlayerController.cpp:5614-5712` | 104 |
| 003 | High | §15 Network | Host migration bị compile out; debug path cũng hỏng: không gọi `SetNextHost`, vòng tìm không dừng khi join và kết thúc ở main menu. | `Info/HostMigrationManager.cpp:174-186,329-356`; `ReadyOrNotGameInstance.cpp:2500-2520` | 104 |
| 004 | High | §16 Tooling | `UReadyOrNotDebugSubsystem` không có Shipping guard, nên hàm cheat được compile vào Shipping. Không đồng nghĩa frontend DevMenu cũng có trong Shipping. | `Source/ReadyOrNot/ReadyOrNotDebugSubsystem.h:12`; `.cpp:31-35` | 104 |
| 005 | High | §2 Architecture | TDD cho biết API key production của mod.io được commit trong header; danh mục này không chép giá trị key. | `ReadyOrNot.h:141-152` | 104 |
| 006 | High | §4 Weapons | `bFirstShot` không bị xóa, khiến multiplier recoil/spread của phát đầu áp cho mọi phát. | `Actors/BaseWeapon.cpp:1021-1052`; `Actors/BaseWeapon.h:626` | 104 |
| 007 | High | §7 Doors | Chia số nguyên làm offset kick khi chân bị thương thành 1, khiến phép roll luôn thất bại theo phân tích của TDD. | `Actors/Door.cpp:6561-6563` | 104 |
| 008 | High | §9 AI | Khi chỉ có một cover candidate, code bỏ qua mọi phép kiểm tra candidate. | `Info/Activities/Tasks/FindCoverTask.cpp:684-691` | 104 |
| 009 | High | §9 AI | CS gas manager giữ null nếu không ở ListenServer. | `Info/CSGasManager.cpp:44` | 104 |
| 010 | High | §9 AI | Cooldown di chuyển qua lỗ tường chỉ tick trong editor. | `Actors/WallHoleTraversal.cpp:18` | 104 |
| 011 | High | §9 AI | Trace exposure chiều ngược dùng lại mảng handle của chiều xuôi. | `Components/TargetingComponent.cpp:1170` | 104 |
| 012 | High | §9 AI | Activity PlaceTrap bị hard-disable nhưng vẫn được giao cho AI. | `Info/Activities/PlaceTrapActivity.cpp:24` | 104 |

## Medium — 38 hàng

| ID | Severity | Mục nguồn | Nhận định trong PDF, diễn giải tiếng Việt | Locator do TDD cung cấp | Trang |
|---|---|---|---|---|---:|
| 013 | Medium | §10 Rules | `CheckWinConditions` dừng ở primary objective đang chạy đầu tiên; objective phía sau có `bFailureEndsMission` thất bại không kết thúc nhiệm vụ, phụ thuộc thứ tự. | `GameModes/CoopGM.cpp:691-704,733` | 104 |
| 014 | Medium | §10 Rules | `CacheHasClearedMission` dừng ở nhóm chưa đạt đầu tiên; soft-clear giữ giá trị cũ và nhóm sau không được kiểm tra. | `Info/ScoringManager.cpp:1262-1274` | 105 |
| 015 | Medium | §10 Rules | Tuyến AI tự clear đi qua `ExfiltrateMission`, dẫn tới dòng thất bại và multicast kết thúc thất bại. | `Info/SWATManager.cpp:1836-1854`; `GameModes/CoopGM.cpp:2073-2078` | 105 |
| 016 | Medium | §11 Meta | `IsTherapyFull` trả true khi giới hạn dưới 0; comment ini nói -1 là vô hạn, nhưng đặt -1 lại vô hiệu hóa therapy. | `Source/ReadyOrNot/Commander/RosterManager.cpp:413-419` | 105 |
| 017 | Medium | §11 Meta | Switch thiếu case `ELevelGrade::E`; yêu cầu grade E không mở khóa dù ScoringManager phát `_grade_e`. | `Source/ReadyOrNot/Data/ProgressionData.cpp:13-23` | 105 |
| 018 | Medium | §11 Meta | Vòng `while(true)` trong `SelectRandomMission` treo nếu không có mode nào chọn được. | `Source/ReadyOrNot/Actors/Environment/MissionPortal.cpp:621-652` | 105 |
| 019 | Medium | §14 Presentation | Guard rỗng của `StartInjuryRadialBlurEffect` cho phép truy cập `PostProcesses[INDEX_NONE]`; khả năng kích hoạt phụ thuộc dữ liệu effect. | `Source/ReadyOrNot/Components/PlayerPostProcessing.cpp:1249-1250` | 105 |
| 020 | Medium | §14 Presentation | Blend AMRagdoll, trigger góc sàn/trên không và settle-freeze đều comment out; tunable tương ứng không tạo hành vi. | `Plugins/AMRagdoll/Source/AMRagdoll/Components/RagdollComponent.cpp:169-179` | 105 |
| 021 | Medium | §15 Network | Checksum PAK do client tự báo, Validate luôn true; dedicated server không thực thi phép kick checksum. | `Characters/ReadyOrNotPlayerController.cpp:457-475,689-697` | 105 |
| 022 | Medium | §15 Network | Vivox compile out trên PC/Steam và Xbox; `WITH_VIVOX` chỉ có trong private definitions của PlayStation theo checkout. | `ReadyOrNot.Build.cs:104,127,194,293` | 105 |
| 023 | Medium | §15 Network | Tick LOD AI chỉ dùng viewpoint local; có thể giảm tick AI gần remote client, dedicated dùng transform identity. Tác động là suy luận của TDD. | `Info/ReadyOrNotSignificanceManager.cpp:100-120,309-339` | 105 |
| 024 | Medium | §16 Tooling | Stack-up test báo thành công bằng `AddError`, nên harness vẫn nhìn thấy thất bại. | `Plugins/ReadyOrNotTests/Source/ReadyOrNotTests/Private/SwatStackUpTest.cpp:420,429-431` | 105 |
| 025 | Medium | §16 Tooling | Writer spin-test ghi `ProjectDir/../../../../PerfTests`, reader đọc `ProjectDir/PerfTests`, nên capture mới không tự hiện ở reader. | `Source/ReadyOrNot/Testing/ReadyOrNotSpinTestController.cpp:55`; `SpinTestHeatmapVolume.cpp:17` | 105 |
| 026 | Medium | §16 Tooling | `ConsoleApplyLevelSpecificSettings` dereference player controller không kiểm tra null. | `Source/ReadyOrNot/lib/ReadyOrNotConsoleFunctionLibrary.cpp:105-106` | 105 |
| 027 | Medium | §16 Tooling | Tên tài khoản Steam build hard-code trong `Build.ps1`; giá trị bị TDD giấu và không được tái bản ở đây. | `G:/Ue5_Prototype/BuildTools/Build.ps1:387` | 105 |
| 028 | Medium | §2 Architecture | Registry `AllAICharacters/AllReadyOrNotCharacters` chỉ được điền từ `PossessedBy` phía server và không replicate. | `Characters/CyberneticCharacter.cpp:1190`; `ReadyOrNotCharacter.cpp:1755`; `ReadyOrNotGameState.h:364-367` | 106 |
| 029 | Medium | §2 Architecture | `PRAGMA_DISABLE_OPTIMIZATION` còn trong code Shipping của CommandFunctionLibrary, Loadout_V2, console HUD và InventoryComponent. | `lib/ReadyOrNotCommandFunctionLibrary.cpp:26-3921`; `HUD/Widgets/Loadout/V2/Loadout_V2.cpp:18-1787` | 106 |
| 030 | Medium | §2 Architecture | Private PCH nguyên khối có 49 include, gồm các gameplay header cụ thể, tạo coupling lớn. | `ReadyOrNot.Build.cs:20-21`; `ReadyOrNot.h:12-74` | 106 |
| 031 | Medium | §3 Player | `Server_SetWalkSpeed` tin `MaxWalkSpeed` do client gửi; Validate luôn true. | `Characters/PlayerCharacter.cpp:3804-3814`; `PlayerCharacter.h:1908-1911` | 106 |
| 032 | Medium | §3 Player | `ToggleWalk` chỉ lật local `bHoldingFastWalk` có replicate; `Server_ToggleWalk` không được gọi, khiến tốc độ client/server lệch nhau. | `Characters/PlayerCharacter.cpp:7257-7260`; `PlayerCharacter.h:1948-1951` | 106 |
| 033 | Medium | §4 Weapons | Ricochet projectile dùng `RandRange(0,1)` số nguyên, cho 50% với mọi `PercentageToDeflect` trong `(0,1]`; giá trị 0 không cho nảy. Điều kiện biên lấy từ §4, tr.26 để làm rõ dòng tóm tắt §17. | `Actors/Projectiles/DamageProjectiles/BulletProjectile.cpp:364` | 106 |
| 034 | Medium | §4 Weapons | Projectile sinh lại sau xuyên/nảy bỏ qua `NewDamage`, giữ damage đầy đủ. | `Actors/Projectiles/DamageProjectiles/BulletProjectile.cpp:139,158` | 106 |
| 035 | Medium | §4 Weapons | Hitscan spalling bị hard-disable bằng `bShouldSpall=false` sau nâng UE5.3. | `Actors/BaseMagazineWeapon.cpp:2117-2122` | 106 |
| 036 | Medium | §5 Damage | Damage less-lethal/stun vẫn giảm Health vì `OnTakeDamage` trả true; gate `bCauseHealthDamage` không còn tác dụng trong đường được phân tích. | `Characters/PlayerCharacter.cpp:2816-2828,2731`; `Characters/CyberneticCharacter.cpp:3017`; `ReadyOrNotCharacter.cpp:6249-6253` | 106 |
| 037 | Medium | §5 Damage | Một handle `TH_EndStunTimer` bị ghi đè mỗi stun; `EndStun` xóa cả StunMap, nên không theo dõi đúng các stun đồng thời. | `ReadyOrNotCharacter.cpp:7484-7495`; `Characters/CyberneticCharacter.cpp:4475` | 106 |
| 038 | Medium | §5 Damage | Heal phía client dùng RPC SetResource/ResetResource chỉ kiểm tra MaxResourceLimit; TDD nêu client sửa đổi có thể đặt Health. | `Components/BleedComponent.cpp:153-174`; `Components/ResourceComponent.h:258` | 106 |
| 039 | Medium | §6 Items | `Server_ChangeEquippedItem` không xác thực `ToItem` có trong inventory. | `Source/ReadyOrNot/Components/InventoryComponent.cpp:906-923` | 106 |
| 040 | Medium | §6 Items | Add/RemoveInventoryItem thiếu authority guard; Remove không multicast. | `Source/ReadyOrNot/Components/InventoryComponent.cpp:1294-1370` | 107 |
| 041 | Medium | §6 Items | Radial stun damage của C2 được áp bên trong multicast trên mọi máy. | `Source/ReadyOrNot/Actors/Gameplay/PlacedC2Explosive.cpp:154-172` | 107 |
| 042 | Medium | §6 Items | Đường đi/vị trí grenade do client quyết định, validation luôn true. | `Source/ReadyOrNot/Actors/BaseGrenade.cpp:1010-1032` | 107 |
| 043 | Medium | §6 Items | C++ không đặt `TS_Activated`; trap có thể kích lại mỗi tick nếu Blueprint không đổi state; `OnTrapTriggered` chạy hai lần trên server. | `Source/ReadyOrNot/Actors/Gameplay/TrapActor.cpp:280-315,606-615,718,760` | 107 |
| 044 | Medium | §7 Doors | Điều kiện bỏ collision của `DestroyAllChunks bKeepHinges` yêu cầu A==B, A==C, A==D nên không thể true với các giá trị phân biệt được xét. | `Actors/Door.cpp:4991` | 107 |
| 045 | Medium | §8 Command UX | Logic dựng trang và dispatch bị lặp theo input device giữa widget và function library. | `HUD/Widgets/SwatCommandWidget.cpp:275-535`; `lib/ReadyOrNotCommandFunctionLibrary.cpp:510-617` | 107 |
| 046 | Medium | §8 Command UX | Phân loại default-tap khác default hiển thị; có chuỗi Cast thứ hai và ternary cửa bị lặp. | `HUD/Widgets/SwatCommandWidget.cpp:3693-3757` đối chiếu `:1989-2157`; `:2060` đối chiếu `:2234` | 107 |
| 047 | Medium | §9 AI | FSM activity áp nhiều transition trong cùng tick. | `Info/Activities/ActivityFiniteStateMachine.cpp:41-50` | 107 |
| 048 | Medium | §9 AI | Vòng chọn interrupt dừng ngay sau match đầu tiên. | `Characters/CyberneticController.cpp:3339` | 107 |
| 049 | Medium | §9 AI | Cover subsystem shutdown khi bất kỳ world nào cleanup, rồi tạo lại octree rỗng. | `Plugins/DynamicCoverSystem/.../CoverSystem.cpp:26-30,71-74` | 107 |
| 050 | Medium | §9 AI | Path following quét mọi cửa mỗi tick, chi phí theo `O(số cửa × số AI)`. | `Components/ReadyOrNotPathFollowingComp.cpp:212-221` | 107 |

## Low — 68 hàng

| ID | Severity | Mục nguồn | Nhận định trong PDF, diễn giải tiếng Việt | Locator do TDD cung cấp | Trang |
|---|---|---|---|---|---:|
| 051 | Low | §10 Rules | Hai bản logic sàn grade không khớp: F hoặc E khi điểm 0 nhưng hoàn thành toàn bộ primary. | `Info/ScoringManager.cpp:1332-1338,1401-1403` | 107 |
| 052 | Low | §10 Rules | Chia TotalScorePool không guard 0; NaN được clamp thành 1,0, TDD **suy luận** có thể ra grade S. | `Info/ScoringManager.cpp:1387,1399` | 107 |
| 053 | Low | §10 Rules | `StartMissionEndTimer` bỏ qua verdict tới sau; `Multicast_OnMissionEnd` phát hai lần. | `GameModes/CoopGM.cpp:787,796,1725` | 107 |
| 054 | Low | §10 Rules | Không có reader C++ cho `MaxHostagesKilledBeforeMissionFailed` và `MaxTeamKillsBeforeAIRetaliates`. Không tự suy ra Blueprint cũng không đọc. | `GameModes/CoopGM.h:186,189` | 107 |
| 055 | Low | §10 Rules | Score ledger replicate nguyên mảng. | `Info/ScoringManager.cpp:123-131` | 107 |
| 056 | Low | §10 Rules | Phạt exfil 500 hard-code, bỏ qua DataTable. | `Actors/Environment/ExfilPortal.cpp:225` | 108 |
| 057 | Low | §11 Meta | TOC High-priority kiểm tra `Last()` thay đầu queue khi xét Flush, nên có thể vượt một Flush đang chờ. | `Source/ReadyOrNot/Info/TOCManager.cpp:183` | 108 |
| 058 | Low | §11 Meta | Danh sách cache và query live của therapist widget có thể khác nhau, trả placeholder “Hello world”; stress trung bình chia squad count không guard 0. | `Source/ReadyOrNot/Commander/TherapistWidget.cpp:43-93` | 108 |
| 059 | Low | §11 Meta | Comment TickTimers nói chỉ khi thành công nhưng `OnPlayerKilled` cũng gọi. | `Source/ReadyOrNot/Commander/RosterManager.cpp:490` | 108 |
| 060 | Low | §11 Meta | Officer bị sa thải không thêm vào PreviousCharacters, không có record lưu và không làm hỏng `ironman_flawless`. | `Source/ReadyOrNot/Commander/RosterManager.cpp:335-359` | 108 |
| 061 | Low | §11 Meta | Counter sa thải reset trong CompleteMission, trái comment. | `Source/ReadyOrNot/Commander/RosterManager.cpp:172-173` | 108 |
| 062 | Low | §11 Meta | Comment TOC Low-priority không khớp code; replication QueuedTOCData bị tắt không ghi lý do. | `Source/ReadyOrNot/Info/TOCManager.cpp:57-63,165-172` | 108 |
| 063 | Low | §11 Meta | Comment TherapistTimeScale nói 4 nhiệm vụ nhưng giá trị là 2,0; key MaximumStartingAccuracy/Reflexes trong ini không có property C++ tương ứng. | `Config/DefaultCommander.ini` | 108 |
| 064 | Low | §14 Presentation | Camera manager thêm lại DefaultModifiers sau Super, tạo hai shake modifier theo đối chiếu engine của TDD. | `Source/ReadyOrNot/ReadyOrNotPlayerCameraManager.cpp:26-48` | 108 |
| 065 | Low | §14 Presentation | Vòng slot null thiếu break, ghi một shake vào mọi slot null; TDD ghi lỗi này kế thừa từ engine 4.27. | `Source/ReadyOrNot/Info/RONCameraModifier_CameraShake.cpp:68-80` | 108 |
| 066 | Low | §14 Presentation | Bỏ qua OnRegister bằng Super::Super rồi ghi settings trực tiếp vào camera: mong manh, không dùng volume blending. | `Source/ReadyOrNot/Components/PlayerPostProcessing.cpp:1669-1677` | 108 |
| 067 | Low | §14 Presentation | Lỗi death fade trên client được workaround bằng kiểm tra lại mỗi tick. | `Source/ReadyOrNot/Components/PlayerPostProcessing.cpp:105-118` | 108 |
| 068 | Low | §14 Presentation | `OnDamageTaken` được bind nhưng body rỗng. | `Source/ReadyOrNot/Components/PlayerPostProcessing.cpp:1548-1553` | 108 |
| 069 | Low | §14 Presentation | Tick path của SkinComponent không chạy mặc định vì `bCanEverTick=false`; Blueprint child có thể đổi setting này. | `Source/ReadyOrNot/Components/SkinComponent.cpp:9,166-171` | 108 |
| 070 | Low | §14 Presentation | Cooldown phụ đề dùng wall-clock, bỏ qua pause và time dilation. | `Source/ReadyOrNot/Subsystems/SubtitlesSubsystem.cpp:239-242` | 108 |
| 071 | Low | §15 Network | Tích hợp mod.io compile out do `WITH_MODIO` không được định nghĩa ở cấu hình đã kiểm tra. | `ReadyOrNot.Build.cs:103,245`; `Info/ModioManager.cpp:7-430` | 109 |
| 072 | Low | §15 Network | Không có native caller cho multicast thông báo Yes/No/VoteStarted. | `Actors/Voting/Referendum.cpp:157-185` | 109 |
| 073 | Low | §15 Network | StartHostMigration/StartMigration dereference manager/controller không null-check; mục §15 lưu ý manager hiện được Init tạo trước. | `ReadyOrNotGameInstance.cpp:2564`; `Info/HostMigrationManager.cpp:224-225` | 109 |
| 074 | Low | §15 Network | SteamParty chỉ có một header, thiếu build file/plugin hoàn chỉnh. | `Plugins/SteamParty/Source/SteamParty/Public/SteamBeaconTypes.h` | 109 |
| 075 | Low | §16 Tooling | Heatmap reader so sánh FVector chưa khởi tạo trước khi InitFromString. | `Source/ReadyOrNot/Testing/SpinTestHeatmapVolume.cpp:30-38` | 109 |
| 076 | Low | §16 Tooling | Bool RTX master lệch trạng thái khi dùng toggle từng effect. | `Source/ReadyOrNot/ReadyOrNotDebugSubsystem.cpp:144-226` | 109 |
| 077 | Low | §16 Tooling | RemoveReport dùng RemoveAt với ID cũ có thể xóa sai note; xóa theo location không khớp toàn chuỗi nên thành code vô hiệu. | `Source/ReadyOrNot/Debug/BadAIAction.cpp:148-156` | 109 |
| 078 | Low | §16 Tooling | DevMenu giữ pointer vào mảng KeyBindings của InputComponent; tái cấp phát có thể làm dangling pointer. TDD chưa trace tới crash. | `Plugins/DevMenu/Source/DevMenu/Private/DevMenu.cpp:379-383` | 109 |
| 079 | Low | §16 Tooling | Load-time test hard-code 19 map; các test không nối vào CI được tác giả khảo sát. | `Source/ReadyOrNot/Testing/ConsoleAutomation.cpp:243-287` | 109 |
| 080 | Low | §2 Architecture | GetKey kiểm tra khóa đã bỏ khoảng trắng nhưng đọc khóa chưa bỏ; khóa có khoảng trắng trả 0 thay fallback. | `GameplayConfig.h:61-65`; `GameplayConfig.cpp:17-19` | 109 |
| 081 | Low | §2 Architecture | GetWorldStatic chọn play world/PIE instance 0, mơ hồ khi nhiều client PIE; GetReadyOrNotLevelScript thiếu null-check. | `lib/BpGameplayHelperLib.cpp:70-82`; `lib/ReadyOrNotStatics.cpp:11` | 109 |
| 082 | Low | §2 Architecture | Module chính include Private header của plugin InputRemapping. | `ReadyOrNot.Build.cs:24` | 109 |
| 083 | Low | §2 Architecture | Có 18 file `.cpp.bak` lạc trong source, một số khác file live. | `Characters/ReadyOrNotPlayerController.cpp.bak` | 109 |
| 084 | Low | §2 Architecture | Bốn pattern manager không nhất quán, không có quy tắc lựa chọn được nêu. | `Info/*.h` | 109 |
| 085 | Low | §2 Architecture | Hai thư viện utility chồng trách nhiệm; cast GameSingleton bị lặp. | `lib/BpGameplayHelperLib.cpp:89`; `lib/ReadyOrNotFunctionLibrary.cpp:325` | 109 |
| 086 | Low | §3 Player | ToggleSprint luôn gọi Sprint→FastWalk vì bWalking không đổi dưới RON_NO_SPRINT, làm trạng thái slow walk bị giữ. | `Characters/PlayerCharacter.cpp:7379-7397`; `PlayerCharacter.h:2006` | 110 |
| 087 | Low | §3 Player | Penalty tốc độ khi stun áp hai lần, mỗi lần 0,75 ở GetRunSpeed và bRepStunned. | `Characters/PlayerCharacter.cpp:3997-4007,1621`; `ReadyOrNotCharacter.cpp:1223` | 110 |
| 088 | Low | §3 Player | ForceLowReadyVolume chỉ WhileInside chưa đủ ép low-ready vì còn yêu cầu WhileAimingAt. | `Characters/PlayerCharacter.cpp:4948-4952` | 110 |
| 089 | Low | §3 Player | SlowDownVolume không stack; rời vùng reset về 1,0; bind cả actor/component overlap khiến event phát đôi. | `Actors/Environment/SlowDownVolume.cpp:14-60` | 110 |
| 090 | Low | §3 Player | Sprint, jump và vault nằm sau global define bị tắt; DoVaultTraces đã comment out. | `ReadyOrNot.h:290-292`; `Characters/PlayerCharacter.cpp:6928-7080` | 110 |
| 091 | Low | §3 Player | ApplyCameraLocation là virtual rỗng nhưng được gọi mỗi tick. | `Characters/PlayerCharacter.cpp:3765-3768,1631` | 110 |
| 092 | Low | §3 Player | SetRunSpeed clamp rồi ghi đè bằng giá trị chưa clamp. | `Characters/PlayerCharacter.cpp:3795-3802` | 110 |
| 093 | Low | §4 Weapons | ApplyHitscanDamage trộn damage từ ShotAmmoType với curve/penetration của CurrentAmmoType. | `Actors/BaseMagazineWeapon.cpp:2355-2363` | 110 |
| 094 | Low | §4 Weapons | Cvar bỏ qua hit validation được đọc mà không có Shipping guard. | `Actors/BaseMagazineWeapon.cpp:63,2241` | 110 |
| 095 | Low | §4 Weapons | Toggle Safe là stub deprecated nhưng FM_Safe vẫn còn trong enum/NextFireMode. | `Actors/BaseWeapon.cpp:395-445` | 110 |
| 096 | Low | §4 Weapons | Năm file source `.bak` nằm cạnh code live. | `Actors/BaseMagazineWeapon.cpp.bak`; `Actors/Attachments/WeaponAttachment.cpp.bak` và 3 file khác theo TDD | 110 |
| 097 | Low | §5 Damage | Tunable stun không có reader C++: WeaponDownLengthOnStun, StunSpeedMultiplier, LessThanLethalAmount, pepperball StunDurationHeadshot/Leg; thời gian stun player hard-code. | `DamageTypes/StunDamage.h:103-114`; `DamageTypes/LessLethal/PepperballDamageType.h:15-18`; `ReadyOrNotCharacter.cpp:7471-7475` | 110 |
| 098 | Low | §5 Damage | StartBeingTasered bỏ qua PingStunDuration, nên stun state có thể lệch countdown taser 8 giây. | `ReadyOrNotCharacter.cpp:7548-7551`; `Actors/Items/Taser.cpp:913-916` | 110 |
| 099 | Low | §5 Damage | IsUnconsciousNotDead luôn false và không override; nửa unconscious của IsDeadOrUnconscious không có hiệu lực trong C++ được đọc. | `ReadyOrNotCharacter.cpp:1612-1620` | 110 |
| 100 | Low | §5 Damage | Nhánh self-heal hồi đầy không tới được với HealCount mặc định 3. | `Components/BleedComponent.cpp:31,153-156,244` | 111 |
| 101 | Low | §5 Damage | Causer ABulletProjectile khiến TryApplyBulletDamage return trước xử lý bone/armor/limb. | `Characters/PlayerCharacter.cpp:2847-2885`; `ReadyOrNotCharacter.cpp:6456-6488` | 111 |
| 102 | Low | §5 Damage | Proxy client chỉ thấy một cặp replicate `(bRepStunned, RepStunnedWith)`, không toàn bộ tập stun. | `ReadyOrNotCharacter.cpp:1223-1233,7420-7435` | 111 |
| 103 | Low | §6 Items | `UInventoryComponent::CanEquip` là no-op luôn true. | `Source/ReadyOrNot/Components/InventoryComponent.cpp:925` | 111 |
| 104 | Low | §6 Items | Danh sách body visibility hard-code trong header. | `Source/ReadyOrNot/Components/InventoryComponent.h:237-239` | 111 |
| 105 | Low | §6 Items | C2 không dùng DamageInnerRadius/OuterRadius; radial stun hard-code 100/300. | `Source/ReadyOrNot/Actors/Gameplay/PlacedC2Explosive.cpp:139,172` | 111 |
| 106 | Low | §6 Items | RemoveFromTarget duyệt mọi ADetonator. | `Source/ReadyOrNot/Actors/Gameplay/PlacedC2Explosive.cpp:313-328` | 111 |
| 107 | Low | §7 Doors | Nhánh one-way BreakDoor(true) của BreachDoorFromPoint không tới được sau clamp `[0,Max]`. | `Actors/Door.cpp:6347-6356` | 111 |
| 108 | Low | §7 Doors | Morale đường BSG đọc RepeatBreachDamageFactor/MaximumDoorBreachesImpactingMorale nhưng thiếu fallback như đường ram. | `Actors/Door.cpp:6377-6378` | 111 |
| 109 | Low | §7 Doors | Gọi Multicast_CheckSupports rồi gọi trực tiếp `_Implementation` khiến server kiểm tra support hai lần. | `Actors/Door.cpp:4880-4881,4895-4896,5005-5006` | 111 |
| 110 | Low | §7 Doors | bCanBreakOffWithKick bị tắt trong KickDoor nhưng còn trong KickSubDoor, nên chỉ cánh cửa đối tác tách ra. | `Actors/Door.cpp:6633-6639` đối chiếu `:6725-6728` | 111 |
| 111 | Low | §7 Doors | bCantKick chặn kick mặt trước của cửa one-way Forward; nhánh break-off chỉ chạy với forced kick. | `Actors/Door.cpp:6576-6628` | 111 |
| 112 | Low | §7 Doors | SetLocked trên cửa mở hoặc đang di chuyển lại mở khóa thay vì trì hoãn khóa. | `Actors/Door.cpp:4463-4467` | 111 |
| 113 | Low | §7 Doors | Code detach/launch lặp và lệch mass/impulse/scale: 0,92 so với 0,98, 100 kg so với 50 kg. | `Actors/Door.cpp:4756-4763,6439-6448,6781-6800,6482-6534` | 111 |
| 114 | Low | §8 Command UX | LoadDefaultCommands không tải DefaultCheckDoorCommand, nên giữ SC_StackUp. | `HUD/Widgets/SwatCommandWidget.cpp:1963`; `SwatCommandWidget.h:97` | 111 |
| 115 | Low | §8 Command UX | Sentinel container không thống nhất giữa PC_* và SC_None. | `lib/ReadyOrNotCommandFunctionLibrary.cpp:124`; `HUD/Widgets/SwatCommandWidget.cpp:199,2380` | 112 |
| 116 | Low | §8 Command UX | Màu team khai báo bốn lần với giá trị khác nhau. | `SwatCommandWidget.h:239-245`; `SwatCommandEntryWidget.h:33-39`; `SwatCommandStatusWidget.h:111-115`; `Console/CommandWheel.h:97-99` | 112 |
| 117 | Low | §8 Command UX | CommandCombo trong library chỉ được ghi, không đọc. | `lib/ReadyOrNotCommandFunctionLibrary.h:19`; `.cpp:3741,3810,3844` | 112 |
| 118 | Low | §8 Command UX | DoCommand trộn gọi subsystem trực tiếp với server RPC; TDD định khung đây là rủi ro bảo trì, không khẳng định mọi nhánh sai authority. | `HUD/Widgets/SwatCommandWidget.cpp:2810-2852` | 112 |

## Các mô típ để học, không phải bản vá đã xác minh

TDD nhóm nguyên nhân lặp lại ở tr.112–113: tính năng chỉ hoạt động trong một build flavor; vòng lặp thiếu break hoặc dừng quá sớm; tin giá trị client; tunable còn trong editor nhưng reader đã bị xóa/comment out; sai kiểu số nguyên hoặc cờ trạng thái không reset. Các dòng về whole-array replication hay coupling là rủi ro thiết kế/bảo trì, khác với một crash đã tái hiện. Dòng về API key chỉ ghi sự hiện diện theo tác giả, không cung cấp bí mật.

**Proposed — bài thực hành tái tạo:** chọn một mô típ, viết một ví dụ nhỏ bằng code/asset riêng và tạo phép thử có đối chứng. Ví dụ FSM chỉ được chuyển một lần mỗi tick; chọn interrupt cho cùng kết quả khi đảo thứ tự candidate; RPC dùng state server để kiểm tra đầu vào; min/max roster bị validator từ chối trước runtime; feature flag có một nơi khai báo và ma trận build rõ ràng.

**Phụ thuộc:** repository riêng, automation harness và log có build configuration/net mode/version dữ liệu. **Tiêu chí đạt:** mỗi nhận định có điều kiện tái hiện, kết quả mong đợi, kết quả quan sát và giới hạn; lỗi không tái hiện được ghi “chưa tái hiện”, không được xóa nhãn nguồn hoặc tuyên bố đã sửa. Với vấn đề chỉ suy luận, trước tiên xác định call path/data/Blueprint còn thiếu. Dùng [quy trình kiểm chứng](../04-Kien-Truc-Va-Quy-Trinh/06-Quy-Trinh-Kiem-Chung.md) để tách chứng cứ tĩnh khỏi đo runtime; dùng [playbook](./18-reuse-playbook.md) để chuyển bài học thành thiết kế mới.

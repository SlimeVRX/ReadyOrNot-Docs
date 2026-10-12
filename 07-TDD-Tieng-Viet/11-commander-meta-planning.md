# 11. Commander, roster, stress, loadout và lập kế hoạch

> Chương diễn giải từ §11, trang in 69–75 của tài liệu R&D phân tích ngược ngày **27/09/2026**. Đây không phải TDD chính thức của VOID hoặc xác nhận bản thương mại hiện tại. Mỗi `file:line` là **TDD locator**, chưa được chương này kiểm chứng độc lập. “Config đi kèm” bên dưới chỉ `DefaultCommander.ini` trong bộ nguồn TDD đã khảo sát, không phải cam kết về cấu hình hiện hành trên máy người chơi.

## Ba loại trạng thái ngoài trận

Commander đặt hậu quả kéo dài lên vòng nhiệm vụ: tuyển officer, phân đội, chọn nhiệm vụ/loadout, xử lý stress và mất người. Cần tách ba loại dữ liệu: chiến dịch lưu lâu dài; preset loadout toàn cục; planning/TOC phục vụ phiên đang chơi. Chúng không cùng một save và cũng không có cùng authority. [TDD §11, tr.69–70.]

`UCommanderProfile` giữ campaign, completed levels, progression tags, roster blob, playtime và ironman. `UMetaGameProfile` là save toàn cục, gồm preset loadout. `URosterManager` là UObject do game mode tạo, quản lý runtime officer và không replicate. `URosterCharacter` giữ identity, trait, stress/state/storyline; `URosterManagerSettings` là `UDeveloperSettings(Config=Commander)`. Hub widget đọc manager đang sống, không sở hữu roster. `AMissionPlanManager` và `ATOCManager` là actor phiên chơi, không ghi vào save. [TDD §11, tr.69–70; TDD locator: `Commander/CommanderProfile.h:179`, `Commander/RosterManager.h:261`, `GameModes/LobbyGM.h:61`.]

`ACommanderGM::InitGame` nạp profile theo URL `?save=`, tạo roster rồi LoadFromProfile. Event nhiệm vụ như player/officer chết, kill, arrest, exfil được chuyển tới manager và sửa memory. Khi win, completion xử lý roster, SaveToProfile rồi ghi save; EndPlay lưu lần nữa. `UCampaignData` là asset tên/tác giả/danh sách level có thứ tự. `ARosterPortal` dù còn lớp đã tự Destroy trong BeginPlay, không nên lấy nó làm điểm vào UI hoạt động. [TDD §11, tr.69–70,75; TDD locator: `Commander/CommanderGM.cpp:24-46,99-165,194-360`, `Commander/RosterPortal.cpp:41`.]

## Stress là phép tích lũy có người chịu khác nhau

Recruits khởi đầu random `[0,MaximumStartingStress]`. Player chết/officer chết tăng stress squad; kill suspect hoặc civilian cho từng squad member một base random min–max, nhân thêm cho người bắn và nhân civilian khi phù hợp. Arrest giảm stress, suspect giảm nhiều hơn civilian. Exfil tăng theo mode; officer bị bỏ lại chịu mức riêng, còn bomb threat đang active thì đi qua xử lý killed để roll mất người. Hoàn tất `ron_campus_core` áp stress floor 0.6 cho toàn squad. [TDD §11, tr.70; TDD locator: `Commander/RosterManager.cpp:165-168,488-616,877`.]

Squad-wide gain nhân giá trị curve của trait `SBAGS`; trait thiếu hoặc curve rỗng trả 0. Arrest relief nhân `max(MoralOfficer,1)`, nên trait không làm giảm hiệu lực relief. Giá trị stress phải đọc sau cả event lẫn trait, không chỉ nhìn bảng base. TDD chưa đọc giá trị curve `RT_SBAGS`, nên khả năng zero mọi gain là rủi ro có điều kiện. [TDD §11, tr.70,74; TDD locator: `Commander/RosterManager.cpp:443-469,555-566`.]

Player giết friendly hoặc NPC surrender tăng `PlayerFriendlyKills`; lần đầu không thêm penalty stress này, các lần sau cộng `0.05 ×3^(n−1)` cho **toàn roster**, không chỉ squad. Lần thứ tư mức thêm 1.35 đủ đẩy người lên trần sau clamp. Đây là một event khác với gain thường do mất officer, nên “lần đầu miễn” chỉ nói riêng penalty này. [TDD §11, tr.70; TDD locator: `Commander/RosterManager.cpp:568-583`.]

## Therapy, nghỉ và permadeath

Trước review, stress clamp `[0,1]`. `ProcessRoster` xử lý ai đạt ≥1: còn chỗ therapy thì bắt buộc trị liệu cộng một nhiệm vụ phạt; hết chỗ thì removal reason Overstressed. Removed officers vào `PreviousCharacters`; memorial chỉ liệt kê Deceased, không mọi removal. Tự nguyện therapy cần available và stress đạt `MinimumStressForTherapy`. [TDD §11, tr.71; TDD locator: `Commander/RosterManager.cpp:176-184,402-411,619-666`, `Commander/MemorialWidget.cpp:23-31`.]

Khi vào therapy, stress clamp 0.99; thời gian vắng tính bằng `RoundFromZero((stress − min)/min ×TherapistTimeScale)`, ít nhất một nhiệm vụ. Timer chỉ tick cho officer đang bench; khi trở về therapy reset stress 0. Officer available nhưng nghỉ cũng giảm `PassiveStressLoss`. Comment nói tick chỉ khi win không hoàn toàn đúng: player death cũng gọi `TickTimers`, nên recovery và giảm stress có thể tiến trên nhiệm vụ thất bại; ý đồ chưa xác minh. [TDD §11, tr.71,75; TDD locator: `Commander/RosterManager.cpp:372-399,490,737-784`.]

`OnCharacterKilled` roll `DeathChance` clamp `[0,1]` từ roster stream. Nếu chết, reason Deceased dù storyline ngẫu nhiên có thể viết death hoặc unfit-for-duty; nếu incapacitated thì nghỉ random 1..MaxIncapacitationTime nhiệm vụ, stress tối đa MaxStressWhenIncapacitated. Paramedic làm tròn curve value thành int rồi so số officer đã killed trong trận để cứu; diễn giải “mỗi Paramedic cứu một người” phụ thuộc curve chưa đọc. Local human giết squad member đặt force-incapacitation, không roll chết. [TDD §11, tr.71; TDD locator: `Commander/RosterManager.cpp:499-539`, `Commander/CommanderGM.cpp:194-211`.]

## Trait, tuyển người và phân squad

Trait là `URosterTrait` chứa `FRuntimeFloatCurve`, đánh giá theo số available squad member mang trait. Mỗi completion mở trait cho **đúng một** officer eligible chọn ngẫu nhiên: đã đi đủ `MissionsUntilTraitUnlockable`, trait chưa mở. Nhưng TDD thấy điều kiện đếm trait bị đảo: khi kiểm unlock bình thường, hàm bỏ người `bTraitUnlocked=true`, tức đếm người còn khóa; CVar unlock-all che lỗi. Điều này ảnh hưởng SBAGS, MoralOfficer và Paramedic. [TDD §11, tr.71,74; TDD locator: `Commander/RosterManager.cpp:443-469,786-813`, đặc biệt `:461`.]

Roster capacity = `RosterSize` cộng một slot cho mỗi threshold AdditionalRosterSlots không lớn hơn số completed levels. Roster mới lại tạo `RosterSize−1` người. Pool ứng viên có `NumRecruitableCharacters`, tái tạo sau mỗi hire và mỗi ProcessRoster. Bốn vị trí squad là RedOne/RedTwo/BlueOne/BlueTwo; `ValidateSquad` bỏ trùng rồi điền vị trí trống bằng officer available chưa gán đầu tiên. Nếu vẫn thiếu lúc spawn pawn, hệ tạo stand-in không thêm vào roster. [TDD §11, tr.71; TDD locator: `Commander/RosterManager.cpp:216-236,273-290,687-735,815-819`, `Commander/CommanderGM.cpp:310-335`.]

Firing cho phép khi `FiredCount < MaxOfficersFiredAtOnce`; số âm là unlimited. Count reset trong CompleteMission, khác comment “khi bắt đầu”. Mỗi firing cập nhật stress mọi người bằng `min(s + gain, max(cap,s))`, nên không thể làm stress giảm. Officer fired bị xóa khỏi Characters nhưng không vào PreviousCharacters, không có hồ sơ lưu lịch sử. [TDD §11, tr.71,75; TDD locator: `Commander/RosterManager.cpp:172-173,335-370`.]

## Phải đọc config cùng mặc định C++

| Setting | C++ mặc định | Config đi kèm TDD |
|---|---:|---:|
| RosterSize | 10 | 7 |
| NumRecruitableCharacters | 3 | 3 |
| AdditionalRosterSlots | {3,6,9}, max 13 | {3,5,7,9,11}, max 12 |
| MaximumStartingStress | 0.35 | 0.2 |
| MissionsUntilTraitUnlockable | 2 | 2 |
| DeathChance / MaxIncapacitationTime / MaxStressWhenIncapacitated | 0.5 / 3 / 0.9 | Như C++ |
| MaxOfficersFiredAtOnce / StressGainForOfficerFired / MaxStressForOfficerFired | 1 / 0.15 / 0.9 | Như C++ |
| StressGainForOfficerKilled | 0.2 | 0.2 |
| StressGainForPlayerKilled | 0.25 | 0.15 |
| StressGainForExfil / ActiveShooterExfil | 0.05 / 0.25 | Như C++ |
| StressGainForOfficerNotExfil | 1.0 | 0.5 |
| StressGainForFriendlyKilledByPlayer / Base | 0.05 / 3.0 | Như C++ |
| Min/MaxBaseStressGainForKill | 0.05 / 0.1 | 0.04 / 0.08 |
| StressMultiplierForKillInstigator / CivilianKill | 1.3 / 2.0 | Như C++ |
| StressLossForCivilianArrest / SuspectArrest | 0.05 / 0.1 | 0.03 / 0.08 |
| PassiveStressLoss | 0.05 | 0.125 |
| MaxCharactersInTherapy / MinimumStressForTherapy | 3 / 0.5 | Như C++ |
| TherapistTimeScale | 4.0 | 2.0 |

Bảng giữ tên rút gọn ở các cặp nhưng cùng ý nghĩa như nguồn. Config có 14 AvailableTraits, EOD và BorderPatrol disabled; SBAGS, Paramedic, MoralOfficer enabled. `MaximumStartingAccuracy/Reflexes` không có property C++ tương ứng, là dead config. Tên/ảnh/voice/backstory/storyline/reminder lấy archetype primary assets và event data, không phải chuỗi viết cứng trong manager. [TDD §11, tr.72–73; TDD locator: `Commander/RosterManager.h:261-388`, `Config/DefaultCommander.ini`, `Commander/RosterManager.cpp:910-928`.]

Có hai lệch comment đáng tránh: therapy capacity −1 được comment unlimited nhưng `IsTherapyFull` lại true khi maximum <0, thành tắt therapy; mô tả gần 100% stress nghỉ khoảng bốn mission theo C++ scale 4, trong config scale 2 cho khoảng hai, forced therapy cộng một. [TDD §11, tr.74; TDD locator: `Commander/RosterManager.cpp:413-419`, `Config/DefaultCommander.ini`.]

## Save, ironman và progression

Mỗi officer được serialize riêng qua `FObjectAndNameAsStringProxyArchive`/memory writer thành byte blob trong `FRosterSaveData`, gồm live và previous lists. Stream khôi phục từ RosterSeed; seed −1 ở profile mới thì lấy `FMath::Rand`. Lưu seed tạo cơ sở tái lập random theo thiết kế nguồn, nhưng blob opaque làm inspection/migration khó hơn dữ liệu schema tường minh. [TDD §11, tr.72,75; TDD locator: `Commander/RosterManager.cpp:51-131`, `Commander/CommanderProfile.h:10-40`.]

Ironman bật khi tạo profile. Khi tạo end widgets, nếu mọi human đã chết, bỏ SWAT AI khỏi phép all-dead, controller local xóa profile, null con trỏ để EndPlay không lưu lại, rồi mở IronManGameOver. `ironman_flawless` khi mọi campaign level complete và PreviousCharacters rỗng. Officer fired không vào previous nên firing không phá flawless theo implementation được mô tả. [TDD §11, tr.72; TDD locator: `Commander/CommanderGM.cpp:174-238`.]

Completion thêm tag `<prefix>_grade_<g>` cho mọi grade tới kết quả đạt, luôn thêm f, thêm e từ 50%. `ULevelCompleteRequirement` xây tag để kiểm lock, nhưng switch thiếu `ELevelGrade::E`, rơi về “none”; requirement grade E tìm `_grade_none` thay vì `_grade_e` và không mở theo đường dự kiến. [TDD §11, tr.72,74; TDD locator: `Info/ScoringManager.cpp:1407-1434`, `GameModes/CoopGS.cpp:513-524`, `Data/ProgressionData.cpp:10-27`.]

## Loadout và ready-up

Loadout budget là sức mang của armor, không phải tiền. Maximum lấy `TotalSlots` từ armor CDO, không armor fallback 10. Current cộng grenade, tactical, primary/secondary ammo rồi clamp capacity. Đổi sang giáp ít slot dùng sanitize cắt excess theo thứ tự tactical → grenade → secondary ammo → primary ammo. Preset `FSavedLoadout` lưu trong global `UMetaGameProfile::LoadoutPresetMap`. [TDD §11, tr.72; TDD locator: `lib/ReadyOrNotLoadoutManager.cpp:595-661,1062-1141`.]

Mission portal tính `RequiredReadyPlayers = ceil(playerControllers/2)`, chia None/Minority/Majority/All. Giữ đúng công thức nguồn: với số người chẵn, ngưỡng này có thể đúng một nửa, dù state tên Majority. Chuyển state gửi reliable multicast timer: All mặc định 10 s, Majority 120 s, dưới ngưỡng thì cancel. URL lấy mission/mode; Commander thay bằng `?game=commander?save=<slot>`, có entry point thì nối `?entrypoint=<name>`. [TDD §11, tr.72–73; TDD locator: `Actors/Environment/MissionPortal.cpp:659-675,870-915`, `.h:159-160`.]

Portal replicate mission/mode URL, entry point và ready count, **không replicate timer đang chạy**. Một multicast khởi động đồng hồ local rồi tự tick; late join hoặc hitch không được resync. Thêm server deadline là đề xuất tái tạo hợp lý. `SelectRandomMission` lặp `while(true)` tới mode selectable, có thể treo game thread nếu map không có mode nào chọn được. [TDD §11, tr.74–75; TDD locator: `Actors/Environment/MissionPortal.h:60-69,152-153,172`, `.cpp:621-652,917-930`.]

## Bảng kế hoạch và radio TOC

Plan manager dùng fast arrays MarkerArray/LineArray; cấp PlanningPlayerNumber nhỏ nhất còn trống từ 1. Lobby server xóa plan khi chọn mission; chỉ server được ClearPlan. Nét vẽ tự do khác marker/line: nằm trong player state `FPlanningDrawingArray`, giới hạn `MaxDrawings=10` và số điểm mỗi stroke. Đây là dữ liệu phối hợp phiên chơi, không thuộc roster save. [TDD §11, tr.72,74; TDD locator: `Info/MissionPlanManager.h:188-192`, `.cpp:47-109`, `Actors/Gameplay/ReadyOrNotPlayerState.h:374-381`, `.cpp:570-584`.]

TOC queue có bốn mức: Flush xóa queue rồi thêm line; High chèn đầu hoặc index 1 nếu **phần tử cuối** là Flush; Medium nối cuối; Low chỉ thêm khi queue rỗng. Line đầu một burst phát prefix. Server chọn file voice ngẫu nhiên rồi multicast chính xác tên để các máy cùng take; client không tự đưa networked line vào queue. `ROEViolateTOCResponseDelay=1.15 s`. [TDD §11, tr.72–73; TDD locator: `Info/TOCManager.cpp:65-86,135-201,224-235`, `.h:33`.]

Kiểm cuối queue thay đầu khiến High có thể vượt Flush đang chờ trong `[Flush,Medium]`. Comment Low nói thay line Low cuối nhưng code thực bỏ nếu queue không rỗng. `QueuedTOCData` được khai Replicated rồi bị `DISABLE_REPLICATED_PROPERTY`; queue không replicate, chỉ file được multicast. Không nên “sửa” bằng bật replicate lại khi chưa hiểu giao thức. [TDD §11, tr.74–75; TDD locator: `Info/TOCManager.h:54-55`, `.cpp:57-63,165-183`, `Data/TOCData.h:13`.]

Roster/profile chỉ ở host; clients thấy hiệu ứng như customization pawn. Widget therapist có thể trả placeholder “Hello world” nếu danh sách therapy cached lệch live roster, đồng thời chia squad count không guard 0. Mở rộng officer/storyline bằng asset, trait bằng asset/config và gameplay consumer; requirement bằng subclass; campaign bằng asset nhưng hỗ trợ modded campaigns còn TODO. QA có CVars unlock traits, unlimited firing, unlock missions và override previous level; chúng không thay thế test đường bình thường. [TDD §11, tr.73–75; TDD locator: `Commander/TherapistWidget.cpp:43-93`, `Commander/RosterManager.cpp:11-12`, `Commander/CampaignData.h:8`, `Actors/Environment/MissionPortal.cpp:16`, `StationSubLevelController.cpp:12`.]

## Bài thực hành tái tạo

**Kịch bản tự thiết kế:** squad bốn người xong nhiệm vụ, một officer therapy, một incapacitated, rồi đổi giáp và chuẩn bị nhiệm vụ mới. Host sở hữu roster/save/progression; server sở hữu ready state, deadline và plan; client gửi thao tác. States officer: Available/Assigned/Therapy/Incapacitated/Removed; lưu removal reason riêng. Phụ thuộc: event ledger, config, trait curves, seeded RNG, save version và UI đọc state sống.

**Tiêu chí chấp nhận:** bảng review khớp event tính stress; thiếu trait không âm thầm xóa mọi gain; −1 therapy có nghĩa được định nghĩa và kiểm thử; officer fired còn audit record trong prototype; save-load giữ identity/timer/RNG theo hợp đồng đã chọn; grade E mở đúng requirement; đổi giáp cho thấy thứ tự cắt slot; late join đọc đúng countdown từ deadline; TOC Flush không bị High vượt. Thử ironman trên save tạm của prototype, không dùng save thật. Đây là bài thực hành thiết kế độc lập.

Đọc kèm [save, config và version](../04-Kien-Truc-Va-Quy-Trinh/04-Save-Config-Version.md), [vòng nhiệm vụ](../02-Gameplay-Va-AI/01-Vong-Nhiem-Vu.md), [lộ trình tái tạo](../04-Kien-Truc-Va-Quy-Trinh/05-Lo-Trinh-Tai-Tao.md).

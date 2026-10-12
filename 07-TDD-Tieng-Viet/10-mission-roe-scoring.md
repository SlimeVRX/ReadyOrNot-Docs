# 10. Nhiệm vụ, ROE, mục tiêu và chấm điểm

> Diễn giải §10, trang in 62–68 của tài liệu R&D phân tích ngược ngày **27/09/2026**. Đây không phải TDD chính thức của VOID và không xác nhận trạng thái bản thương mại hiện tại. Mọi `file:line` là **TDD locator**, chưa kiểm chứng độc lập. Giá trị asset chưa đọc, nhánh chỉ đọc header và kết luận suy luận được ghi rõ để không biến tên lớp thành tính năng đã xác nhận.

## Hệ trọng tài tách khỏi hành động chiến đấu

Một mục tiêu bị hạ là sự kiện gameplay; trận thắng hay thua và hành động đó hợp lệ hay không là quyết định của hệ luật. `AReadyOrNotGameMode` giữ match state, spawn/team và entry exfil. `AReadyOrNotGameState` replicate dữ liệu nhiệm vụ, objectives và planning. `ACoopGM/ACoopGS` chuyên biệt single-player/co-op PvE. `AScoringManager` giữ sổ điểm, penalty, thống kê và cache clear/fail. Mỗi actor có thể chấm điểm sở hữu `UScoringComponent`; `AObjective` cũng sở hữu một component như vậy. [TDD §10, tr.62–63; TDD locator: `ReadyOrNotGameMode.h:222`, `GameModes/CoopGS.h:32,52,100,152,201`.]

Component khi BeginPlay bỏ khoảng trắng trong group name; nếu enabled và auto-add thì server áp row DataTable rồi đăng ký vào manager. Gameplay phát `GiveScore/GivePenalty/TakeAllScores`; manager cập nhật pool/cache. GameMode poll kết quả thấp tần thay vì mọi actor tự kết thúc trận. Phân chia này làm điểm từng vật chứng và mục tiêu có nguồn gốc riêng, nhưng vẫn cần bảo vệ khỏi đăng ký hoặc phát thưởng hai lần. [TDD §10, tr.63; TDD locator: `Components/ScoringComponent.cpp:38-53,91-109`, `Info/ScoringManager.cpp:873-884`.]

## Vòng nhiệm vụ và ba cách kết thúc

Lobby chọn mode/map/loadout, không tính điểm; các lớp lobby chỉ được nêu, không mở trong phần này. Planning là sublevel stream, `PlanningTimeLeft=480 s`, có deployables, personnel và squad points. `StartMatch` gọi base rồi ResetLevel, RespawnAllPlayers, SpawnPolice, đặt poll, InitWorld, InitAI, ResetSquadLeader. `TimeTillGameStartCountdown=10 s` tồn tại nhưng cách gate countdown chưa traced. Poll thắng/thua lặp 1 s, lần đầu sau 10 s. [TDD §10, tr.62–63; TDD locator: `ReadyOrNotGameState.h:272,297-304,505`, `GameModes/CoopGM.cpp:134-157`.]

Trong `MS_Playing`, chết hết human player bắt đầu kết thúc thua. Manager tick các objective đang làm, GameMode kiểm primary/failure. Hard clear là hoàn tất các nhóm bắt buộc; soft complete có thể khi mọi primary xong hoặc soft-clear cache đúng, cho phép người chơi kết thúc trước khi thu dọn toàn bộ. `bMissionSucceded` được gán từ phủ định fail cache; soft complete không đồng nghĩa bỏ mọi penalty. [TDD §10, tr.63; TDD locator: `GameModes/CoopGM.cpp:664-763`, `Info/ScoringManager.cpp:860-871,1254-1293`.]

Khi hard clear hoặc vote `VS_MajorityYes`, gọi completion một lần rồi end với success thật. Soft completion lần đầu mở vote “End The Mission?”, chuyển InProgress và đổi prompt exfil thành End Mission. `StartMissionEndTimer` phát TOC, multicast kết quả và chờ 5 s; timer đã chạy thì bỏ lời gọi sau, nên verdict đầu giữ nguyên. `MissionEnd` chỉ chạy khi đang Playing, đặt MatchEnded, multicast lần hai, tắt AI, khóa pawn/input sang UI và mở debrief. [TDD §10, tr.63–64; TDD locator: `GameModes/CoopGM.cpp:780-801,1715-1763`.]

Portal exfil chạy qua server, yêu cầu exfil enabled. Nếu đã soft complete, nó bỏ đường abort, bỏ phiếu yes hoặc ép majority rồi để poll kết thúc đúng success flag. Chỉ exfil giữa nhiệm vụ mới phạt **500** và gọi `ExfiltrateMission`: đặt exfil flag, dừng poll, bắt đầu end(false). `AExfilActor` chỉ là target lệnh có tag Exfil, bản thân nó không kết thúc trận. Phân biệt portal, actor lệnh và hàm abort tránh nối sai luồng. [TDD §10, tr.63–64; TDD locator: `Actors/Environment/ExfilPortal.cpp:185-228`, `Actors/Gameplay/ExfilActor.cpp:9-44`, `ReadyOrNotGameMode.cpp:3072-3077`, `GameModes/CoopGM.cpp:2073-2078`.]

## ROE là chuỗi điều kiện có thứ tự

`IsROEDisabled` true cho ActiveShooter và Raid; false cho None, BombThreat, HostageRescue, BarricadedSuspects và default. Nhưng kiểm mode nằm **sau** nhiều điều kiện bảo vệ mục tiêu, nên “ROE disabled” không có nghĩa được phép gây mọi loại damage. [TDD §10, tr.64; TDD locator: `GameModes/CoopGM.cpp:545-557`, `Enums.h:36-45`.]

`IsUnjustifiedUseOfForce` đi qua các nhóm sau:

1. Không aggressor, tự gây damage hoặc god-mode aggressor ở non-shipping được coi justified. SWAT đánh SWAT luôn unjustified. Mục tiêu đang arterial bleed được justified.
2. Area stun Flash/Gassed/Stung luôn justified. Direct stun pepperball/spray/beanbag/taser/rubberball: mục tiêu đã/đang còng hoặc đang được mang thì unjustified; có aggressive force trong 4 s thì justified; đã surrender 1.5 s thì unjustified; còn lại justified.
3. Nhánh dân thường áp dụng theo điều kiện nguồn nêu về aggressor gây kill/incapacitate hoặc lần damage cuối do player: đã còng/đang còng/carried thì unjustified; aggression trong 2 s thì justified; surrender 1.5 s thì unjustified; đã gây damage cho SWAT/aggressor thì justified; còn lại unjustified. Nhánh này trước kiểm mode.
4. Đối tượng khác: còng/carried bị cấm; aggression trong 4 s được phép; surrender 1.5 s bị cấm; dead/unconscious/incapacitated hơn 3 s bị cấm; suspect từng rời surrender được phép.
5. Đến đây mới xét ROE-disabled mode và cho justified.
6. Đã hại SWAT/aggressor hoặc mặc vest nổ được justified. Sau đó xét behavior: melee, nâng súng, bắt con tin, đã bắn, đang aim/rút súng được justified; giả chết hơn 2 s, đang đứng lên, do dự 1.5 s, startled bị unjustified. Nhánh fall-through dựa combat activity chưa được nguồn đọc.

Các cửa sổ thời gian là điều kiện gameplay của snapshot nguồn, không phải hướng dẫn hay chuẩn tác chiến thực. [TDD §10, tr.64; TDD locator: `Characters/CyberneticCharacter.cpp:1299-1460+`.]

Player gây non-lethal damage trái phép cho non-SWAT nhận Unauthorized Force; unjustified kill lấy hết điểm mục tiêu rồi phát cả Unauthorized Force và Unauthorized Deadly Force. Một call site force penalty khác ở dòng 4677 chưa rõ context. Tên penalty còn gồm Friendly Fire, Team Kill, không báo officer gục, giết người incapacitated, civilian killed, trap triggered và exfil. Giá trị hầu hết ở `FScoringDataTable`, **không đọc asset nên không điền số từ trí nhớ game**; 500 exfil là ngoại lệ hard-code. [TDD §10, tr.64; TDD locator: `Characters/CyberneticCharacter.cpp:2871-2875,4677,5186-5191`, `Info/ScoringManager.cpp:36-55`, `Components/ScoringComponent.cpp:23-36`.]

## Pool điểm, tỷ lệ và grade

Manager bỏ group rỗng, sort `OrderPriority`, cộng cả active/inactive score vào primary/secondary/tertiary pool. Tổng bao gồm bonus enabled hoặc disabled, đã cho hoặc chưa cho. Điểm thực = tổng bonus đã cho − penalty; hàm trả 0 nếu `bIsBuildPirated`. Hàm tính thô không clamp, caller mới clamp `[0, TotalScorePool]` để tính tỷ lệ cuối. [TDD §10, tr.64; TDD locator: `Info/ScoringManager.cpp:133-188,1318-1323,1395-1406,1774-1811`.]

| Tỷ lệ tối thiểu | Grade |
|---:|:---|
| 1.00 | S |
| 0.95 | A+ |
| 0.90 | A |
| 0.80 | B |
| 0.70 | C |
| 0.60 | D |
| 0.50 | E |
| Dưới 0.50 | F |

Có floor E khi mọi primary complete dù tỷ lệ dưới 0.5, nhưng được viết hai lần: hàm grade từ score trả F trước nếu score ≤0; hàm percentage vẫn nâng lên 0.5. Do vậy zero score + primary complete có thể hiển thị F hoặc E tùy caller. Đây không phải hai hệ luật có chủ ý được xác nhận. [TDD §10, tr.65,67; TDD locator: `Info/ScoringManager.cpp:1330-1406`.]

## Objective, evidence và report

`AObjective` là replicated always-relevant `AInfo`, mặc định primary, group MissionObjectives; trạng thái InProgress/Complete/Failed. Complete lúc hidden là no-op nếu không bật `bAllowCompletionWhileHidden`, và chỉ cho enabled scores. Fail lấy lại điểm đồng thời disable. Unlock có None, Reportable, Objective, Self; lúc tạo nhiệm vụ nó bind actor đầu tiên có class bằng class yêu cầu. Hidden objective phụ thuộc objective khác cũng fail nếu điều kiện mở khóa fail. [TDD §10, tr.65; TDD locator: `Objectives/Objective.h:19-30`, `.cpp:6-70,112-157`.]

Các ví dụ gồm ArrestXSuspects mặc định 1; neutralize/rescue theo tag; rescue-all hoàn thành khi mọi dân thường restrained và fail khi có civilian killed; report theo tag mặc định 1; pickup evidence theo tag; defuse bomb; interact door; bring order; don't die. Vài loại chỉ đọc header, logic tag evidence còn là suy luận. Thêm objective bằng subclass, tick hoặc event rồi gọi Completed/Failed; không tự viết một sổ điểm riêng. [TDD §10, tr.65–66; TDD locator: `Objectives/RescueAllOfTheCivilians.cpp:22-58`, `Objectives/ArrestXSuspects.h:20`, `Objectives/ReportReportableByTag.h:17`.]

Evidence dùng collection có thời gian: default 2.0 s bị ghi đè bởi `GetEvidenceCollectionTime()` của collector. Hoàn tất spawn bag, clone scoring component, đăng ký pool, cho điểm và chép tags. Một pickup path khác cũng thưởng component gốc, tạo nghi vấn double pool chưa xác nhận vì chưa kiểm việc gỡ original. Reportable là volume box/sphere/capsule, mặc định box extents 200 uu, radius 50, capsule height 200; report cho điểm, broadcast, disable và đặt reported. Incrimination clue nối `NextClue`, marker sau 120 s. [TDD §10, tr.65,67; TDD locator: `Actors/Gameplay/EvidenceActor.cpp:216-219,289,315-328`, `ReportableActor.h:69-79`, `.cpp:232-239`, `IncriminationClue.h:84`.]

## Mode, spawn, tuning và replication

Co-op chỉ spawn bốn SWAT AI khi có tối đa một human và không phải dedicated: Alpha/Beta Blue, Charlie/Delta Red, thêm bốn trailer entries; ngoài điều kiện đó total officers lấy player count. Suspect/civilian dùng roster spawner khi `GT_RandomScenarios`, còn lại random entry mỗi `AAISpawn`. Bomb subset random tới `MaxBombs`. Các tên mode khác như ArrestAndRescue, CTF, Defusal, Firefight, KOTH, TDM, VIP, Training, FreeMode và party modes tồn tại trong cây nhưng không được mở; tên không chứng minh tính năng sử dụng được. [TDD §10, tr.65–66; TDD locator: `GameModes/CoopGM.cpp:613-636,810-1019`.]

Default settings: MaxPlayers 8, RoundStartTime 160, ReinforcementTimer 30, RespawnTimer 5, Timelimit 1200, RoundsPerMap 2, TDMScoreLimit 0; game state Scorelimit 150. Co-op có `AISpawnDistance=5000`, `MaxHostagesKilledBeforeMissionFailed=1`, `MaxTeamKillsBeforeAIRetaliates=2`; hai field cuối không có C++ reader, Blueprint chưa kiểm. Bomb multitool 15.0 s, explosion radius 100000.0. Những số này thuộc lớp/dữ liệu được ghi, không mặc định áp dụng mọi mode. [TDD §10, tr.66–67; TDD locator: `lib/DataSingleton.h:396-420`, `ReadyOrNotGameState.h:532`, `GameModes/CoopGM.h:186,189,286`, `Actors/Gameplay/BombActor.h:42-45`.]

Score mutation chặn non-authority, ngoại lệ `ChangeGroup` sửa row local không guard; `ChangeScoreGroup` có guard. Manager replicate objective groups, penalty groups, total pool, official flag, always relevant với NetUpdateFrequency 1.0/priority 2.0, không fast array/push model. Objective replicate status và OnRep phát lại event; unlock/display-score là reliable multicast. [TDD §10, tr.66; TDD locator: `Components/ScoringComponent.cpp:97-298`, `Info/ScoringManager.cpp:95-131`, `Objectives/Objective.cpp:25-30,106-109`.]

## Lỗi kết quả có thể phụ thuộc thứ tự

Objective loop break ở primary đầu tiên đang làm, nên có thể chưa thấy failure-ending objective phía sau. Clear-cache cũng break group chưa đạt đầu tiên: soft flag phụ thuộc group trước, group soft-only còn tính vào hard clear. Dự án mới nên tính đủ mọi điều kiện rồi quyết định một lần. [TDD §10, tr.66–67; TDD locator: `GameModes/CoopGM.cpp:691-704,733`, `Info/ScoringManager.cpp:1262-1274`.]

TDD còn chỉ ra chia `TotalScorePool` không guard 0: trường hợp 0/0 → NaN → Clamp có thể cho 100%/S là **suy luận chưa chạy**, không kết quả test. SWAT auto-clear gọi exfil abort(false), có thể báo fail dù đã thu dọn; mode nào tới nhánh này chưa traced. Verdict đầu thắng, multicast end hai lần, whole-array score replication, penalty exfil hard-code và evidence double pool là các điểm kiểm thêm. Một civilian death do player đặt fail bất kể hai tunable chết; không nên cân bằng bằng field không ai đọc. [TDD §10, tr.67; TDD locator: `Info/ScoringManager.cpp:771-772,1283,1387,1399`, `Info/SWATManager.cpp:1836-1854`, `GameModes/CoopGM.cpp:787,796,1725,2073-2078`.]

## Bài thực hành tái tạo

**Kịch bản tự thiết kế:** một primary cứu người, một secondary evidence, một objective fail kết thúc nhiệm vụ. Server sở hữu ledger, objectives, vote và verdict; UI đọc snapshot. States: Lobby → Planning → Playing → EndPending → Ended; objective có InProgress/Complete/Failed, hidden là cờ riêng. Phụ thuộc: event gameplay, scoring rows, idempotency key, timer và end UI.

**Tiêu chí chấp nhận:** đảo thứ tự objective không đổi thua/thắng; báo cùng người hai lần không cộng điểm; pool 0 có kết quả định nghĩa rõ; zero score và primary complete chỉ có một grade thống nhất; exfil sau soft complete đi đường kết thúc thường, giữa nhiệm vụ đi abort; verdict và UI end chỉ phát một lần. Viết bảng ca ROE theo thứ tự, gồm civilian trong Raid, direct stun người đã còng và suspect vừa aggressive trong 4 s. Đây là yêu cầu prototype học tập.

Liên hệ [vòng nhiệm vụ](../02-Gameplay-Va-AI/01-Vong-Nhiem-Vu.md), [mục tiêu, ROE và điểm](../02-Gameplay-Va-AI/02-Muc-Tieu-ROE-Diem.md), [quy trình kiểm chứng](../04-Kien-Truc-Va-Quy-Trinh/06-Quy-Trinh-Kiem-Chung.md).

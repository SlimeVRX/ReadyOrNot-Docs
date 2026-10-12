# 02 — Kiến trúc codebase: dữ liệu, vòng đời và dịch vụ

Chương này diễn giải **PDF §2, trang in 8–14**, thuộc bản R&D reverse-engineered độc lập ngày 27/09/2026; không phải TDD chính thức của VOID Interactive. Mô tả là kết quả khảo sát snapshot trong PDF, không xác nhận source hiện tại hay bản retail mới nhất. Các `file:line` là **định vị do TDD ghi**, chưa được kiểm tra độc lập ở chương này. “Suy luận” và “chưa xác minh” giữ đúng giới hạn bằng chứng của nguồn.

## 2.1. Kiến trúc giúp thay nội dung mà không sửa cơ chế — PDF §2, tr. 8–9

Một kiến trúc gameplay hữu ích phải giải quyết hai câu hỏi: dữ liệu nằm ở đâu, và ai sống đủ lâu để sở hữu nó? TDD dùng ví dụ designer thêm section theo tên map vào `Config/AILevelData.ini` để thay cách nghi phạm hành xử tại map ấy. Người chơi nhận thấy tính cách tình huống khác đi; họ không cần biết một giá trị đến từ INI hay Blueprint.

Cây `Source/ReadyOrNot` được §2 thống kê có 1.497 file `.h/.cpp`, không tính 18 `.cpp.bak`, và khoảng 247.650 dòng `.cpp`. Khoảng 22 header dữ liệu và 15 dịch vụ cấp world/session tạo nên nhiều điểm tinh chỉnh. Đây là phạm vi đếm hẹp hơn thống kê `Source/` ở §1.

| Nhóm/lớp | Trách nhiệm trong snapshot | Vòng đời hoặc cách truy cập |
|---|---|---|
| `UGameplayConfig` | Đọc INI có fallback theo ngữ cảnh | Lớp cơ sở trừu tượng; caller cung cấp mặc định C++ |
| `UReadyOrNotAIConfig` | Tuning AI theo map | GameInstance sở hữu; `Get()` dùng global world |
| `UReadyOrNotVoiceConfig` | Tra voice line, chu kỳ take tránh lặp liền | GameState tạo trong `BeginPlay` |
| `UDataSingleton` | `ULookupData`, cờ debug, loading data, `FGameModeSettings` | `GEngine->GameSingleton`, cấu hình bằng Blueprint |
| `AReadyOrNotGameMode` | Luồng trận có authority | Server; mỗi mode có subclass |
| `AReadyOrNotGameState` | Trạng thái trận và 12 mảng registry actor | World hiện tại; một số field replicate |
| `UReadyOrNotGameInstance` | Backend, loadout, ModioManager, AIConfig | Sống xuyên các world của session |
| `Data/*.h`, `Enums.h`, `Structs.h` | Asset, row và ngôn ngữ chung | 57 `UENUM`, 31 `USTRUCT` trong hai header dùng chung |

**Định vị do TDD ghi:** `ReadyOrNotAIConfig.cpp:17-28`; `ReadyOrNotGameState.h:170,204-222,358-370`; `ReadyOrNotGameState.cpp:343-345`; `ReadyOrNotGameInstance.h:379-388`; `ReadyOrNotGameMode.h:27`; `Enums.h`, `Structs.h`, `ReadyOrNot.h:63-64`.

Với người mới, GameMode và GameState không phải hai bản sao của cùng một object. GameMode quyết định luật ở server; GameState có thể chuyển những kết quả cần quan sát sang client. GameInstance tồn tại lâu hơn một world, phù hợp trạng thái session. Nhầm vòng đời này dễ gây dữ liệu cũ còn lại sau khi đổi map hoặc mất dữ liệu đáng ra phải tồn tại.

## 2.2. Một lời gọi tìm dịch vụ đi qua những gì? — PDF §2, tr. 8–10

Ba đường tiện ích cùng tồn tại. `UBpGameplayHelperLib` có 302 `UFUNCTION`, chứa `GetWorldStatic()` và `GetRoNData()`. `UReadyOrNotFunctionLibrary` có 106 `UFUNCTION`, gồm helper đọc INI. `UReadyOrNotStatics` có các accessor có kiểu cho LevelScript, GameInstance, GameMode, GameState, ConversationManager và PlayerController.

Luồng thường gặp là gameplay gọi static accessor → accessor tìm world → lấy GameInstance/GameState/AuthGameMode của world → cast về lớp RoN. TDD không thấy interface hoặc dependency injection ở giữa. `GetWorldStatic()` chọn `GEngine->GetCurrentPlayWorld()`; nếu không có thì lấy world của PIE instance 0. Cách này tiện ở một world, nhưng dễ mơ hồ trong nhiều cửa sổ client PIE cùng một process.

**Định vị do TDD ghi:** `lib/BpGameplayHelperLib.h:46`, `lib/BpGameplayHelperLib.cpp:70-96`; `lib/ReadyOrNotStatics.h:31-46`, `lib/ReadyOrNotStatics.cpp:9-42`. Nguy cơ lấy nhầm world được PDF đánh dấu **suy luận**, chủ yếu liên quan công cụ và kiểm thử, không được chứng minh là lỗi bản phát hành. `GetReadyOrNotLevelScript` còn dereference world thiếu null check tại `ReadyOrNotStatics.cpp:11`.

`GameSingletonClassName` trỏ `BP_DataSingleton_C`. Object này có trước world, còn default nằm trong asset nhị phân. Do đó chỉ đọc C++ sẽ không biết hết cấu hình thực tế. Hai utility library đều cast `GEngine->GameSingleton`; `UDataSingleton::Get()` được khai báo mọi cấu hình nhưng chỉ định nghĩa trong `#if WITH_EDITOR`, và TDD không tìm thấy `.cpp` gọi nó.

**Định vị do TDD ghi:** `Config/DefaultEngine.ini:38`; `lib/BpGameplayHelperLib.cpp:85-96`; `lib/ReadyOrNotFunctionLibrary.cpp:325`; `lib/DataSingleton.h:440`, `lib/DataSingleton.cpp:7-101`.

## 2.3. INI với ba mức lựa chọn giá trị — PDF §2, tr. 9–10

`UGameplayConfig` cung cấp getter cho `int`, `float`, `bool`, string, vector, vector2D và hai dạng string array. Logic chung `GetKey<T>` thử section cụ thể, thử section fallback, rồi mới dùng mặc định C++. File được đưa vào cache `GConfig` bằng `ReloadConfig()` gọi `GConfig->LoadFile`; kiểm tra key và đọc giá trị đều dùng cache đó.

Có thể mô tả thuật toán mà không sao chép source:

```text
Chuẩn hóa tên key.
Nếu section ngữ cảnh chứa key: đọc ở section ấy.
Nếu không, nếu section chung chứa key: đọc ở section chung.
Nếu cả hai không có: dùng mặc định do caller cung cấp.
```

| Lớp | File | Section cụ thể | Section fallback |
|---|---|---|---|
| `UReadyOrNotAIConfig` | `AILevelData.ini` | Tên world bỏ `StreamingLevelsPrefix` của PIE | `Global` |
| `UReadyOrNotVoiceConfig` | `DefaultVoice.ini` | `Settings` | `Settings`, nên không có fallback thứ hai thực chất |
| `UGameplayConfig` cơ sở | Tên rỗng | `Gameplay` | `Default` |

`AILevelData.ini` có 41 section trong snapshot. Một map chỉ cần khai báo giá trị khác biệt; phần còn lại thừa hưởng `Global` hoặc C++ default. Đây là cách hạn chế cấu hình trùng lặp. Tuy nhiên, tạo object mà quên `ReloadConfig()` có thể khiến mọi getter âm thầm rơi về mặc định; PDF ghi đây là **suy luận** từ đường kiểm tra khi cache chưa có file.

Một chi tiết vòng đời đáng nhớ: `UReadyOrNotAIConfig::Get()` fatal log nếu gọi trong constructor `UObject`. VoiceConfig có kiểm tra tương tự nhưng chỉ ở non-shipping. Constructor không phải nơi an toàn để mặc định rằng world và các dịch vụ runtime đã có.

**Định vị do TDD ghi:** `GameplayConfig.h:23-71`, `GameplayConfig.cpp:5-102`; `ReadyOrNotAIConfig.cpp:7-33`; `ReadyOrNotVoiceConfig.cpp:9-13,73-86`; `lib/ReadyOrNotFunctionLibrary.cpp:1200-1206,1261-1276`; `ReadyOrNotGameInstance.cpp:1531-1532`; `ReadyOrNotGameState.cpp:343-345`.

PDF chỉ ra một lỗi cụ thể: `GetKey` bỏ dấu cách trong bản sao key để kiểm tra tồn tại, nhưng lambda đọc dùng key nguyên bản. Key có dấu cách có thể vượt kiểm tra rồi đọc sai tên; output giữ giá trị khởi tạo 0 thay vì fallback mong muốn. Khi tái tạo, chuẩn hóa một lần và dùng cùng một key cho cả hai bước. Đây không phải kết luận rằng mọi key hiện có đang gây lỗi.

## 2.4. Dữ liệu nào nên thành asset, row hoặc settings? — PDF §2, tr. 10–11

Song song INI theo map là `UDeveloperSettings` với `UCLASS(Config=Game, DefaultConfig)`, dùng cho tuning toàn dự án qua Project Settings. Ví dụ có `UReadyOrNotCSGasSettings` và `USubtitlesSettings`; còn có section cho GameInstance, VoIP, impact effect, user settings và roster. Không nên mặc định mọi setting đều đi qua cơ chế fallback của `UGameplayConfig`.

Trong `Data/`, standalone `UDataAsset` có `UCSGasData`, `UDeployableData`, `UInteractionsData`, `UObituaryData`, `UPenetrationData`, `UPlayableCharacterData`, `URadialWidgetThemeData`, `USoundData`, `UTocSpeechData`, `UAIData`, `UItemData`, `UMusicData`. `UCustomizationDataBase` và `UModLevelData` dùng `UPrimaryDataAsset`. Các object quy tắc/instanced gồm `UActivityData` (`EditInlineNew`), `UPostProcessRequirement`, `UProgressionRequirement`. Row `FTableRowBase` gồm `FAIDataLookupTable`, `FLevelDataLookupTable` và row trong `WidgetDataTable.h`; `TOCData.h` chỉ giữ `USTRUCT`.

Sự khác biệt thực dụng là asset có danh tính riêng để tham chiếu; row đặt nhiều bản ghi cùng cấu trúc trong một bảng; object quy tắc biểu diễn một điều kiện hoặc hành vi cấu hình được. Không cần ép mọi dạng dữ liệu vào một công cụ duy nhất.

`FGameModeSettings` có các **default C++**, không phải thông số đã xác nhận cho mọi mode:

| Field | Default |
|---|---:|
| `MaxPlayers` | 8 |
| `RoundStartTime` | 160 s |
| `ReinforcementTimer` | 30 s |
| `RespawnTimer` | 5 s |
| `Timelimit` | 1.200 s |
| `RoundsPerMap` | 2 |
| `TDMScoreLimit` | 0 |

**Định vị do TDD ghi:** `Data/AIData.h:248,360`; `Data/ItemData.h:207`; `Data/LevelData.h:688,889`; `Data/CustomizationData.h:124-125`; `Data/ActivityData.h:61-62`; `lib/DataSingleton.h:396-427`; `Info/CSGasManager.h:13-14`; `Subsystems/SubtitlesSubsystem.h:39-40`; `Config/DefaultGame.ini:250-265`, `Config/DefaultCommander.ini:1`, `Config/DefaultGameUserSettings.ini:10`.

## 2.5. Voice line: ngẫu nhiên có kiểm soát — PDF §2, tr. 10–11

`FSequencedVOLookup` dựng một hoán vị chỉ số take bằng cách rút số ngẫu nhiên và bỏ số đã dùng. Sau đó phần tử đầu hàng đợi được chuyển xuống cuối; đọc take tiếp theo theo chu kỳ. `GetNextPlayIdx` tăng hàng đợi **trước** mỗi lần đọc, kể cả lần đầu. Với `N > 1`, một take không phát hai lần liên tiếp, nhưng thứ tự chu kỳ giữ nguyên khi quay vòng. Đây không phải rút ngẫu nhiên độc lập mỗi lần phát.

Danh sách take không hoàn toàn là dữ liệu tác giả điền sẵn. `Init()` quét đệ quy thư mục VO dưới Content, mỗi speaker một thư mục: editor dùng `.wav`, console dùng `.fsb`, các trường hợp khác lấy `Ext` trong INI, default `*.ogg`. Kết quả đi vào `VoicelineMap`. Vì vậy thêm một ID mà thiếu file đúng tên vẫn chưa tạo ra voice line hoạt động.

Catalog ID gồm `VO_SWAT_COMMAND` 106 ID, `VO_SWAT_GENERAL` 127, `VO_SUSPECTS_AND_CIVILIAN` 96, `VO_TOC` 11; `VO_PREFIXES` thêm BARK, TELL, REPLY. Mở rộng cần giữ ID và tên file đồng bộ. Khi tự làm, Fisher–Yates thích hợp hơn cách rút lại đến khi không trùng vì tạo hoán vị trực tiếp.

**Định vị do TDD ghi:** `ReadyOrNotVoiceConfig.h:8-47,72,86-481`; `ReadyOrNotVoiceConfig.cpp:28-70,88-111`.

## 2.6. Registry, manager và authority — PDF §2, tr. 9, 11–12

Registry là danh sách actor đã biết để truy vấn thường xuyên, tránh tìm toàn world mỗi lần. `AReadyOrNotGameState` khai báo mảng, còn actor tự thêm/xóa mình. Item và door thêm ở `BeginPlay`; character thêm ở `PossessedBy`; cả bốn loại xóa ở `EndPlay`. Đây là quy ước phân tán, không phải bảo đảm tự động của một base class.

Các mảng có `UPROPERTY(BlueprintReadOnly)` nhưng không có `Replicated`. Mỗi máy có bản riêng. Item/door chạy `BeginPlay` trên mỗi máy nên tự dựng danh sách tương ứng. TDD **suy luận** character registry chỉ được điền ở server dựa trên semantics thông thường của `PossessedBy`; việc có caller phía client thật sự cần các mảng đó **chưa xác minh**. Không nên biến nghi vấn này thành khẳng định rằng mọi tính năng client đều hỏng.

**Định vị do TDD ghi:** `Actors/BaseItem.cpp:241,300`; `Actors/Door.cpp:1047,1095`; `Characters/CyberneticCharacter.cpp:236,1177-1190`; `ReadyOrNotCharacter.cpp:565,1743-1755`; `ReadyOrNotGameState.h:203-222,357-370`.

TDD ghi bốn manager actor tự replicate và `bAlwaysRelevant = true`:

| Manager | Base | Định vị do TDD ghi |
|---|---|---|
| `AScoringManager` | `AInfo` | `Info/ScoringManager.cpp:105-106,123` |
| `AMissionPlanManager` | `AInfo` | `Info/MissionPlanManager.cpp:41-42,74` |
| `ATOCManager` | `AActor` | `Info/TOCManager.cpp:11-12,57` |
| `AAIFactionManager` | `AActor` | `Info/AIFactionManager.cpp:16-17` |

`HideCategories` của hai actor sau chỉ giấu nhóm thuộc tính trong editor, không vô hiệu rendering/collision. `AReadyOrNotGameState` replicate một số field như `NextHost`, `MigrationGUID`, `ScoringManager`; không vì vậy mà mọi field của nó cũng replicate (`ReadyOrNotGameState.cpp:56-62`). PvP dùng subclass `AReadyOrNotGameMode_PVP`, không gom mọi mode vào một nhánh runtime.

Nhóm `UTickableWorldSubsystem` gồm `USWATManager`, `UActivityManager`, `UCSGasManager`, `USuspectsAndCivilianManager`, `USoundManager`, `UReadyOrNotSignificanceManager`; `UMusicManager` là `UWorldSubsystem` thường. Chúng là `UObject` không tự replicate. PDF **suy luận** sự nhất quán client phải đến từ actor replicate mà chúng điều khiển. Nhóm `Subsystems/` còn có log, threat awareness, console multiplayer, PS5 activities, subtitles và achievement; vài lớp dùng `FTickableGameObject`.

## 2.7. Mở rộng và những khoản nợ cần nhận diện — PDF §2, tr. 12–14

Muốn thêm config, kế thừa `UGameplayConfig`, cung cấp tên file/section/fallback, tạo object và gọi `ReloadConfig()`. Muốn thêm dữ liệu designer, dùng asset hoặc row phù hợp. Muốn thêm registry thấy được ở client, theo mẫu `BeginPlay/EndPlay` của item/door. Muốn thêm manager, PDF đề nghị dùng subsystem theo vòng đời, chỉ dùng actor replicate khi **state của manager** cần tới client.

Các rủi ro khác của snapshot cần giữ đầy đủ khi học:

| Vấn đề PDF nêu | Ý nghĩa và giới hạn |
|---|---|
| 18 file `.cpp.bak` cạnh file thật | UBT không compile chúng, nhưng người đọc có thể sửa nhầm. Có bản lớn hơn file hiện hành, nên không chỉ là bản cũ hoàn toàn trùng. Trạng thái trong version control chưa xác minh |
| `PRAGMA_DISABLE_OPTIMIZATION` còn trong nhiều vùng | Inventory, command library, loadout V2 và chín console HUD file compile không tối ưu ở mọi cấu hình; có chạy mỗi frame hay không chưa xác minh |
| Private PCH gồm 49 include, hơn 30 header gameplay cụ thể | Sửa một header dễ gây rebuild rộng; PCH còn khai báo 15 log category |
| API key trong header | TDD ghi có key mod.io production/test trong source nhưng không công bố giá trị; chương không xác minh key còn hiệu lực và không tái bản giá trị |
| Public include trỏ vào Private của plugin | Phá ranh giới API của InputRemapping, làm nâng cấp khó hơn |
| Bốn kiểu manager cùng tồn tại | `AInfo`, `AActor`, subsystem, raw `UObject`/tickable; không có quy tắc lựa chọn thành văn |
| Hai utility library chồng trách nhiệm | Tra cứu khó, cast singleton trùng lặp; cần ranh giới theo miền chức năng |

**Định vị do TDD ghi:** `Components/InventoryComponent.cpp:1574-1618`; `lib/ReadyOrNotCommandFunctionLibrary.cpp:26-3921` và `.h:10-118`; `HUD/Widgets/Loadout/V2/Loadout_V2.cpp:18-1787`; `HUD/Widgets/Console/ItemWheel.cpp:7-652`; `ReadyOrNot.Build.cs:20-24`; `ReadyOrNot.h:12-74,141-152`. Với raw manager, TDD nêu `UModioManager`, `ULoadoutManager`, `UHostMigrationManager`, `UReadyOrNotBackend`; không có cơ sở để tự gán cùng chính sách mạng cho tất cả.

Khi làm dự án nhỏ, quy tắc dễ duy trì là world service dùng `UWorldSubsystem`, session service dùng `UGameInstanceSubsystem`; truyền world-context rõ ràng; utility chia theo domain. Tái tạo ý tưởng fallback, registry và chu kỳ VO; không cần tái tạo mọi khoản nợ lịch sử của cây lớn.

## Bài thực hành tái tạo — một dịch vụ cấu hình nhiệm vụ

**Bài tập đề xuất:** làm một phòng thử có ba giá trị tuning AI, hai map context và một chu kỳ ba voice take giả lập bằng nhãn text.

| Phần hợp đồng | Yêu cầu |
|---|---|
| Owner | Một service theo world giữ context; server áp dụng tuning gameplay; client chỉ đọc state cần hiển thị |
| State | `Unloaded → Loaded`; mỗi giá trị ghi lại nguồn `Map/Global/Default`; VO có hàng đợi chỉ số |
| Dependencies | INI loader, world-context rõ ràng, registry actor BeginPlay/EndPlay, log nguồn giá trị |
| Acceptance | Map thiếu key nhận Global; thiếu cả hai nhận default; key có khoảng trắng không đọc nhầm; đổi world không rò config; destroy actor xóa registry; ba take không lặp liền khi có nhiều take |

Chạy thêm hai world thử nghiệm và xác nhận mỗi world đọc đúng context. Làm một manager không replicate trước; chỉ thêm actor state khi client thật sự cần nhận dữ liệu. Như vậy quyết định kiến trúc có thể giải thích bằng nhu cầu quan sát cụ thể.

Đọc cùng [bản đồ source](../04-Kien-Truc-Va-Quy-Trinh/01-Ban-Do-Source.md), [asset, Blueprint và level](../04-Kien-Truc-Va-Quy-Trinh/02-Asset-Blueprint-Level.md), [multiplayer và state ownership](../04-Kien-Truc-Va-Quy-Trinh/03-Multiplayer-State-Ownership.md), [save/config/version](../04-Kien-Truc-Va-Quy-Trinh/04-Save-Config-Version.md). Tiếp theo là [player, movement và tương tác](03-player-movement-interaction.md).

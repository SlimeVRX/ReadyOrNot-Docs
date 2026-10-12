# 04 — Vũ khí, đường đạn, xuyên vật liệu, recoil và xác nhận hit

Chương này diễn giải **PDF §4, trang in 22–27**, bản R&D reverse-engineered độc lập ngày 27/09/2026; không phải TDD chính thức của VOID Interactive. Mọi hành vi nói về snapshot mà PDF khảo sát, không xác nhận bản retail mới nhất hoặc source hiện tại. `file:line` là **định vị do TDD ghi**, chưa được kiểm tra độc lập trong chương này. Nhãn “xác nhận trong PDF” chỉ nói mức bằng chứng tác giả nguồn ghi nhận; các suy luận và đường code vô hiệu vẫn được đánh dấu.

## 4.1. Một phát súng có nhiều lớp trách nhiệm — PDF §4, tr. 22–23

Vũ khí chọn hitscan hay physical projectile bằng `bHitScan` theo từng weapon, default false. Hitscan giải đường bắn tức thời bằng trace; projectile tạo actor chuyển động và va chạm. Hai đường dùng chung mô hình fire mode, ammo, spread và recoil, nhưng cơ chế xuyên/nảy và mạng không giống nhau. Khi học, đừng suy một công thức từ đường này sang đường kia.

Ammo được theo dõi **từng magazine**, không chỉ một kho tổng. `Magazines` chứa `FMagazine` với `uint16 Ammo`, `uint16 AmmoType`; `AmmoType` là chỉ số vào `AmmunitionTypes`. Cấu trúc hỗ trợ các magazine dùng loại đạn khác nhau. Mỗi loại đạn là row `FAmmoTypeData`, có damage, curve theo khoảng cách, số pellet, spread, penetration, ricochet, spalling, dismemberment, artery/hit reaction và multiplier từng limb. Fire mode có Single, Auto, Burst, Continuous; Safe còn trong enum nhưng đã deprecated.

| Lớp | Phần việc trong snapshot |
|---|---|
| `ABaseWeapon` | Chuyển fire mode, tra ammo row, recoil pattern/proc recoil, spread, ADS zoom, attachment |
| `ABaseMagazineWeapon` | Magazine/reload, fire RPC, hai đường đạn, heat, suppression, shell, hit validation; khoảng 3.700 dòng |
| `AProjectile`, `ADamageProjectile` | Movement/collision và damage plumbing; TDD chưa xác minh sâu hơn header |
| `ABulletProjectile` | Đạn vật lý, xuyên/nảy bằng sinh actor tiếp nối, lọc network relevancy |
| `UBulletProjectileMovementComponent` | Subclass projectile movement; triển khai chưa xác minh |
| `UAmmoComponent` | Resource component tổng quát, không phải hệ magazine đang mô tả |
| `UPenetrationData` | Khoảng 50 trường theo `EPhysicalSurface`, chọn bằng `GetPenetrationData` |
| `UHitRegistrationSettings` | `UDeveloperSettings(Config=Game)` cho validation và forgiveness |
| `UWeaponAttachment` | Multiplier recoil/spread và các biến thể scope/laser/light/magazine |
| `UPlayerEffect_ModifyRecoil` | Effect có thời gian, Modify/Add/Subtract proc recoil, lọc class hoặc weapon cụ thể |

Aim-assist plugin có `UUASAimAssistComponent`; `ABaseWeapon::AimAssistRotation` là `BlueprintReadWrite`. TDD không tìm thấy C++ writer, nên quan hệ qua Blueprint là **suy luận**, chưa phải toàn bộ đường gọi đã xác minh.

**Định vị do TDD ghi:** `Actors/BaseMagazineWeapon.h:28-51,252,335`; `Actors/BaseWeapon.h:24-158,290-291`; `Components/AmmoComponent.h:15`; `Data/PenetrationData.h:12-50`; `lib/HitRegistrationSettings.h:17-33`; `Enums.h:537-544`; `GameplayEffects/PlayerEffect_ModifyRecoil.h:10-22,95-120`.

## 4.2. Từ nhấn cò tới xác nhận phát bắn — PDF §4, tr. 23

`OnFire` từ chối khi animation-blocking hoặc `RefireDelayTimer > 0`. Nếu ADS qua scope có socket `CenterPoint`, hướng được chỉnh về điểm cách 2.000 cm trên trục scope; `AimAssistRotation` khác 0 có thể thay hướng đó. Hàm tạo seed bằng `FMath::Rand()`, client rút lại nếu seed đã có trong `ActiveProjectiles`.

Phía người chơi, `LocallySimulateFire` tăng `CurrentHeat` thêm `HeatPerShot` và clamp tới `HeatMax`; đặt `RefireDelayTimer = FireRate + RefireDelay`; lặp `SpawnProjectileCount` lần, mỗi pellet lấy seed riêng từ `FRandomStream(Seed)`. Với owner AI, hàm local thoát sớm sau broadcast `OnWeaponFire`. Seed chung giúp các phần có dùng seeded stream tái tạo cùng lựa chọn, nhưng không có nghĩa mọi random của spread đều đã deterministic.

Server chạy vòng spawn có authority, trả `Client_OnProjectileValidation(Validated)`, rồi mới `RemoveAmmo(1)`. Thứ tự này cần thiết vì bỏ đạn có thể làm đổi loại đạn đang chamber. Hết ammo trả `Invalid`. Tiếng bắn gửi tới AI hearing có loudness 2,5, range 2.000 cm hoặc 1.500 cm khi suppressed.

Trong hitscan, remote client gọi `SpawnProjectile(bLocalOnly=true)` rồi chạy `Multicast_PerformHitscan_Implementation` local để có phản hồi ngay. Server dùng `SpawnProjectile(false)` và gửi multicast. Client đã bắn bỏ qua multicast này để không mô phỏng cùng phát lần hai.

**Định vị do TDD ghi:** `Actors/BaseMagazineWeapon.cpp:639-782,884-962,1769-1770`. Các tên `FireRate`/`RefireDelay` phải được hiểu theo phép đặt timer ở đây; đừng tự diễn giải `FireRate` thành RPM nếu dữ liệu chưa chứng minh.

## 4.3. Spread và recoil không phải một phép cộng duy nhất — PDF §4, tr. 23, 25

`CalculateRecoil` thêm vào `PendingSpread`: `GetSpread()` nhân first-shot factor nếu có, rồi nhân các `SpreadMultiplier` attachment. `GetSpread` dùng rotator ngẫu nhiên đều **không seed** trong khoảng ±`SpreadPattern`. Khi spawn từng pellet, seeded stream tiếp tục tạo offset theo thứ tự:

1. Lấy ngẫu nhiên trong ±(`|PendingSpread|` + weapon `SpreadPattern`).
2. Thêm `SpreadPattern.X/Y` của ammo, trừ khi `bIgnoreAmmoTypeSpread`.
3. Nhân multiplier spread của từng attachment.
4. Thêm nhiễu ±(`2D owner speed × VelocitySpreadMultiplier`).
5. Nhân `ADSSpreadMultiplier` nếu ADS, rồi `FirstShotSpread` nếu first shot; roll đặt 0.

Thứ tự nhân/cộng làm khác kết quả, đặc biệt khi ammo spread, attachment và movement cùng hoạt động. AI không có `bAIFireAtBulletSpawn` dùng trace `CachedHitScanResult` thay điểm muzzle. Tái tạo đường AI cần kiểm tra riêng, không chỉ gắn súng player lên bot.

`GetRecoil` đi tuần tự qua `RecoilPattern`, hết thì quay lại 0. Với `IC_AssaultRifle`, yaw bị đặt 0 và có TODO yêu cầu xem lại. `CalculateRecoil` nhân first-shot factor rồi scale pitch bằng vertical attachment multiplier và `1 + clamp(RecentShotsFired, 0, AmmoMax) × RecoilMultiplierPitch`; yaw dùng nhánh horizontal tương ứng. `RecentShotsFired` reset sau `2 × FireRate` ở Single, `1.1 × FireRate` ở mode khác. Khi reset, `PendingRecoil` trả lại phần `RecoilReturnPercentage` của recoil tích lũy.

Proc recoil là lớp khác: `TriggerProcRecoil` chọn hướng translation lùi và rotation với `RecoilRandomness`; `ComputeProcRecoil` đẩy trong `RecoilFireTime`, cộng buildup tùy chọn, rồi damping về 0 bằng `RecoilDampStrength/RecoilBuildupDampStrength`. ADS scale lực đẩy và buildup, không scale damping. Tác động trực tiếp của offset này lên view/weapon mesh thay vì bullet direction được TDD ghi là **suy luận**.

| Default handling | Giá trị trong header |
|---|---|
| `ADSRecoilMultiplier`, `ADSSpreadMultiplier` | 1; 1 |
| `RecoilReturnRate`, `RecoilReturnPercentage` | 10; 0,9 |
| `FirstShotRecoil`, `FirstShotSpread`, `FirstShotResetTime` | 0,7; 0,1; 0,2 |
| `VelocitySpreadMultiplier`, `VelocityRecoilMultiplier` | 0,01; 0,005 |

**Định vị do TDD ghi:** `Characters/PlayerCharacter.cpp:3455`; `Actors/BaseMagazineWeapon.cpp:834-869,2621-2688`; `Actors/BaseWeapon.cpp:573-598,1065-1098,1212-1279`; `Actors/BaseWeapon.h:497-504,585-615`. Đây là default C++, không phải profile đã đo của mọi weapon.

## 4.4. Hitscan: hai trace và ngân sách xuyên — PDF §4, tr. 23–24

`Multicast_PerformHitscan_Implementation` trộn thêm origin/direction vào seed. Hai `LineTraceMultiByChannel` complex chạy trên `ECC_PROJECTILE`, dài 100.000 cm, tất cả channel được xử lý overlap: một trace xuôi, một ngược để lấy điểm thoát. Có cặp entry/exit mới ước lượng được độ dày, thay vì mặc định vật liệu nào cũng là tấm mỏng.

Ngân sách `MaxPenetrationDistance` lấy ammo `PenetrationDistance × 0.1`, chuyển **mm sang cm**. Nếu không có ammo type, fallback là weapon `PenetrationDistance × 2`. Tối đa `MaxBounces = 3`. Với surface cho ricochet, code phản xạ hướng và tính `EnergyModifier = clamp(1 − N·R)`, rồi rút xác suất có trọng số từ tích:

`RicochetChance × EnergyModifier × RicochetChanceMultiplier × (1 − spentBudget/Max)`.

Nếu nảy, spent budget tăng `Max × (1 − EnergyModifier)`. Pitch ra bị clamp 35°–85° so với normal, jitter từ −5° đến +20°, yaw jitter tối đa ±20°. Những hit sau bị bỏ và một cặp trace mới được nối vào. Như vậy đây là chuỗi đoạn đạn, không phải mọi hit của đường thẳng ban đầu vẫn tiếp tục hợp lệ sau khi nảy.

Với character, `ABaseArmour::CheckPenetration` quyết định theo bone; bone không armor luôn xuyên. Với surface khác cần `bIsPenetrable` và `PenetrationLevel >= ArmourLevel`. Chi phí tường là:

`(depth / 2) × entry PenetrationDensity + (depth / 2) × exit PenetrationDensity`.

Xuyên thất bại, ballistic shield hoặc thứ tự entry/exit sai đẩy spent budget lên `BIG_DIST`; quá ngân sách thì bỏ các hit còn lại. Bộ nhớ theo character tránh damage lặp trên cùng component hoặc bone group. Entry hit phát noise `"BulletHitSurface"`, tạo physics impulse theo `RagdollImpulseMultiplier`. Ngoài entry đầu và exit cuối, impact FX chỉ có 25% cơ hội spawn để giảm mật độ hiệu ứng.

**Định vị do TDD ghi:** `Actors/BaseMagazineWeapon.cpp:1763-2218`, nhất là `:1772-1812,1824-1833,1853-1924,1942-1990,2083,2192-2211`.

## 4.5. Damage, projectile và suppression — PDF §4, tr. 24

`ApplyHitscanDamage` lấy damage hoặc curve theo range của weapon, rồi ammo type có thể override. `Energy = (MaxPen − spent) / MaxPen`, với mẫu số tối thiểu 1 cm; dưới 0,1 thì energy thành 0. Khi shooter là suspect **hoặc weapon không có owner**, energy ≤0,9 khiến damage vào character bị cap 5 **trước** bước nhân energy. TDD giải thích đây là hạn chế suspect gây chết người qua tường; không phải lớp bảo vệ suspect khỏi người chơi.

Damage cuối được nhân energy và truyền `ApplyPointDamage` với `UBulletDamageType`, hoặc `ArmorPiercingDamageType` nếu weapon AP. Đừng trộn cổng xuyên armor với kết quả damage cuối: một hit có thể xuyên nhưng đã tiêu hao gần hết ngân sách.

Đường physical projectile dùng `ABulletProjectile::OnHit`:

| Trường hợp | Cơ chế nguồn mô tả |
|---|---|
| Ricochet | Cần góc hit vượt `RequiredAngleToDeflect = 60°`, qua chance check và `bCanRicochet`; actor nối tiếp có speed `InitialSpeed × SpeedLossMultiplierPerSurface × 0.5` |
| Pierce | Nếu `bIsPenetrable`, trace bước 1 cm tới khi ra khỏi solid; giới hạn bước dựa `PenetrationDistance × material PenetrationMultiplier`; tiếp tục với speed nhân `SpeedLossMultiplierPerSurface` |
| Respawn continuation | `Multicast_OnRespawnProjectile` sinh cùng class; penetration distance còn 0,75 lần cũ; đạn cũ tắt collision và sống thêm 3 s |
| Thiết lập từ weapon | AP gấp đôi penetration distance; speed/damage loss multiplier đặt 0,75 cho AP, 0,5 trường hợp khác; lỗi tiêu thụ damage loss được nêu phía dưới |

Suppression hitscan chạy một lần trước resolve hit, trên mỗi máy thực thi trace. AI khác team shooter phải có dot hướng 2D ≥0,85 và điểm gần nhất trên shot line trong 100 cm. Nếu AI đang track shooter hoặc có LOS, code tìm bone gần nhất trong 22 bone có tên, map strength từ 1,0 xuống 0,01 ở khoảng cách 0–100 cm và ghi `BoneSuppressionAmount`. Nếu không, gọi biến thể `_NoLineOfSight` với strength 0. Cả hai gọi `_Implementation` trực tiếp, không qua RPC; phản ứng AI sau đó thuộc §9.

**Định vị do TDD ghi:** `Actors/BaseMagazineWeapon.cpp:2337-2530,2730,2776-2777`; `Actors/Projectiles/DamageProjectiles/BulletProjectile.cpp:139-189,278-455`.

## 4.6. Reload và các nhóm dữ liệu mặc định — PDF §4, tr. 24–25

`FindNextMagIndex` ưu tiên magazine khác hiện tại có `DesiredAmmoType` và nhiều viên nhất; nếu không có thì chọn magazine khác có nhiều viên nhất bất kể loại. Normal reload và tactical reload đều gọi nó, rồi chọn animation/sound empty hay partial qua `GetAmmo() <= 0`. Chỉ normal reload từ chối khi `NextMagIndex == MagIndex && !bCanReloadSameMagazine`. Điều kiện này cần giữ nguyên khi kiểm thử cùng magazine và đổi loại đạn.

**Định vị do TDD ghi:** `Actors/BaseMagazineWeapon.cpp:1418-1642`.

| Dữ liệu | Default TDD ghi |
|---|---|
| `FAmmoTypeData`: damage và số đạn | `Damage=30`, `ProjectileCount=1`, `SpreadPattern=(0,0)`, `DurabilityDamage=1` |
| Penetration/ricochet ammo | `PenetrationLevel=0`, `PenetrationDistance=400 mm`, `RicochetChance=1.0` |
| Spalling và tổn thương | `SpallingDamage=10`, `SpallingRadius=50`, `DismembermentDamage=0`, `ArteryHitChance=0`, `HitsChance=0.2`, `ArmouredHitsChance=0`, `WoundSize=35` |
| Limb multiplier ammo | Head 2,0; Upper/Lower body 1,0; Arm/Leg 0,3; Hand/Foot 0,1; năm ragdoll impulse field đều 3.000 |
| `FMaterialPenetration` | `bIsPenetrable=false`, `PenetrationDensity=1`, `PenetrationMultiplier=1`, `ArmourLevel=0`, `bCanRicochet=true`, `RicochetChanceMultiplier=1`, `SpallingChance=0` |
| Weapon penetration/heat | `PenetrationDistance=20 cm`; `HeatPerShot=0.4`, `HeatThreshold=1`, `HeatMax=2`, `HeatDissipation=0.2` |
| Weapon magazine/khác | `MagazineCountDefault=4`, Min=1, Max=1; `LockIntegrityMin/MaxDamage=0`; `GunshotFullOcclusionDepth=150`; `bNoLocalProjectile=false`; `SpawnProjectileCount=1` |
| Projectile | `RequiredAngleToDeflect=60`, `PercentageToDeflect=0.5`, `DeflectionAmount=90`, `SpeedLossMultiplierPerSurface=0.5`, `DamageLossMultiplierPerSurface=0.5`, `VelocityRequiredToRespawn=5000` |

`PenetrationMultiplier` của material chỉ dùng cho projectile và là `VisibleAnywhere`. Giá trị `MagazineCountDefault/Min/Max` khác nhau được giữ đúng như PDF, không tự “sửa hợp lý”. Loadout catalog `UItemData` chứa danh sách `FWeaponData` primary/secondary/PvP, mỗi entry tham chiếu Blueprint `TSubclassOf<ABaseWeapon>`; catalog **không chứa ballistics**.

**Định vị do TDD ghi:** `Actors/BaseWeapon.h:24-158,576`; `Actors/BaseMagazineWeapon.h:317,373-376,524,673-715,764`; `Data/PenetrationData.h:12-44`; `Actors/Projectiles/DamageProjectiles/BulletProjectile.h:85-110`; `Data/ItemData.h:12-33,215-225`.

## 4.7. Hit registration: client báo, server kiểm tra — PDF §4, tr. 25–26

Projectile remote client tạo đạn local không replicate, trừ khi `bNoLocalProjectile`. Server tạo bản replicate nhưng `IsNetRelevantFor` loại instigating controller; owning client không cần nhận thêm một đạn hiển thị trùng. `Client_OnProjectileValidation` đánh dấu bản dự đoán valid/invalid. `Multicast_OnFire` mang cosmetic event; `Multicast_SimulateFireForViewTargets` phục vụ spectator.

Hitscan chia quyền damage cụ thể hơn:

| Máy thực thi trace | Được làm gì |
|---|---|
| Server | Damage non-character trực tiếp; character chỉ trực tiếp khi shooter locally controlled tại server, tức listen host hoặc AI |
| Remote owning client | Gửi `Server_HitscanHit(HitResult, SynchronizedTime, TraceStart, TotalDistance, Penetration, AmmoTypeIndex)` kèm `PredictHitEffects`; server xử lý damage character sau validation |
| Client khác | FX và suppression từ trace; không quyết định damage character |

Server lưu một snapshot character mỗi tick, tối đa `MaxSnapshots = 48`, bỏ cũ nhất khi đầy. Snapshot bắt đầu từ mesh bounds, kéo về hướng `centre + velocity × SnapshotVelocityFactor`, nới `HitscanForgiveness` và bảo đảm tối thiểu `MinimumSnapshotExtent`. 48 là **số mẫu**, không phải số giây lag được bù cố định; thời gian phủ phụ thuộc tick.

`UHitRegistrationSettings` default: `bEnableValidation=true`, `HitscanForgiveness=50`, `ShooterForgiveness=250`, `TraceDistanceForgiveness=5`, `SnapshotVelocityFactor=1.0`, `MinimumSnapshotExtent=(75,75,90)`. Thuật toán server:

1. Resolve ammo index client khai; index sai thì bỏ hit.
2. Có thể bỏ validation nếu setting tắt hoặc CVar `a.RonForceDisableHitValidation` bật.
3. Từ chối nếu khoảng cách origin–impact trừ forgiveness lớn hơn total distance được khai.
4. Yêu cầu **mọi** shooter snapshot tại hoặc trước thời gian khai, sau khi nới `ShooterForgiveness`, đều chứa trace origin.
5. Lấy cặp target snapshot mới nhất kẹp thời gian khai, nội suy box và chấp nhận nếu impact nằm trong box.

Mỗi lần từ chối ghi lý do qua `LogReadyOrNotHitRegistration`; `Client_HitscanDebug` vẽ box ở non-shipping. Các CVar như `a.RonDrawBallisticsDebug`, `a.RonNoBullets`, `a.RonServerNoProjectile` được khai báo không điều kiện; phần lớn nơi đọc có guard shipping, nhưng bypass hit validation không có guard đó. Đây là cơ chế server-side, không phải quyền client tự tắt validation.

PDF còn mô tả nhánh anti-piracy: khi `UReadyOrNotGameInstance::bIsBuildPirated` bật, fire loop truyền `FakeBulletProjectile`; spawn tắt hitscan và tạo `ABulletProjectile` damage 0, speed 10.000, không attach/destroy on hit. Đây chỉ là mô tả nhánh trong snapshot, không suy ra trạng thái của bất kỳ bản cài người dùng nào.

**Định vị do TDD ghi:** `Actors/BaseMagazineWeapon.cpp:56-63,726-733,871-882,907-910,2093-2106,2220-2319,2690,2708-2739`; `Actors/Projectiles/DamageProjectiles/BulletProjectile.cpp:457-460`; `ReadyOrNotCharacter.cpp:599-604,9074-9110`; `ReadyOrNotCharacter.h:2464`; `lib/HitRegistrationSettings.h:17-33`.

## 4.8. Điểm mở rộng và các lỗi không nên sao chép — PDF §4, tr. 26–27

Ammo mới thêm row `FAmmoTypeData` vào `AmmoDataTable` và tên vào `AmmunitionTypes`, không cần đổi code. Weapon mới dùng Blueprint subclass `ABaseMagazineWeapon`, đặt `bHitScan`, class projectile thường/AP, recoil/spread/fire-rate/pellet rồi thêm catalog `FWeaponData`. Attachment mới dùng `UWeaponAttachment` hoặc subclass và `AddAttachment/CanAddAttachment`; tên property từng slot **chưa được TDD kiểm tra lại**. Material mới cần thêm trường và `case` C++ trong `UPenetrationData`, khác với ammo data-only. Effect recoil có thể dùng `UPlayerEffect_ModifyRecoil` với Modify/Add/Subtract và `WeaponFilter`/`bApplySpecific`.

**Định vị do TDD ghi:** `Actors/BaseWeapon.cpp:655-874`; `Data/PenetrationData.cpp:6-118`; `GameplayEffects/PlayerEffect_ModifyRecoil.h:10-22,95-120`.

Những lỗi sau được PDF gọi là xác nhận từ code, trừ nơi ghi rõ suy luận:

| Vấn đề | Cơ chế và giới hạn |
|---|---|
| First-shot bonus luôn bật | `bFirstShot` default true; cả `TriggerFirstShot` và `ResetFirstShot` đều đặt true, không có nơi clear. Với giá trị mặc định, mọi phát nhận hệ số recoil 0,7 và spread 0,1; cần đối chiếu asset/profile để biết giá trị thực tế. Spread factor xuất hiện ở cả CalculateRecoil lẫn SpawnProjectile, kể cả pellet shotgun. Last-shot counterpart bị comment |
| Chance nảy projectile thành tung đồng xu | `FMath::RandRange(0,1)` chọn overload integer, chỉ trả 0/1; threshold trong `(0,1]` cho 50%. Exit loop còn trace lặp cùng đoạn 1 cm thay vì tiến. Hitscan dùng weighted roll đúng kiểu |
| Projectile nối tiếp không giảm damage | `OnHit` truyền `NewDamage`, nhưng respawn không đọc, sao chép damage cha. Speed và penetration giảm; damage-loss AP/thường không có hiệu quả ở đường này |
| Hitscan spalling không chạy | `bShouldSpall=false`; weighted roll bị comment dưới `##UE5.3UPGRADE##`. Dữ liệu spalling tồn tại không chứng minh cơ chế hoạt động |
| Trộn ammo khi tính damage | Base damage lấy `ShotAmmoType`, curve và MaxPenetration lấy `CurrentAmmoType`. Code mix được ghi nhận; tác động khi server đã đổi ammo trước validated hit là suy luận |
| Bypass validation trong shipping | CVar được đọc không có shipping guard; rủi ro được PDF xếp thấp và giới hạn ở server cấu hình sai |
| Kiểm tra origin quá chặt | Mọi snapshot cũ phải chứa origin nới 250 cm; teleport/vehicle có thể làm mất hit hợp lệ. Đây là suy luận; box nới velocity che phần lớn movement bình thường |
| Safe mode deprecated | `SafeModeToggle` return ngay; `NextFireMode` vẫn có đường rời `FM_Safe` qua `FiremodeBeforeSafe`. UI để nút safe có thể không làm gì |
| File backup cạnh source | Năm `.bak`, gồm `BaseMagazineWeapon.cpp.bak` và `WeaponAttachment.cpp.bak`; tránh đọc/sửa nhầm |

**Định vị do TDD ghi:** `Actors/BaseWeapon.h:626`; `Actors/BaseWeapon.cpp:395-445,1021-1024,1047-1052`; `Actors/Projectiles/DamageProjectiles/BulletProjectile.cpp:139,158,364,381-393`; `Actors/Projectiles/DamageProjectiles/BulletProjectile.h:104-106`; `Actors/BaseMagazineWeapon.cpp:2117-2122,2241,2355-2363`.

Khi tự làm, giữ ammo row tách weapon, bảng material chung và ngân sách depth × density. `TMap<EPhysicalSurface, FMaterialPenetration>` tránh thêm field/switch cho mỗi material. Tách fire, recoil, reload và hit registration thành trách nhiệm rõ ràng; kiểm thử state flag đổi giá trị và continuation tiêu thụ tham số. Nếu ricochet không phục vụ trụ cột gameplay, có thể bắt đầu chỉ với penetration budget.

## Bài thực hành tái tạo — bắn qua hai tấm vật liệu trong co-op

**Bài tập đề xuất:** hai ammo row tự tạo, hai tấm có density/độ dày khác nhau, target có hit box, một weapon hitscan và một chế độ projectile tối giản.

| Phần hợp đồng | Yêu cầu |
|---|---|
| Owner | Server giữ magazine, damage và snapshot; client dự đoán muzzle/recoil/impact rồi nhận kết quả xác nhận |
| State | Ready/Refire/Reload, ammo type của **phát bắn**, shot seed, first-shot flag, spent penetration budget, hit accept/reject |
| Dependencies | Ammo table, material map, entry/exit trace, clock đồng bộ cho hit claim, bounded snapshot buffer, reason log |
| Acceptance | Không damage đôi; đổi magazine sau bắn không đổi ammo của hit đang chờ; cùng seed và đầu vào cho kết quả trace nhất quán; hai tường tiêu đúng ngân sách; first-shot clear sau phát đầu; projectile nối tiếp nhận damage mới |

Kiểm tra host và remote client riêng, rồi thêm latency mô phỏng. Log shot ID, ammo row tại lúc bắn, origin, thời gian, energy và lý do reject. Với xác suất, dùng nhiều mẫu và kiểm tra nhiều threshold; chỉ thử 0,5 sẽ không phát hiện lỗi mọi threshold đều trở thành tung đồng xu.

Đọc cùng [fire, ammo, chamber](../03-Gun-Gameplay/03-Fire-ammo-chamber.md), [ballistics và penetration](../03-Gun-Gameplay/04-Ballistics-penetration.md), [damage, armor và reaction](../03-Gun-Gameplay/05-Damage-armour-reaction.md), [reload interruption](../03-Gun-Gameplay/08-Reload-interruption.md), [network authority](../03-Gun-Gameplay/09-Network-authority.md) và [tái tạo, hiệu chuẩn](../03-Gun-Gameplay/10-Tai-tao-va-hieu-chuan.md).

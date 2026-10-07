# 10 — Tái tạo bằng các lát cắt có thể giải thích và đo

[Trước: Network](09-Network-authority.md) · [Mục lục](README.md)

Mục tiêu học tập tốt không phải đọc hết source trước rồi mới làm. Hãy dựng một lát cắt nhỏ chạy được, biết chính xác dữ liệu nào quyết định hành vi, đo nó với bản gốc rồi mở rộng. Chương này là **kế hoạch thực hành đề xuất**; các mốc hoàn thành bên dưới chỉ được đánh dấu sau khi bạn có artifact và bằng chứng tương ứng.

## Tầng kiến thức và sản phẩm phải làm được

| Tầng | Kiến thức cần có | Sản phẩm thực hành | Điều kiện qua tầng |
|---|---|---|---|
| 0 — Đọc dự án | Module, class, header/cpp, Blueprint CDO, asset reference | Bản đồ một weapon và các dependency | Tìm đúng owner của 10 property quan trọng |
| 1 — Vòng đời | Pawn/controller, actor owner, component, inventory | Spawn → add inventory → equip → remove | Không có item orphan, state cầm đúng |
| 2 — Input và state | Action binding, guard, timer, enum/state | Single/auto/burst + dry fire | Đếm đúng shot và ammo theo event |
| 3 — Ballistics | Vector/rotation, trace, collision channel, surface | Bia hit, spread, penetration có kiểm soát | Giải thích từng hit bằng dữ liệu đầu vào |
| 4 — Animation | Skeleton/socket, montage, notify, FP/TP | Reload có commit và interruption | Count đổi đúng event, pose đúng ca |
| 5 — Feel | Camera rotation/FOV, interpolation, viewmodel, audio/VFX | Một súng có đường đáp ứng đo được | So được curve và video cùng điều kiện |
| 6 — Dữ liệu nội dung | CDO, DataTable, ammo rows, attachment compatibility | Catalog class và profile tái hiện được | Một profile load lại cùng cấu hình |
| 7 — Network | Authority, RPC, replication, prediction, snapshots | Host/client gun loop | Local response và server health nhất quán |
| 8 — Gameplay đầy đủ | AI perception, reactions, health/armour, rules | Ca encounter nhỏ có objective | Phát bắn tác động cả hệ thống đúng kỳ vọng |

Các tầng dựa trên phụ thuộc thực tế đã phân tích: native equip đặt ownership; fire chia local/server; hitscan đọc material/ammo; notify thay magazine; camera và sound được gọi từ callback. Nguồn nền tảng nằm tại P01–P05. Thứ tự học là lựa chọn sư phạm, không phải lịch phát triển của studio.

## Hai sản phẩm khác nhau nên tồn tại cạnh nhau

**Gun Lab trong project hiện có:** giữ pawn, native weapon class, animation data, sound data, ammo và attachment của bản source. Nó trả lời “hành vi hiện tại là gì?” và cho một chuẩn đối chiếu gần nguồn nhất có thể. Harness chịu trách nhiệm chọn/cấp súng, reset test và hiển thị thông tin; không tự thay recoil/trace nếu mục tiêu là giữ cảm giác gốc.

**Prototype học tập độc lập:** tự viết từng phần bằng asset bạn có quyền dùng; ban đầu dùng mesh đơn giản cũng được. Nó trả lời “mình có thể giải thích và xây lại cơ chế này không?”. Một prototype có thể dễ đọc hơn source lớn mà vẫn giúp bạn hiểu các quyết định thiết kế. Không coi việc nạp toàn bộ asset gốc vào project khác là đã hiểu kiến trúc.

Các nhận định về reuse là **đề xuất** dựa trên chuỗi gọi native ở P01–P03. Chúng không phải chứng nhận rằng Gun Lab hiện đã đạt parity mọi weapon. Đọc trạng thái kiểm chứng riêng của phần lab để biết cái gì thực sự chạy.

## Chọn một súng chuẩn trước khi mở rộng catalog

Chọn generated class player có FP animation, ammo row và sound data load được. Bắt đầu single fire, đứng yên, hip, không attachment bổ sung; sau đó ADS, sustained fire, reload còn đạn, reload rỗng, đổi ammo type và attachment. Với shotgun/taser/pepperball/launcher, lập profile riêng vì native subclass có override state/fire/reload. [P06]

| Profile đối chiếu | Những trường phải cố định |
|---|---|
| Nội dung | Class path, ammo row, attachment classes, asset revision |
| Player | Pawn class, stance, lean, aim assist, movement, health/armour |
| Hình ảnh | Resolution, aspect ratio, camera FOV, weapon FOV, graphics/frame pacing |
| Môi trường | Map, vị trí và hướng spawn, ánh sáng, indoor/outdoor, target material |
| Thực thi | Standalone/host/client, build config, engine version, source snapshot |
| Đầu vào | Input sequence, thời điểm press/release, mouse movement |

Đây là schema profile đề xuất. Nếu hai video khác FOV hoặc attachment, chúng chưa phải phép so recoil công bằng.

## Những phép đo ưu tiên

| Nhóm | Đại lượng | Cách thu thập đề xuất | Điều không nên kết luận |
|---|---|---|---|
| Fire | Shot interval, shots per press, ammo consumed | Event timestamp và server ammo | Âm nghe nhanh nghĩa fire rate đúng |
| Recoil | Pitch/yaw curve, peak, recovery | Control rotation theo frame + mouse input | Pixel sight jump bằng recoil ballistic |
| Spread | Phân bố hit theo shot/pellet | Điểm trúng mặt phẳng ở range cố định | Một loạt 5 viên đủ mô tả mọi randomness |
| ADS | Thời gian tới pose/FOV ổn định | State và camera/pose capture | Montage duration bằng ADS usable time |
| Reload | Request/commit/end, mag retention | Notify/event log + inventory snapshot | Animation đẹp nghĩa ammo state đúng |
| Ballistics | Entry/exit, budget, damage trước/sau vật cản | Physical surface + hit/damage log | Decal mặt sau chứng minh target nhận damage |
| Presentation | Audio onset/tail, muzzle/shell, montage | Video/audio đồng bộ cùng shot log | Bất kỳ flash vắng đều là lỗi |
| Network | Local response, accepted/rejected hit, corrections | Log gắn seed/shot ID theo từng role | Host pass nghĩa client pass |

Đây là kế hoạch đo, không phải các phép đo đã được thực hiện. Chọn trước một tolerance cho mỗi metric dựa trên mục tiêu bài học; không tự gán mức “giống 95%” khi chưa có baseline và định nghĩa sai số.

## Các công cụ debug đã có trong source

File magazine weapon khai báo các console variable sau. Những nhánh vẽ được đọc ở đây có guard build phát triển; bảng là điểm bắt đầu để thử trong Editor/Development, không phải cam kết lệnh sẽ vẽ trong Shipping. [P08]

| Lệnh quan sát | Mục tiêu |
|---|---|
| `a.RonDrawAmmoDebug 1` | Hiển thị ammo và ammo type của item đang equip |
| `a.RonDrawPenetrationDebug 1` | Xem physical material, density/armour level và budget xuyên ở hướng muzzle |
| `a.RonDrawBallisticsDebug 1` | Vẽ đường/hit trong nhánh ballistics hitscan |
| `a.RonDrawHitRegistration 1` | Xem box/impact debug của xác nhận hit client |

Đặt lại `0` sau ca đo để ảnh đối chiếu không bị debug che. Không dùng các cvar thay đổi luật xử lý đạn/damage/validation cho baseline parity; nếu cố ý thử một biến thể debug, ghi tên và giá trị trong profile. Đây là quy ước đo đề xuất.

## Một buổi thực hành hoàn chỉnh

1. Lưu profile và snapshot source/asset đang dùng.
2. Chạy baseline native bằng class thật trong lab, ghi đầy đủ log lỗi load asset.
3. Bắn đơn khi đứng yên, không di chuột; lấy control rotation và impact.
4. Bắn loạt bằng input có thời điểm xác định; đếm server shot và interval.
5. Reload còn đạn, reload rỗng, đổi loại; kiểm tra chamber/queued ammo.
6. Lặp hip/ADS và cùng một attachment thay đổi.
7. Với projectile/shotgun/special weapon, chạy thêm ca riêng.
8. Chạy lại cùng profile ở prototype; thay đúng một tham số/lớp cơ chế mỗi lần.
9. Lưu cả kết quả khớp và sai, cùng giả thuyết cần kiểm chứng tiếp.

Nếu một generated class không có FP anim hoặc dependency không load, ghi `blocked_content`/`runtime_failed` cùng nguyên nhân, không thay bằng một asset khác rồi giữ tên súng cũ trong kết quả. Nếu chưa có sound, công bố rõ bài đo đang giới hạn ở cơ học/hình ảnh.

## Hợp đồng event cho prototype của bạn

Một schema sự kiện tự thiết kế có thể chứa `run_id`, `shot_id`, `role`, `time`, `weapon_class`, `ammo_type`, `ammo_before`, `ammo_after`, `fire_mode`, `origin`, `direction`, `seed`, `hit_actor`, `bone`, `surface`, `raw_damage`, `final_damage`. Chỉ điền trường có dữ liệu thật; giá trị chưa quan sát dùng null/unknown.

Đây là thiết kế học tập, không phải dump source hoặc một API sẵn có của game. Trong mạng, shot ID nên liên kết được các sự kiện cùng phát giữa máy; trong shotgun thêm pellet index. Khi bắn nhiều viên, phân biệt log một phát, một projectile, một hit và một damage event.

## Bốn thói quen để đọc dự án lớn hiệu quả

**Đọc từ hành vi quan sát được.** Chọn “đổi ammo khi còn chamber” rồi tìm input, state mutation, notify, replication; tránh đọc file lớn từ đầu tới cuối không có câu hỏi.

**Tìm caller và consumer.** Một property tồn tại chưa chứng minh có tác dụng. Ví dụ `RecoilInterpSpeed` được đọc vào một biến nhưng nhánh camera apply đang dùng hằng khác; `bHitScan` quyết định liệu movement-speed của projectile có nằm trên đường đang chạy. [P03, P07]

**Ghi bằng chứng và mức chắc chắn.** Source path + symbol + line; CDO export; runtime trace; video. Mỗi loại chứng minh một điều khác nhau. Các chương này cung cấp phần source, còn kết quả runtime phải có báo cáo riêng.

**Xây lát cắt dọc.** Một súng đầy đủ equip → fire → hit → reload → sound → client đáng học hơn 100 mesh chỉ đổi được bằng menu. Sau khi lát cắt ổn, catalog và nội dung mở rộng mới có nền tảng đo lường.

## Definition of done cho mục tiêu gun gameplay

- Mọi class ứng viên trong catalog có trạng thái phát hiện/load/equip/fire/reload riêng.
- Class được đánh dấu playable phải chạy qua native player pipeline với dependency hợp lệ.
- Có ít nhất một profile mỗi họ cơ chế: magazine thông thường, shotgun và các lớp đặc biệt có trong catalog thực tế.
- Mỗi profile có source/asset identity, cấu hình, input sequence và kết quả đo.
- Mọi sai khác còn lại được mô tả bằng hệ thống và bằng chứng, không bị che bằng một điểm “cảm giác giống”.

Đó là tiêu chí đề xuất cho việc học và tái tạo. Khi chưa đạt một mục, tài liệu phải nói rõ còn thiếu bước nào. Bộ source lớn trở thành một hệ thống bạn có thể kiểm chứng từng phần, thay vì một hộp đen phải nhớ hết.

## Bằng chứng nền cho lộ trình

| ID | Đường dẫn và symbol | Dòng |
|---|---|---|
| P01 | `Ready Or Not/Source/ReadyOrNot/Characters/ReadyOrNotPlayerController.cpp` — `Server_Equip_Implementation`; `Ready Or Not/Source/ReadyOrNot/Components/InventoryComponent.cpp` — `AddInventoryItem` | 4618–4633; 1294–1321 |
| P02 | `Ready Or Not/Source/ReadyOrNot/Characters/PlayerCharacter.cpp` — `PrimaryUse`, `OnEquippedWeaponFire` | 5354–5515, 3355–3397 |
| P03 | `Ready Or Not/Source/ReadyOrNot/Actors/BaseMagazineWeapon.cpp` — `SpawnProjectile`, hitscan material handling | 2621–2740, 1763–2000 |
| P04 | `Ready Or Not/Source/ReadyOrNot/Animation/Notifies/AnimNotify_NextMag.cpp` — `Notify`; `Ready Or Not/Source/ReadyOrNot/Actors/BaseMagazineWeapon.cpp` — `Server_NextMagazine_Implementation` | 6–17; 1318–1391 |
| P05 | `Ready Or Not/Source/ReadyOrNot/Actors/BaseMagazineWeapon.cpp` — local/server fire, sound routing | 639–781, 884–961, 3013–3060 |
| P06 | `Ready Or Not/Source/ReadyOrNot/Actors/Items/Shotgun.cpp` — `GetAmmo`, `PlayReloadLoop`; `Ready Or Not/Source/ReadyOrNot/Actors/Items/Taser.cpp` — `Server_OnFire_Implementation`; `Ready Or Not/Source/ReadyOrNot/Actors/Items/PepperballGun.cpp` — `GetAmmo`, `RemoveAmmo`; `Ready Or Not/Source/ReadyOrNot/Actors/Items/GrenadeLauncher.cpp` — `FullySimulateGrenadePath` | 138–173, 421–459; 311; 56–65; 106 |
| P07 | `Ready Or Not/Source/ReadyOrNot/Characters/PlayerCharacter.cpp` — `RecalculatePendingRecoil`, `ApplyRecoil` | 3443–3459, 3666–3702 |
| P08 | `Ready Or Not/Source/ReadyOrNot/Actors/BaseMagazineWeapon.cpp` — console variable declarations và debug drawing branches | 56–60, 334–397, 1842–1850, 2321–2335 |

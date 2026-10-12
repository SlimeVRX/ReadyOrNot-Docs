# 05. Sát thương, sức khỏe, giáp, choáng và bắt giữ

> Bản học tiếng Việt diễn giải §5, trang in 28–34 của tài liệu R&D phân tích ngược đề ngày **27/09/2026**. Đây không phải TDD chính thức của VOID, cũng không xác nhận hành vi bản thương mại hiện tại. Các `file:line` dưới đây là **TDD locator**, được chép làm chỉ dẫn tra cứu, chưa được chương này kiểm chứng độc lập. Giá trị C++ có thể bị dữ liệu ghi đè; “suy luận”, “chưa kiểm chứng” giữ nguyên mức tin cậy của nguồn.

## Một phát đạn thay đổi những gì?

Nhân vật có một quỹ `Health` chung và năm vùng chi thể theo dõi riêng: đầu, hai tay, hai chân. Bắn vào tay vẫn trừ máu chung; vị trí trúng quyết định hệ số sát thương, vé chịu đòn của chi thể, phản ứng đau và khả năng được giáp che. Vì vậy, không nên hình dung năm thanh máu độc lập quyết định cái chết. TDD mô tả không có hồi máu tự động, nhưng thao tác tự băng bó có thể nâng máu lên một ngưỡng. “Không hồi tự động” không đồng nghĩa “băng bó chỉ dừng chảy máu”. [TDD §5, tr.28–30; TDD locator: `Components/CharacterHealthComponent.h:119-148`, `Components/BleedComponent.cpp:153-168`.]

`AReadyOrNotCharacter` nhận sát thương và cung cấp các hàm có thể ghi đè. `APlayerCharacter` xử lý người chơi; `ACyberneticCharacter` xử lý AI. `UResourceComponent` giữ tài nguyên hiện tại/tối đa, ngưỡng thấp và sự kiện cạn. `UCharacterHealthComponent` bổ sung chi thể, trạng thái bất tỉnh chiến đấu và hồi sinh. Giáp thực dùng đi qua `ASWATArmour`, `AHeadwear`, `ASuspectArmour`; component có tên `UArmourResourceComponent` không có nơi tham chiếu trong C++ được TDD khảo sát. Tên lớp có vẻ phù hợp chưa đủ chứng minh nó tham gia runtime. [TDD §5, tr.28–30; TDD locator: `Components/ResourceComponent.h:287-299`, `Components/ArmourResourceComponent.h:45-60`.]

Luồng server gồm: kiểm tra có được nhận sát thương → loại bone bị miễn và bone con → gọi `OnTakeDamage` với lượng sát thương truyền bằng tham chiếu → phân nhánh đạn hoặc choáng. Nhánh đạn xử lý hệ số vùng và giáp khi đi tới `ApplyDamageToBone`; nhánh choáng chạy hook riêng. Trở về `TakeDamage`, chỉ gọi `DecreaseHealth` một lần nếu hook trả `true`. Hệ số vùng và giáp sửa cùng lượng sát thương cuối, nên thực sự tới phép trừ máu; ngoại lệ projectile return sớm được giải thích ngay dưới đây. Đây là điều bản TDD đã sửa so với nhận định sai trong bản nháp. [TDD §5, tr.29,33; TDD locator: `ReadyOrNotCharacter.cpp:6195-6253,6500,6562-6661`.]

## Hệ số đạn và vé chi thể

Với người chơi, `GametypeDamageModifier` áp dụng trước. Nguồn gây sát thương là `ABulletProjectile` còn có roll `DamageSeverityChance`, nhân ngẫu nhiên trong `[1, DamageSeverityMultiplier]`; AI có damage cap mặc định 60 ở nhánh projectile và 70 ở nhánh point-hit. TDD suy luận từ control flow rằng nhánh projectile return trước xử lý bone/giáp; việc projectile thực tế có được truyền làm causer ở runtime chưa kiểm chứng. Không được biến kết luận có điều kiện này thành “mọi viên đạn bỏ qua giáp”. [TDD §5, tr.29,33; TDD locator: `Characters/PlayerCharacter.cpp:2638-2639,2847-2904`.]

| Vùng trúng | Mặc định `FAmmoTypeData` |
|---|---:|
| Đầu | ×2.0 |
| Thân trên / thân dưới | ×1.0 / ×1.0 |
| Tay / bàn tay | ×0.3 / ×0.1 |
| Chân / bàn chân | ×0.3 / ×0.1 |

Đây là mặc định, chưa đọc từng ammo row. Nếu causer không phải `ABaseWeapon`, các hệ số giữ 1.0. Bàn tay dùng hệ số bàn tay nhưng tính vé vào tay. Những field trông tương tự trong `UBulletDamageType`, như đầu ×3.0, không có nơi đọc C++ được TDD tìm thấy. [TDD §5, tr.29; TDD locator: `Actors/BaseWeapon.h:124-142`, `DamageTypes/BulletDamageType.h:32-48`.]

Mỗi `FLimbHealthData` mặc định có Health/MaxHealth 100, limit 1000, ngưỡng thấp 0.25, `LimbDamageMultiplier=1.25`, `MaxTickets=4`, `MaxLimbHealthHalving=3`. Nhưng đường đạn chủ yếu trừ **một vé mỗi hit**, không đi qua phép giảm máu chi thể có hệ số 1.25. Hết vé phát `OnLimbBroken`; tay người chơi còn nhận hiệu ứng tăng bất lợi recoil, montage đau và rung camera. Các hậu quả gameplay khác của gãy chi chưa kiểm chứng. [TDD §5, tr.29; TDD locator: `Components/CharacterHealthComponent.cpp:1263-1268,1318-1325`, `Characters/PlayerCharacter.cpp:3005-3081`.]

Không thể lấy `ApplyHeadDamage` lớp base để kết luận headshot luôn giết ngay: cả người chơi lẫn AI đều ghi đè. Mũ có thể giảm sát thương và chặn mất vé đầu. Người chơi không bị ép chết trong override; mức chết phụ thuộc máu sau hệ số. Ngoại lệ rõ trong nhánh AI là headshot `UBeanbagDamageType` khi không có giáp đầu, có lệnh giết bắt buộc. [TDD §5, tr.29–30; TDD locator: `Characters/PlayerCharacter.cpp:2960-2993`, `Characters/CyberneticCharacter.cpp:1984-2015`.]

## Giáp là tấm che có độ bền

Trúng thân gọi `ApplyBodyDamage` của lớp cụ thể rồi `HandleDamage`. Đạn `bIgnoresArmour` bỏ qua toàn bộ giáp. Các đạn khác trừ durability của tấm bằng `DurabilityDamage × damage factor`. Nếu tấm còn bền, hoặc vật liệu tắt durability, và `PenetrationLevel < ArmourLevel`, dùng `DamageReduction`; ngược lại dùng `MinDamageReduction`. Sát thương nhân với giá trị đã clamp của `1 − reduction`. Bị xuyên không nhất thiết mất mọi giảm sát thương. Vùng phủ trước/sau/hông quyết định dùng tấm hay carrier. Trait `Armorer` cộng 0.05 reduction và nhân durability ×1.5. [TDD §5, tr.30; TDD locator: `Actors/SWATArmour.cpp:85-149,189-199`.]

Công thức phần trăm resistance của `UArmourResourceComponent`, với mặc định 30% và một ticket, thuộc hệ khác không nằm trong luồng trên. Khi tái tạo, nên chọn một mô hình thống nhất thay vì đưa cả hai vào rồi hy vọng chúng bổ sung nhau.

## Gục, chảy máu và băng bó

Khi bật `bEnableIncapacitation`, max health nhân `IncapacitationHealthMultiplier`, mặc định 2.0. Máu còn dương nhưng xuống tới `Max/multiplier` chuyển `HS_Incapacitated`; về 0 chuyển `HS_Dead`. Nếu cho phép revive, sát thương tiếp theo khi máu đã 0 đi vào revive pool. Mặc định: 4 lần hồi, `ReviveTime=20 s`, giảm mỗi lần `5 s`, thao tác `5 s`, `MaxReviveHealth=150`; còn phải được `bRevivesAllowed` của game state cho phép. [TDD §5, tr.30; TDD locator: `Components/CharacterHealthComponent.h:647,668-681`, `.cpp:1196-1219`.]

Chảy máu động mạch của NPC là cơ chế riêng: bỏ qua SWAT, chỉ thử khi chưa có artery hit trước và chưa gib chi nào; roll `ArteryHitChance`, kiểm bone rồi giao đường bắn với cầu `ZoneSize=10`. Thành công đặt timer `DeathTime=5 s` dẫn tới cạn máu. Đây không phải bleeding 3 HP/s của người chơi. [TDD §5, tr.30; TDD locator: `ReadyOrNotCharacter.cpp:7012-7066`, `Data/BloodData.h:28,36`.]

Point hit không bị giáp đầu/thân chặn trên nhân vật do người chơi điều khiển có 30% cơ hội bắt đầu bleeding. Component tick `0.0167 s`; authority áp dụng `3 × DeltaTime`, xấp xỉ 3 HP/s. `HealCount` mặc định 3 rồi còn tăng khi bắt đầu heal: nhánh hồi đầy dành cho `<=1` không tới được; nhánh thực dùng nâng máu lên nửa max nếu đang thấp hơn và xóa bleeding. Chuẩn bị heal cất vật đang cầm: thường chờ nửa độ dài holster ×0.7, nhưng khiên chờ toàn bộ độ dài. [TDD §5, tr.30,33; TDD locator: `Components/BleedComponent.cpp:20,30-31,105-112,153-244`, `Characters/PlayerCharacter.cpp:2909-2917`.]

## Ít gây chết vẫn có thể trừ máu

`UStunDamage` có hook `ScriptedStunEvent`. Base bỏ qua nạn nhân chết/unconscious và SWAT AI, thêm phản hồi camera cho người chơi rồi `StartStun`. Base stun dài 6 s, gas 1 s, bỏ qua người đang bị bắt. Khi đang trang bị khiên, người chơi chặn mọi stun trừ gas. AI lấy thời lượng từ config, riêng gas 4 s và cộng trait `RiotControl`. [TDD §5, tr.30–31; TDD locator: `ReadyOrNotCharacter.cpp:7459-7488`, `Characters/PlayerCharacter.cpp:7677-7691`, `Characters/CyberneticCharacter.cpp:4407-4439`.]

Các loại không chỉ khác hiệu ứng hình ảnh:

- CS cộng `Damage ×25` vào tích lũy gas; tới 1.0 thì stun và clamp, nên các tick sau tiếp tục stun.
- Beanbag headshot do người chơi bắn vào AI dùng phản ứng đầu và ragdoll impulse mặc định 4000 hoặc ammo override; hit AI khác dùng `ST_Beanbag`.
- Nine-banger chỉ tác động AI, cộng penalty accuracy; tới 4 lần penalty thì stun.
- Pepperball cập nhật abuse counter, không stun lại AI đang choáng cùng loại. Pepper spray có timer riêng 4 s chỉ dừng montage.
- Taser bỏ qua tham số duration ở `StartBeingTasered`; AI trúng đầu còn knockout 10 s. Item taser tự đếm `PingStunDuration=8 s`, có thể lệch thời gian trạng thái stun.

Mặt nạ chỉ miễn `ST_Gassed` và pepper spray; inventory còn có danh sách `BlockAnyDamageFrom`. Người chơi có giới hạn tốc độ khi stun, tùy chọn khóa aim; yêu cầu nhìn causer kiểm cả recently rendered và LOS. [TDD §5, tr.31–32; TDD locator: `DamageTypes/LessLethal/CSGasDamageType.cpp:19-33`, `BeanbagDamageType.cpp:9-51`, `NinebangDamageType.cpp:16-29`, `PepperballDamageType.cpp:12-45`, `ReadyOrNotCharacter.cpp:6365-6376,7530-7566`.]

Cảnh báo quan trọng của TDD: cả hai `OnTakeDamage` trả true với stun, nên lượng damage vẫn bị trừ khỏi Health. Cờ `bCauseHealthDamage` chỉ che một lời gọi đã comment, không còn bảo đảm tính không gây chết; ý đồ phụ thuộc dữ liệu chưa đọc. Hệ nhiều stun cũng chỉ tái dùng một timer; stun mới thay timer cũ, kết thúc xóa cả `StunMap`. Proxy chỉ nhận một cặp `bRepStunned/RepStunnedWith`. Các tunable `WeaponDownLengthOnStun`, `StunSpeedMultiplier`, `LessThanLethalAmount` và thời gian pepperball theo vùng không có C++ reader; Blueprint chưa kiểm chứng. [TDD §5, tr.32–33; TDD locator: `Characters/PlayerCharacter.cpp:2731,2816-2828`, `ReadyOrNotCharacter.cpp:7484-7495`.]

## Tuân phục, báo cáo, còng và mang đi

`CanArrest` từ chối SWAT, người đã bắt đầu death sequence, đã/đang bị còng, hoặc mất một trong hai tay. Cho phép người đầu hàng; người chết, unconscious hoặc incapacitated sau khi báo cáo; ragdoll được phép không cần báo cáo. Tương tác với mục tiêu đã còng/chết/gục ưu tiên báo cáo nếu chưa báo, rồi xét mang đi. Còng ragdoll cần giữ 0.5 s; còng đứng chạy ngay khi đúng vị trí, nếu sai vị trí yêu cầu quay lưng. AI bỏ qua hold; mang người cũng hold 0.5 s. [TDD §5, tr.31; TDD locator: `ReadyOrNotCharacter.cpp:1810-1888,3393-3415`, `.h:1172,1178`.]

`Arrest` vứt vũ khí, khóa hành động, đặt đang bị bắt. `ArrestComplete` mở khóa, đặt hoàn tất, áp dụng physics asset còng nếu có, lưu pose cho ragdoll và phát event. `Surrender()` thuộc AI; trait `Pacifier` có thể khiến AI đang stun đầu hàng qua roll. Hàm `IsUnconsciousNotDead` trong C++ khảo sát luôn false, không có override, nên không thể mặc nhiên coi mọi tên trạng thái unconscious là nhánh đang hoạt động. [TDD §5, tr.31,33; TDD locator: `ReadyOrNotCharacter.cpp:1617-1620,3487-3578`, `Characters/CyberneticCharacter.cpp:3007-3015`.]

## Authority, mở rộng và bài học

Server kiểm `ShouldTakeDamage`, role và quyền game mode. Multicast dành cho máu, giáp, suppression và phản hồi. Tuy nhiên RPC sửa resource chỉ kiểm amount theo limit, không kiểm đầy đủ lý do heal; TDD suy luận client sửa đổi có thể yêu cầu health tới limit 1000 qua luồng tự heal. Bản tái tạo nên để server tính kết quả băng bó. Thêm loại less-lethal qua `UStunDamage`, giáp qua `ABaseArmour::HandleDamage`, ammo qua dữ liệu, blood/gore qua `UBloodData`; không sao chép component chết hoặc bảng hệ số trùng. [TDD §5, tr.32–33; TDD locator: `ReadyOrNotCharacter.cpp:6197-6199,6288-6379`, `Components/ResourceComponent.h:168-267`.]

## Bài thực hành tái tạo

**Kịch bản tự thiết kế:** một phòng tập, một mục tiêu có giáp trước, một mục tiêu đầu hàng, hai loại hit “đạn” và “choáng”. Không cần asset gốc. Owner của health, armor, stun và arrest là server; client chỉ yêu cầu hành động và trình bày kết quả. Trạng thái cần có: khỏe → bleeding/gục/chết; tự do → đầu hàng → đang còng → đã còng. Phụ thuộc tối thiểu: dữ liệu ammo, timer, vùng hit, interaction và event ledger.

**Tiêu chí chấp nhận:** cùng một hit chỉ trừ health một lần; đầu/tay cho kết quả theo bảng đang cấu hình; giáp chỉ giảm trong vùng phủ; băng bó không vượt ngưỡng quy định; quy tắc hai stun chồng nhau được định nghĩa rõ; giữ còng bị hủy không tạo trạng thái “đã còng”; client không tự tăng health. Ghi riêng ca projectile và point hit để phát hiện nhánh bỏ qua giáp. Đây là yêu cầu của bài tập, không phải xác nhận runtime nguồn.

Đọc tiếp [sát thương và giáp trong quyển súng](../03-Gun-Gameplay/05-Damage-armour-reaction.md), [tương tác và bắt giữ](../02-Gameplay-Va-AI/03-Tuong-Tac-Bat-Giu-Vat-Chung.md), [quyền sở hữu multiplayer](../04-Kien-Truc-Va-Quy-Trinh/03-Multiplayer-State-Ownership.md).

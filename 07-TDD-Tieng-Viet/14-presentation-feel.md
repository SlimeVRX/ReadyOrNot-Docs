# 14. Hình ảnh, hoạt ảnh và cảm giác điều khiển

> **Phạm vi nguồn:** bản học tập từ TDD R&D reverse-engineered độc lập ngày **27/09/2026**, §14, tr.83–89. Không phải tài liệu chính thức VOID, không xác nhận bản bán lẻ mới nhất. Các `file:line` là **địa chỉ được TDD dẫn**, chưa được kiểm chứng lại ở chương này; đường dẫn mặc định thuộc `Source/ReadyOrNot/`.

## Biến trạng thái mô phỏng thành cảm giác

TDD mô tả HUD thường trực khá ít thông tin: hình nhân, đạn và lời nhắc theo ngữ cảnh. Sự kiện bị trúng đạn, choáng, áp chế, đeo mặt nạ hoặc chết được chuyển thành hậu kỳ toàn màn hình, camera shake, hoạt ảnh, ragdoll và phụ đề. Một hệ thống như vậy có hai trách nhiệm: phản ánh trạng thái đủ rõ để người chơi hiểu, đồng thời không làm cảm giác điều khiển biến thành nhiễu khó đọc.

`UPlayerPostProcessing` kế thừa `UPostProcessComponent`, giữ mảng effect có tên. `UPostProcessEffectData` chứa material và các track scalar/vector; requirement dạng Blueprint quyết định effect được bắt đầu hoặc đảo chiều hay không. Một requirement native kiểm tra damage causer có chiếu lên màn hình không (`Data/PostProcessRequirement.cpp:31-40`). Các anim notify nối montage với nạp đạn, breach, bước chân, IK, gắn item và effect theo tên. TDD đếm khoảng 58 lớp notify/notify-state, không chứng minh tất cả đều được content sử dụng.

Hệ phân cấp hoạt ảnh cần đọc đúng: PlayerFP/PlayerTP/PlayerTP_V2 đi qua `UReadyOrNotAnimInstance`; `URoNAnimInstance_HumanBase` kế thừa trực tiếp `UAnimInstance`, là nhánh ngang cho nhân vật người/NPC, không phải lớp giữa của cả ba lớp player. `Choreo` chỉ có module Editor. `Motivity` có các lớp giống motion matching nhưng không có header trong `Source/ReadyOrNot` tham chiếu nó; việc Blueprint/content sử dụng vẫn **chưa xác minh**, thuật toán tìm pose chưa được tác giả đọc. Nguồn: §14, tr.83–84.

## Dòng sự kiện và bộ phát effect

`BeginPlay` đăng ký delegate cho stun, bullet impact, suppression, damage, damage details, killed và thay đổi inventory. Handler cập nhật số scalar hoặc gọi effect theo tên. Mỗi tick xử lý effect thông thường, rồi NVG/gas mask/blood/injury/suppression/outline, fade blendable, DOF/motion blur và cuối cùng chép toàn bộ `FPostProcessSettings` vào camera góc nhìn thứ nhất (`Components/PlayerPostProcessing.cpp:29-187`).

Vì `OnRegister/OnUnregister` gọi `Super::Super`, component bỏ qua đăng ký riêng của `UPostProcessComponent` (`:1669-1677`). TDD **suy luận dựa trên engine 4.27** rằng cách này bỏ đường blend qua post-process volume của world. Không nên kể đây như một pattern an toàn xuyên mọi phiên bản UE: chính TDD coi nó là thiết kế mong manh khi đổi engine.

Một track có trạng thái Hold, Forward, Reverse, WaitingForReverse, Ended; lựa chọn cuối track là Hold, Loop hoặc Reverse. `FPlayHead` giữ hướng chạy, vòng lặp và thời gian còn lại. Track lấy mẫu curve hoặc ease từ Start đến End, với `EffectLifetime` mặc định **3 giây**. Reverse có requirement riêng hoặc được phép bất cứ lúc nào. Khởi động lại effect đang chạy không có tác dụng trừ khi bật `bRestartIfAlreadyPlaying`. Nguồn: `Data/PostProcessEffectData.h:24-39,43-280`; `PlayerPostProcessing.cpp:403-423,533-798`.

## Công thức phản hồi tổn thương: giữ nguyên cả điểm lạ

Các con số dưới đây là literal C++ mà TDD ghi nhận tại `PlayerPostProcessing.cpp:1555-1592`, không phải tham số designer tự chỉnh trong asset. Không tự “sửa hợp lý” những công thức có vẻ ngược trực giác:

| Tình huống | Giá trị được ghi nhận trong TDD |
|---|---|
| Greyscale | `RemapClamped(HealthRemaining, [0,100] → [0,0.6])` |
| Headshot hoặc helmet block | Cộng 1,0 headshot radial blur, clamp 0–5 |
| Armor hoặc helmet block | Cộng 1,6 white flash; đặt hit amount và red flash về 0 |
| Hit khác | Cộng `1 − Damage/100` vào hit amount, clamp 0–3; drain 1,0/giây; red flash 2,0 |
| Viền vignette | Cộng `(1 − Health/100)×2 + (1 − Damage/100)×1`, clamp 0–1 |
| Mọi hit | Sharpness 3,0; suppression intensity về 0 |
| Hit chết, HealthRemaining ≤ 0 | Hit amount 3,0; drain 0,1/giây; headshot radial blur 1,0 |

`ProcessInjuryEffects` giảm tuyến tính về 0 bằng `FInterpConstantTo`: vignette 2,0/giây; hai radial blur 0,125/giây; sharpness 3,0/giây; hit amount theo `HitAmountDrain`; red flash 10,0/giây; white flash 8,8/giây; greyscale 0,1/giây (`:1333-1352`). Khi một trong hai chân bị thương, bắn cộng 0,2 injury radial blur, tối đa 0,5 (`:1658-1667`).

Suppression cập nhật `Intensity = clamp(Intensity + Strength×0.05, 0, 1)`, đặt cuối track `BlurRadius = Intensity×15`, rồi giảm intensity 0,2/giây (`:1362,1383,1541-1546`). Cách mỗi cue có tốc độ giảm khác nhau giúp flash ngắn và áp chế kéo dài có cảm giác khác nhau. Đó là pattern đáng tái tạo; các hằng số cụ thể cần hiệu chuẩn cho game mới.

## Camera và chuyển động

`AddCameraShake2` nhận instance do caller cung cấp. Shake single-instance được restart tại chỗ nếu cả hai phía không custom-initialize; nếu có custom initialization, instance cũ bị stop và slot thành null. Việc lấy lại từ expired-shake pool đã comment out (`Info/RONCameraModifier_CameraShake.cpp:7-96`). `StartCameraShake2` bỏ qua null hoặc scale ≤ 0. TDD nêu hai lỗi bảo trì: camera manager thêm lại `DefaultModifiers` sau `Super`, tạo hai shake modifier; vòng tìm slot null thiếu `break`, đưa một shake vào nhiều slot. TDD nói vòng lặp thứ hai kế thừa từ stock 4.27; tác động tới expired pool **chưa xác minh**.

DOF nội suy bằng `FInterpTo`, speed 10. ADS thường dùng f-stop 5, focal distance 1000; PiP scope dùng f-stop 10, focal distance 50, focal region 200. Khi ngừng ADS, nó trở về camera CDO rồi tắt override. TDD không ghi đơn vị riêng cho các giá trị focal này; không tự đổi thành mét. Motion blur dùng setting người dùng và đặt `MotionBlurTargetFPS = 0`. Outline dùng hai cvar sample mặc định 3 và 6, clamp 1–32, chỉ đẩy vào MID khi thay đổi (`PlayerPostProcessing.cpp:1406-1462,1594-1617`).

`URoNMoveStyleComponent` tra DataTable cho style/gait, replicate tên style, gait index/name và strafing. Pending gait/time-out được TDD **suy luận** là buffer tránh đổi gait liên tục. Stride warping làm mượt velocity, tính tỷ lệ tốc độ so với animation gốc, clamp 0–3; playrate bị giới hạn `1 ± PlayrateMaxAdjustment`, phần tỷ lệ còn lại chuyển thành stride scale, cũng clamp 0–3. Turn-in-place của human dùng yaw limit **140°**; thời gian 0,42 giây cho flip ±180° chỉ là đề nghị trong comment, không phải giá trị runtime đã xác nhận. Nguồn: `Animation/MoveStyle/RoNMoveStyleComponent.h:23-100`; `Animation/StrideWarping/StrideWarpingLibrary.cpp:7-14`; `Animation/TurnInPlace/AnimTurnInPlaceLibrary.h:37-39`.

## Ragdoll: phân biệt thiết kế với nhánh đang chạy

`EnableRagdoll("Death")` áp physical-animation profile dưới pelvis, bật CCD, bật mô phỏng các body dưới pelvis và bật kiểm tra ragdoll. Callback va chạm capsule/body là trigger còn hoạt động; ngưỡng mặc định 1 nên một va chạm đủ kích hoạt. Default gồm floor angle 15°, floor/in-air delay 0; freeze velocity 3,0, freeze delay 3,5 giây; blend 0,15 giây; bone pelvis và collision profile Ragdoll.

Nhưng ba nhánh update đã **comment out**: blend animation sang ragdoll, trigger floor-angle/in-air và freeze theo vận tốc pelvis. Vì vậy các trường chỉnh tương ứng hiện diện trong editor nhưng không tạo hành vi như tên gọi; TDD mô tả vào ragdoll là chuyển thẳng sang simulation, xác nằm yên không được component này freeze. Nguồn: §14, tr.86,88; `Plugins/AMRagdoll/Source/AMRagdoll/Components/RagdollComponent.cpp:27-39,169-231,533-600`. Không dùng các field này làm bằng chứng blend/freeze còn hoạt động.

## Phụ đề, authority và rủi ro

Subsystem phụ đề loại câu dài ≤ 0 hoặc đang cooldown; tạo khóa `<Speaker>_<VoiceLineBaseName>`, tra tên từ `names.csv` với fallback, rồi broadcast delegate. CSV theo speaker/locale; fallback locale mặc định `en`. Thư mục VO thay theo target VO/VO_PS/VO_XBOX và được stage non-UFS. Widget mặc định giữ tối đa 3 dòng, linger 1 giây, fade-in 0,8, fade-out 1, pad-in 0,5. Cooldown bỏ hậu tố cuối `_…` khỏi tên câu và dùng `FPlatformTime::Seconds()`, nên pause/time dilation không kéo dài cooldown (`Subsystems/SubtitlesSubsystem.cpp:150-244`). Điều kiện nghe được nằm ở [chương âm thanh](./13-audio.md).

Effect state không replicate; đây chủ yếu là presentation của client sở hữu. TDD ghi remote character còn sống đi qua early return; đổi `bCanEverTick` sau đăng ký có lẽ không gỡ tick, là **suy luận theo engine**. Remote chết/unconscious vẫn xử lý đầy đủ. Requirement UObject có property Replicated nhưng được tạo cục bộ; TDD cho rằng replication ấy có lẽ không tác dụng. Lỗi death fade client được workaround bằng kiểm tra mỗi tick và phát lại Death; chưa phải sửa nguyên nhân.

Rủi ro nghiêm trọng nhất của mục này là guard rỗng tại `PlayerPostProcessing.cpp:1249-1250`: khi không tìm thấy instance `Hit_RadialBlur` cần thiết, index có thể là `INDEX_NONE`, nhưng vẫn truy cập mảng. TDD dự đoán assert ở development hoặc undefined behavior ở shipping; **có xảy ra hay không phụ thuộc dữ liệu**. Ngoài ra, handler `OnDamageTaken` rỗng nhưng còn bind; tick `SkinComponent` bị tắt mặc định, nên thay skin thực tế dựa trên caller tường minh.

## Proposed — bài thực hành tái tạo

**Phụ thuộc:** character có event damage/stun, camera, montage, data asset, một ragdoll bằng asset riêng và UI phụ đề. Tạo effect có tên, curve và requirement; đưa magnitude/decay vào asset có đơn vị; dùng volume blending hoặc một `UActorComponent` sở hữu rõ toàn tuyến camera.

**Tiêu chí đạt:** asset thiếu effect trả lỗi có kiểm soát; gọi lặp tuân thủ restart policy; cue giảm đúng giây thực hoặc giây gameplay đã chọn; shake không nằm nhiều slot; remote sống không tốn full post-process; client chết nhận fade một lần đúng thời điểm; pause có hành vi cooldown được mô tả; ragdoll blend/freeze được đo riêng trước khi bật. Bài tập này tái tạo ý tưởng, không sao chép code, material hay motion data của RoN.

# Vì sao Gun Lab cần cả hệ thống trình diễn của Ready or Not

Audit ngày 2026-10-08 trên source và asset cục bộ. Các đường dẫn source dưới đây tính từ `Ready Or Not/Source/ReadyOrNot`. Đây là phân tích và công cụ chuyển định dạng cho bản source đang có; không phải khẳng định mọi hành vi giống bản retail hiện hành.

Gunfeel đi qua nhiều tầng cùng lúc. Một phát bắn đúng damage và tốc độ bắn vẫn có thể cảm thấy sai nếu pawn, FP AnimBP, camera shake, attachment, sound data hoặc input state thiếu kết nối.

| Tầng | Đường đi native cần giữ | Hệ quả khi thiếu |
|---|---|---|
| Recoil điều khiển | `Characters/PlayerCharacter.cpp`: `OnEquippedWeaponFire`, `RecalculatePendingRecoil`, `ApplyRecoil` | Tâm ngắm và hướng nhìn không phản ứng theo súng |
| Recoil trên mesh | `Actors/BaseWeapon.cpp`: `ComputeProcRecoil`, `TriggerProcRecoil`; FP AnimBP và WeaponAnimData | Súng trong tay không có đúng chuyển động, spring, ADS blending |
| Camera animation | Camera shake class của weapon/animation notify; `FireCameraShakeInst` tạo trong native `BeginPlay` | Thiếu nhịp draw, reload, fire và hit mặc dù recoil điều khiển vẫn chạy |
| Free aim, sway, inertia | `BaseItem` free-aim getters; `PlayerCharacter::CalcCamera`; `URoNAnimInstance_PlayerFP` | Súng bị dính cứng vào giữa màn hình hoặc không phản ứng với movement |
| Attachment | Native `CanAddAttachment`, `AddAttachment`, `RemoveAttachment` và danh sách khả dụng của từng weapon | Thiếu sight, ADS offset, multiplier, secondary sight, laser/flashlight |
| Âm thanh | Native SoundData, FP/TP FMOD events, animation notify; room/portal graph | Thiếu fire/reload layers hoặc khác trải nghiệm trong phòng |
| Tương tác cận vật cản | Native pawn, collision/weapon state và ready-state logic | Súng không phản ứng đúng khi kê gần tường/cửa |

Không bật `bCalculateProcRecoil` hàng loạt: giá trị này là lựa chọn của weapon CDO. Không cộng thêm recoil mới lên camera; native đã áp dụng recoil điều khiển, mesh motion và các lớp camera khác nhau.

## CameraAnim cũ bị mất trong quá trình chuyển engine

UE5 hiện tại không còn lớp `CameraAnim`/Matinee mà một số asset cũ tham chiếu. Log missing class không có nghĩa toàn bộ recoil đã mất: shake dùng oscillator, recoil điều khiển và camera socket trong animation vẫn là các đường khác nhau.

`extract_legacy_camera_tracks.mjs` đọc tagged properties của asset gốc, kiểm tra giới hạn dữ liệu, lấy SHA-256 và xuất curve vào `Saved/GunLab` cục bộ. Nó không ghi lại asset gốc. Kết quả audit: 71 camera có curve hợp lệ, 64 tham chiếu shake→camera; một camera hit chân phải bị loại vì timestamp không tăng. Tham chiếu không có dữ liệu hợp lệ sẽ không được gán thay thế.

Chuyển thẳng Euler/position keys sang Sequence là sai. Một số animation được tác giả đặt ở vị trí khác origin, hoặc bắt đầu ở góc 90°. Legacy camera lấy transform tại t=0 làm gốc rồi áp dụng `Current * InitialInverse`. Đây là phép đổi hệ tọa độ, không phải trừ riêng từng góc Euler. Constructor UE4 đặt relative transform/FOV=true, BaseFOV=90 và AnimLength=3 giây khi property không được serialize. [Nguồn constructor của Epic](https://github.com/folgerwang/UnrealEngine/blob/release/Engine/Source/Runtime/Engine/Private/Camera/CameraAnim.cpp), [nguồn playback của Epic](https://github.com/folgerwang/UnrealEngine/blob/release/Engine/Source/Runtime/Engine/Private/Camera/CameraAnimInst.cpp).

`recover_legacy_camera_sequences.py` thực hiện chuyển định dạng:

- Position polynomial và tangent được đổi theo inverse initial basis. Cubic tangent chuyển từ đơn vị/giây sang đơn vị/tick; readback kiểm tra các tangent thực sự tham gia cubic interpolation.
- Chỉ chọn chín double transform channels, tách weight channel. API scripting thêm key của engine này đi qua tham số float nội bộ; helper hiện ghi trực tiếp `FMovieSceneDoubleValue` bằng native channel `Set` để giữ double value, float tangent và interpolation đã kiểm tra.
- Khi bản gốc dùng quaternion interpolation, các quaternion key được đổi sang hệ gốc. Legacy dùng slerp giữa key, độc lập với Euler tangent. [Nguồn Matinee của Epic](https://github.com/folgerwang/UnrealEngine/blob/release/Engine/Source/Runtime/Engine/Private/Interpolation.cpp).
- Khi bản gốc dùng Euler polynomial, converter lấy mẫu chính chuyển động đã có để tạo native quaternion track. Bước tối đa 1/240 giây và adaptive probes ngưỡng 0,002°. Đây là chuyển định dạng có sai số đo được, không phải chuyển đổi bit-exact hoặc motion mới do lab thiết kế.
- Kiểm tra dữ liệu đọc lại bằng đúng thuật toán Hermite/slerp của channel native ở 960 Hz cộng các điểm giữa độc lập. Chỉ chấp nhận sai số đo được dưới 0,005° và 0,001 cm. Receipt lưu số mẫu, sai số, timestamp quantization và lý do loại từng asset; không tuyên bố một bound toán học liên tục hoặc kiểm chứng toàn camera stack.
- Sau readback, gọi chính `USequenceCameraShakePattern::ScrubShakePattern` qua helper kiểm chứng và so pose native với curve gốc. Runtime probe đọc player/stand-in của sequence trong các shake đang hoạt động; đây là kiểm tra khác với việc chỉ thấy CDO có một asset tham chiếu.
- Dữ liệu channel và pose đi qua hai lần gọi native theo JSON cho mỗi camera. Python trong editor chỉ nối API/asset; `camera_conversion_worker.mjs` chạy toán và kiểm chứng bằng Node riêng, không giữ hàng loạt scripting-key UObject hoặc wrapper của `FTransform`. Đây là công cụ chuyển định dạng/kiểm chứng, không tham gia tạo chuyển động trong lúc bắn. `legacy_camera_math.py` được giữ làm bản tham chiếu độc lập; pipeline được hỗ trợ dùng bản `.mjs`.
- Output chia theo fingerprint của converter, toán chuyển đổi, helper và phiên bản Node. Phiên bản mới tạo asset mới; chạy lại cùng phiên bản chỉ đọc và kiểm chứng lại mọi key/pose, không xóa rồi xây lại track. Source SHA-256 và fingerprint phải khớp trước khi tái dùng. Script tự tìm Node trên PATH; có thể chọn bằng `-CameraNode` hoặc `RON_GUNLAB_CAMERA_NODE`.
- Sequence factory tạo một spawn track bên cạnh transform track. Converter kiểm tra spawn track có đúng một bool section không giới hạn, default=true và không có key; không bỏ qua track lạ. Chỉ khi toàn bộ camera hợp lệ đã qua kiểm chứng mới thay manifest đang dùng bằng atomic rename. Lần chạy thất bại ghi chẩn đoán riêng trong `Saved/GunLab`, giữ nguyên manifest tốt trước đó.
- Sequence mới chỉ nằm trong `/Game/ReadyOrNot/Level/Study/CameraRecovery`. Helper gán sequence vào shake CDO/instance thiếu sequence, giữ native scale/oscillator, rồi hoàn nguyên khi rời lab. Default object đã gán được giữ sống trong phiên lab để garbage collection không làm mất mapping khi đổi weapon sau đó. Manifest chưa xác nhận transform semantics bị chặn.
- Sau khi thay hoặc hoàn nguyên native property trên Blueprint CDO, helper gọi `UpdateCustomPropertyListForPostConstruction`. Engine dùng danh sách cache này khi tạo instance; chỉ sửa CDO có thể khiến `FireCameraShakeInst` tạo sau đó vẫn nhận `AnimSequence=None`. Khi rời lab, helper cũng hoàn nguyên những instance được tạo sau lần gán đầu, chỉ khi class/world/sequence khớp; dừng riêng sequence đã khôi phục, không thay oscillator.

Curve JSON, asset chuyển đổi và full receipt là dữ liệu học tập cục bộ trong project, không đưa vào repo tài liệu. Repo chỉ chứa công cụ tự viết và phương pháp kiểm tra. Chạy thành công converter vẫn cần PIE để kiểm tra camera manager, animation notify, blending và tình huống cụ thể của từng weapon.

Commandlet có thể trả exit code 1 dù script hoàn tất: log có thể chứa lỗi Blueprint/import đang có trong project hoặc thông báo tìm asset phiên bản mới trước khi tạo nó. Chỉ nhận kết quả khi cả hai stage có receipt mới và pipeline đã ghi completion marker; exit code này không chứng minh project sạch lỗi. Crash, thiếu marker hoặc bất kỳ camera hợp lệ nào không qua kiểm chứng đều làm pipeline thất bại. Kiểm tra gameplay runtime là bằng chứng riêng.

Pipeline Node v24.19.0 ngày 2026-10-08 đã hoàn tất cả hai lần chạy: tạo mới lúc 03:30:16 UTC và tái dùng lúc 03:31:51 UTC. Lần sau đọc lại đủ **71/71 sequence**, không xây lại track hoặc save asset, kiểm chứng **509.900 pose native**, giữ **63 mapping** và không có camera hợp lệ nào thất bại. Cả hai dùng cùng fingerprint `970a3ba17f19…` và cho cùng sai số lớn nhất: 0,001989° rotation, 0,000000394 cm position; quantization key-time tối đa khoảng 29,8 ns. Pipeline tái dùng ghi completion marker lúc 03:32:05 UTC.

Một camera hit chân phải có timestamp không tăng được giữ ngoài mapping. Xem [receipt chỉ chứa số liệu tổng hợp](../lab/manifests/camera_migration_receipt.json). Kiểm tra nhánh thất bại cũng xác nhận giữ nguyên cả manifest đang dùng và aggregate tốt trước đó; chẩn đoán lần thất bại được ghi riêng cục bộ. Các kết quả này chứng minh chuyển định dạng và khả năng chạy lại; cảm giác trong gameplay và các lớp animation/audio còn cần kiểm tra riêng.

Không dùng số sequence đang phát làm tiêu chí chung cho mọi weapon. Kiểm tra tagged property và parent class của các asset cục bộ cho thấy:

| Weapon native | Camera khi bắn | Cách kiểm tra phù hợp |
|---|---|---|
| `Primary_SR16` | `C_Generic_Fire_Shaky_M4A1`, trực tiếp kế thừa `Engine.CameraShake`; có oscillator, không có CameraAnim | Quan sát native legacy shake, thời gian oscillator và camera/recoil; sequence bằng 0 là hợp lệ |
| `Primary_M16A4` | `C_Generic_Fire_Shaky_M16`, tham chiếu authored `Generic_Camshake_Fire_Major` | Bắn weapon thật rồi đo sequence player/stand-in đã chuyển định dạng |
| `Primary_S590_Assault_v2` | `C_Generic_Fire_Shaky_590A`, cũng tham chiếu authored `Generic_Camshake_Fire_Major` | Kiểm tra sau fire và chu kỳ shotgun native |
| `Secondary_G19_V2` | Không serialize `FireCameraShake`; có các tham chiếu draw/holster/reload riêng | Không tự gán camera fire của một G19 khác chỉ vì tên giống nhau |

Source native tạo `FireCameraShakeInst` trong `Actors/BaseWeapon.cpp:134`, rồi gọi từ `Characters/PlayerCharacter.cpp:3388` qua controller và `AReadyOrNotPlayerCameraManager`. Runtime probe đọc đúng `CachedCameraShakeMod` mà đường gọi này sử dụng. CDO có sequence chỉ chứng minh đã nối dữ liệu; số active shake, oscillator time, player time và pose đang được đánh giá mới giúp phân biệt đường camera thực sự chạy.

Trong runtime run `20261008-040132`, phase `recovered_camera_fire` bắn `Primary_M16A4` bằng native input sau khi reset bộ đếm camera. Instance native giữ đúng sequence `Generic_Camshake_Fire_Major` thuộc phiên chuyển đổi `970a3ba17f19`. Observation ở giây 102,007507 ghi một sequence hoạt động, playback lớn nhất 2,975468866 giây và pose rotation lớn nhất 1,744122161°; các observation sau đó giữ nguyên hai cực đại này. Translation bằng 0 phù hợp clip này. Đây là pose do sequence player đánh giá trước native gain/blend và các lớp camera khác; không phải độ lệch cuối cùng của góc nhìn hoặc chứng nhận cảm giác giống retail. Verifier của toàn chuỗi input ghi 38 passed, 0 failed, 0 missing; kết quả camera ở đây chỉ xác nhận đường native playback của ca M16A4 đã thử.

## Ba sửa chữa có giới hạn trong source native

`repair_native_gunplay_port.mjs` chỉ khôi phục nhánh có bằng chứng trong source, với exact-match guards, SHA-256 và backup trong `Saved/Codex/NativeGunplayPort`:

- `BaseMagazineWeapon.cpp` đã đặt xác suất spall thành `false`; khôi phục native weighted random với cùng `Seed + 64`, đúng thứ tự tham số API UE5. Không thay đổi thuật toán penetration, seed hay tuning.
- `TrainingTarget.cpp` đã comment đăng ký point/radial damage delegate; khôi phục đăng ký và cập nhật radial hit-result thành const-reference cho signature UE5. Bia do native actor xử lý damage và score.
- `HUD/Widgets/Loadout/V2/Loadout_V2.cpp`: `NativeDestruct` không gọi `ExitLoadout` khi world đang teardown hoặc engine đã được yêu cầu thoát. Đường gọi cũ có thể save preset, tạo equipment/HUD trong lúc shutdown. Đóng menu bình thường vẫn đi qua native `ExitLoadout` và lưu như trước.

## Những khác biệt phải test đúng ngữ cảnh

`PlayerCharacter::ToggleCrosshairOverlay` mở development overlay từ `CrossHairOverlay` trong WidgetData. Hình debug gốc rất lớn, nên thấy widget tồn tại không đủ chứng minh tâm ngắm hữu ích. Lab dùng đường mở widget native này và texture gốc `HUD_Revised/HUD_Reticle`, chỉnh cách hiển thị trong phiên lab; có thể bật/tắt bằng `ronlab crosshair`. Đây là trợ giúp quan sát khi học, không thay đổi hướng đạn hay độ tản native.

Quick press X đổi mode qua danh sách `AvailableFireModes`; một pistol không tự có Auto/Burst. Hold X hiện chỉ broadcast thông tin safety vì lời gọi đổi safety đã comment. R có ba nhánh native: tap tactical reload, double press speed reload, hold mag-check; asset animation/state và remaining ammo quyết định kết quả nhìn thấy.

Build này định nghĩa `RON_NO_SPRINT`: Shift đi chậm qua hàm native có tên `FastWalk`; phép đo cùng loadout cho tốc độ 240 → 96 cm/s. Không tìm thấy input hay logic prone của player. Dùng hướng dẫn điều khiển và danh sách mode theo CDO trong [native systems và test guide](native-systems-and-test-guide.md).

Capture chạy `-nosound` không chứng minh FMOD hoạt động. Một số notify G19 trỏ tên event cũ bị thiếu trong asset set hiện tại; không nên tự đổi tất cả sang event `_Old` hay `_New` chỉ theo tên. `SoundManager` có fallback legacy khi thiếu room/portal volume, nên native fire có thể nghe được nhưng chưa đủ để kết luận âm học phòng giống level gốc.

Audit bổ sung đã đối chiếu project tác giả `Assets/Fmod-RON/Metadata/Event`, GUID trong `UFMODEvent.AssetGuid` và cả năm `Master.strings.bank` của các platform. Các event `G19 MagIn Old`, `G19 Slide Push Old`, `Arms Quick/Mid/Slow` vẫn có dữ liệu và GUID khớp giữa các nguồn này. Lookup event description từ Desktop banks cũng thành công khi nạp đúng plugin `resonanceaudio`. Đây là kiểm tra metadata bằng FMOD NOSOUND, không phải kiểm chứng nghe được âm thanh.

Các animation notify bị lỗi lại trỏ đường dẫn G19 không có hậu tố `_Old` và Arms trong thư mục `1P` đã mất. Import cũ chỉ cho biết object path, không cung cấp GUID của event bị mất; các bank hiện có cũng không lưu đường dẫn cũ đó. Vì vậy chưa có căn cứ xác nhận phép redirect từ đường dẫn bị mất sang event hiện tại. Không thay một event mới có GUID khác chỉ để xóa cảnh báo. Receipt chi tiết được giữ cục bộ trong `Saved/GunLab/fmod_identity_audit.json`; repo không chứa bank, audio hoặc XML tác giả.

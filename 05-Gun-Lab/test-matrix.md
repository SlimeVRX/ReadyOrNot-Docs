# Ma trận thử gunplay

Mỗi hàng là một phép thử có điều kiện đo. Giữ cùng độ phân giải, FOV, sensitivity, FPS, tư thế, khoảng cách, loại đạn và attachment trước khi so sánh. Ghi class path thay vì chỉ tên hiển thị, vì nhiều variant cùng tên.

Baseline tự động hiện có: [102 hàng equip và sáu cấu hình ADS/fire/reload](./README.md). Sáu cấu hình đều có aiming state và tiêu đạn; năm cấu hình tăng ammo sau yêu cầu reload. Pepperball còn 199 viên và cần kiểm tra reload riêng. Các ca camera, sound, impact, attachment và cảm giác bên dưới chưa được chứng nhận bởi baseline đó.

| Phép thử | Cách làm | Ghi lại | Điều không được suy diễn |
|---|---|---|---|
| Equip | Chọn lớp, chờ draw xong | owner, class, mesh, magazine, log | Equip thành công không bảo đảm bắn thành công |
| Single shot 10m | Đứng đúng vạch, bắn một phát | ammo trước/sau, impact, camera, sound | Camera rung không phải toàn bộ đường đạn |
| Sustained fire 25m | Giữ bắn với chế độ hỗ trợ | cadence, burst/single/auto, grouping | Không quy đổi fire-rate seconds thành RPM bằng tên trường |
| ADS | Hipfire → ADS → thoát ADS | sight alignment, FOV, timing, movement speed | ADS multiplier CDO chỉ là một đầu vào |
| Tactical reload | Magazine còn đạn | animation, đạn buồng, magazine giữ/lưu | Không ép kỳ vọng giống game khác |
| Empty reload | Bắn cạn rồi reload | bolt/slide/pump, delay, ammo, blocking | Không chỉ kiểm tra phím R đã được nhận |
| Movement | Walk/crouch/lean/ADS movement | sway, bob, free aim, collision | Inertia không đồng nghĩa recoil |
| Attachments | Baseline rồi một attachment native | đúng class, đúng socket, sight/recoil | Không so hai loadout khác ammo mà không ghi |
| 50/100m | Dùng lane tương ứng | impact/drop/feedback với native đạn | Target là geometry, không phải mô hình armor/health |
| Presentation | Mở renderer/audio thật | cảnh báo asset, muzzle/impact/reload sound | Headless audit không xác nhận audiovisual parity |

Mẫu nhật ký:

```text
Build/commit:
Class path / display name:
Ammo row / magazine count / attachment classes:
FPS / FOV / sensitivity / posture / distance:
Observed result:
Expected from source/assets:
Log/receipt/screenshot reference:
Status: observed / failed / not tested
```

Nếu so với game phát hành, hãy coi đó là phép đo riêng: cùng vũ khí và loadout chưa đủ khi snapshot source/assets, animation migration, âm thanh, balance patch hay input settings khác nhau. Chỉ ghi cảm giác tương đồng sau khi người chơi thực sự thao tác; không suy ra cảm giác từ test tự động.

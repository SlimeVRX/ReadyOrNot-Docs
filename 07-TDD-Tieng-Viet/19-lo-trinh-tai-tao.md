# Lộ trình tự tái tạo các hệ thống Ready or Not

Đây là **kế hoạch thực hành đề xuất** dựa trên quan hệ hệ thống trong TDD, đặc biệt §1, §2 và §18. Các mốc dưới đây chưa được triển khai chỉ vì đã có chương tài liệu. Mục tiêu là làm cho mỗi bước tạo ra một vòng chơi kiểm chứng được, rồi tăng chiều sâu mà không mất khả năng giải thích.

## Chọn một đơn vị trải nghiệm đủ nhỏ

Đơn vị đầu tiên là nhiệm vụ hai phòng: người chơi xuất hiện, nhận mục tiêu, quan sát cửa, tiếp cận một nghi phạm và một dân thường, xử lý tình huống, thu một vật chứng, báo cáo, xem kết quả và chơi lại. Nó nhỏ về nội dung nhưng có đủ quan hệ để nhận ra Ready or Not khác một sân bắn thông thường ở đâu.

Trước khi thêm súng thứ hai hoặc map thứ hai, bạn phải giải thích được vì sao một lượt thành công và lượt khác thất bại. Một target đứng yên dùng được cho gunfeel; một căn phòng có dân thường mới bắt đầu kiểm được lựa chọn dùng vũ lực. Vì vậy, phạm vi nội dung nhỏ không có nghĩa bỏ luật, phản hồi hay kết quả.

## Hợp đồng hệ thống của dự án học tập

Các owner dưới đây là thiết kế gợi ý cho project mới; không phải danh sách class nguyên bản của Ready or Not.

| Nhóm | State do nó giữ | Nhận input từ | Xuất kết quả cho |
|---|---|---|---|
| Mission | Pha nhiệm vụ, mục tiêu, kết quả đã chốt | Deployment, objective event, player status | HUD, debrief, persistence |
| Player action | Action hiện tại, quyền di chuyển và tương tác | Input, status, trang bị | Animation, item, door, arrest |
| Weapon | Ammo, chamber nếu có, mode, deadline và reload transaction | Trigger, inventory, config | Hit, presentation và HUD |
| Damage/status | Health, armor, stun, bleed và trạng thái sống | Hit đã xác thực, equipment | Action gate, AI, objective |
| Human AI | Perception memory, morale, action/activity đang chạy | Sense, threat, command, status | Movement, interaction, VO |
| Squad | Người được phân công, giai đoạn và hủy lệnh | Context resolver, input, world | Activity của từng đội viên, UI |
| Door | Lock, angle, trap, damage và tương tác đang chiếm dụng | Player, AI, equipment | Collision, navigation, âm thanh |
| Evidence/ROE | Sự kiện đã ghi, identity đã xử lý, ledger | Hành động và kết quả đã xác nhận | Objective, score, debrief |
| Audio/presentation | Voice đang phát, effect, blend và camera offset | Sự kiện gameplay có ID/thời gian | Hình, âm và trạng thái UI có thể đọc |
| Persistence | Profile, loadout, roster và tiến độ đã commit | Kết quả mission, thay đổi ngoài trận | Màn chuẩn bị của lượt tiếp theo |

Một state phải có nơi quyết định rõ ràng. Widget có thể hiển thị số đạn nhưng không tự trừ đạn; montage có thể báo tới mốc gắn băng nhưng giao dịch phải bảo đảm chỉ commit một lần; AI có thể đề xuất hành động nhưng không tự cấp quyền thay health của client khác.

## Các mốc theo phụ thuộc

| Mốc | Sản phẩm phải dùng được | Tài liệu đọc | Điều kiện đi tiếp |
|---|---|---|---|
| M0 | Baseline build, level nhỏ, log và retry | §2, §16 | Ghi được engine/config/build; hai lần reset không giữ state cũ |
| M1 | Nhân vật, nhìn, đi, crouch và action gate | §3, §14 | Từ chối action có lý do; animation và state không mâu thuẫn |
| M2 | Một khẩu, một target, fire/reload/empty/ADS | §4–6, §14 | Ammo bảo toàn; shot, hit, camera, âm thanh và UI cùng khớp sự kiện |
| M3 | Mission hai phòng với fixture tương tác | §1, §5, §7, §10 | Có đường thắng, thất bại, bắt giữ, vật chứng, điểm và retry |
| M4 | Nghi phạm và dân thường có perception/memory/morale | §9 | Mỗi hành động có input và lý do; không đọc thông tin vượt quá thiết kế |
| M5 | Hai SWAT, chọn đội, move/hold/stack/execute/cancel | §8–9 | Một người thất bại không treo toàn đội; lệnh bị hủy sạch |
| M6 | Cửa, khóa/bẫy và ít nhất hai cách tiếp cận | §6–7, §12 | Collision/nav/AI/âm thanh cùng phản ánh state cửa |
| M7 | Biến thiên level, roster và dữ liệu bake | §12, §9 | Tái hiện được lượt từ cấu hình; dữ liệu bake lỗi thời bị phát hiện |
| M8 | Âm thanh theo phòng, VO/subtitle và feedback đầy đủ | §13–14 | Đường âm thanh có thể giải thích; subtitle không tiết lộ điều không nghe được theo luật chọn |
| M9 | Co-op tích hợp, travel và state observer | §15 | Hai máy thống nhất kết quả, chịu được duplicate/cancel/late event trong phạm vi đã thiết kế |
| M10 | Hub, loadout, roster/stress và kết quả bền vững | §11 | Save/load và kết quả mission chỉ commit đúng một lần |
| M11 | Pipeline thêm nội dung và kiểm hiệu năng | §16–18 | Thêm variant/mission bằng dữ liệu có validation, build đóng gói có receipt |

M9 là mốc mở rộng và nghiệm thu co-op, không phải lần đầu nghĩ tới mạng. Ngay M0–M2 hãy định nghĩa authority và chạy một phép thử server/client nhỏ. Nếu toàn bộ luật trước đó phụ thuộc vào singleton của máy local hoặc đọc camera của player 0, tới M9 bạn có thể phải viết lại thay vì chỉ thêm replication.

Thứ tự này chủ động đưa cửa và ROE vào sớm hơn gợi ý “polish” cuối §18 của PDF. Lý do là mục tiêu ở đây là học vòng chơi Ready or Not: nếu cửa, tuân thủ và báo cáo không hiện diện, bài thử chưa kiểm được lựa chọn chiến thuật. Đây là quyết định của lộ trình học, không sửa lại lời mô tả nguyên bản của TDD.

## Đi hết một tình huống thay vì đi hết một thư mục

Giả sử nghi phạm nghe tiếng mở cửa nhưng chưa thấy người chơi. Chuỗi thực hành nên ghi được:

1. Door đổi state được authority cho phép; tạo sự kiện âm thanh tại vị trí đúng.
2. Perception nhận stimulus; memory cập nhật loại thông tin mà AI biết.
3. Bộ đệm phản ứng làm tín hiệu tới decision layer vào thời điểm có chủ ý.
4. Các action được kiểm gate và chấm điểm; một activity bắt đầu, có thời gian commit hoặc điều kiện ngắt.
5. Người chơi yêu cầu tuân thủ; morale/status và tình huống ảnh hưởng lựa chọn tiếp theo.
6. Nếu nghi phạm đầu hàng, arms/weapon pose và quyền hành động phản ánh state đó; “giơ tay” chưa được tính như “đã bị còng”.
7. Player/SWAT bắt giữ; commit cập nhật trạng thái, mục tiêu và các tín hiệu quan sát.
8. Thu vật chứng và báo cáo; ledger ghi đúng một sự kiện, UI/debrief giải thích điểm.
9. Retry dọn event, actor reference, VO và timer của lượt cũ.

Đây là scenario thiết kế cho bài tập. Không phải mọi game đều dùng đúng cùng event hoặc controller. Điều cần giữ là bạn có thể đặt breakpoint/log tại mỗi bước và giải thích hành vi cuối cùng bằng một chuỗi đầu vào rõ ràng.

## Tài nguyên tối thiểu phải đi theo hợp đồng

| Hệ thống | Tài nguyên tối thiểu cho một bản chứng minh | Khi nào mới mở rộng |
|---|---|---|
| Player | Một rig, idle/walk/crouch, action tay, camera và collision phù hợp | Sau khi scale/root/aim/action gate ổn |
| Gun | Một arms–weapon pair tương thích, sockets, fire/ADS/reload, sound/VFX cơ bản | Sau khi timing, interruption và đường bắn có phép kiểm |
| Human | Một rig, locomotion, surrender/arrest/status poses, VO rõ | Sau khi state biểu diễn đúng, tránh dùng animation che logic thiếu |
| Door | Một frame/door mesh, pivot/collision, lock/trap markers, tương tác | Sau khi player và AI dùng cùng quy tắc |
| Level | Hai phòng, đường đi, cover, light và các marker có validation | Sau khi replay/reset và bake không lỗi thời |
| UI | Objective, interaction prompt, ammo/status, command context, debrief | Sau khi mỗi widget có nguồn dữ liệu thật và trường hợp empty/error |
| Audio | Event một phát, reload/door/VO/hit, room/portal fixture | Sau khi routing, vị trí và giới hạn voice được nghe kiểm thực tế |

TDD và kho asset giải quyết hai câu hỏi khác nhau. TDD mô tả trách nhiệm và cách nối; asset cung cấp hình, âm và dữ liệu để trình diễn. Có đủ file không đồng nghĩa có đủ graph, notify, shader, state transition hoặc phụ thuộc runtime. Giữ một manifest cho asset của từng feature, ghi rõ fallback và phần đã tái tạo.

## Định nghĩa hoàn thành cho từng mốc

Một mốc hoàn thành cần có bốn phần cùng lúc: hành vi đúng, trình diễn rõ, trạng thái dọn sạch và bằng chứng tái lập. Ví dụ, cửa mở đẹp nhưng AI mắc nav là chưa hoàn thành; reload có âm thanh nhưng cancel làm nhân đôi ammo là chưa hoàn thành; score đúng ở lần đầu nhưng tăng lại sau reconnect là chưa hoàn thành trong phạm vi co-op.

Ma trận kiểm tra nên có ít nhất: đường thành công, bị từ chối, bị hủy, lặp input, mất actor/phụ thuộc, reset và observer. Bổ sung multi-FPS, packet delay hoặc nhiều AI khi mốc đã có cơ chế tương ứng. Không ghi PASS cho multiplayer trong một bài thử chỉ chạy một người.

Mỗi lần sửa, giữ mốc nhỏ nhất đã được nghiệm thu làm baseline. Ví dụ chỉnh recoil không được đổi nhịp hoặc ammo nếu đó không phải mục tiêu của task. Nếu sửa data làm đổi nhiều hệ thống, mở rộng test theo dependency, không chỉ chạy lại đoạn video đẹp nhất.

## Cách biến mục tiêu lớn thành backlog

Mỗi task nên là một câu về hành vi có thể quan sát: “Khi người chơi hủy bắt giữ trước mốc commit, nghi phạm trở về state có thể tiếp tục xử lý và không tăng objective”. Tránh task chỉ ghi “làm arrest system” hoặc “import all animation”. Sau đó ghi owner, phụ thuộc, fixture, file được sửa và tiêu chí kiểm.

Đặt độ sâu và độ rộng thành hai cột riêng. Một cửa đủ lock/open/close/cancel/AI/audio/network có độ sâu cao; 30 mesh cửa chưa có tương tác chỉ có độ rộng asset. Bạn có thể mở rộng nội dung sau khi một đại diện chứng minh được quy trình, nhưng vẫn cần kiểm từng variant có khác rig, kích thước hoặc luật.

Lượt học tiếp theo tốt nhất là chọn M3 hoặc một điểm yếu thực tế trong lab hiện tại, điền [phiếu](20-phieu-trien-khai.md), rồi làm một vòng nhỏ từ đầu tới cuối. Từ đây, “toàn bộ hệ thống” trở thành danh sách các hợp đồng đã kiểm và các hợp đồng còn thiếu, thay vì một yêu cầu không có điểm bắt đầu.

# Lộ trình thực hành: từ reference native đến một lát game tự làm

Đây là **lộ trình đề xuất theo năng lực**, không phải cam kết thời gian hoặc tuyên bố đã tái tạo toàn bộ game. Người mới nên tiến bằng sản phẩm có thể chơi và giải thích, không bằng số tuần đã trôi qua.

## Hai đường chạy song song

**Reference native** là project hiện có và Native Gun Lab. Giữ lớp súng/pawn/animation gốc làm chuẩn quan sát. **Candidate học tập** là phần bạn tự viết trong map/plugin hoặc project riêng. Mỗi khi thay một lớp, ghi rõ candidate đã khác reference ở đâu. Không biến reference thành một tập patch thử nghiệm khó quay lại.

| Mốc | Điều kiện vào | Sản phẩm ra | Tiêu chí đạt |
|---|---|---|---|
| M0 — Dựng môi trường | Source, Engine, dependencies và đủ dung lượng | Target build được, Editor mở, snapshot/hash rõ | Phân biệt lỗi build, asset, config và runtime |
| M1 — Đọc một khẩu | M0 + catalog/CDO + map native | Feature trace HIP/ADS/single/reload | Chỉ ra owner, interval, commit, asset và giới hạn |
| M2 — Viết một khẩu tối thiểu | M1 + hiểu Pawn/Controller/Component | Candidate input/equip/ammo/fire/hit/reload, ownership thiết kế từ đầu | Empty/cancel/retry đúng state; thử authoritative flow với hai client ở lát súng nhỏ |
| M3 — Ghép gunfeel | M2 + rig/animation/camera/audio | Timeline logic và presentation cùng reference | Đo đủ HIP/ADS, một viên/giữ cò, không sửa nhiều biến cùng lúc |
| M4 — Tương tác hai phòng | M3 + doors/collision/navigation | Door → interaction → target reaction | Cửa khóa/mở/chặn/cancel có output rõ |
| M5 — AI tối thiểu | M4 + perception/action/morale | Một đối tượng phản ứng có lý do | Debug được stimulus → lựa chọn → hành động, không chỉ random |
| M6 — Vòng nhiệm vụ | M5 + objectives/ROE/results | Menu → mission → result → retry | Thành công/thất bại kết thúc đúng một lần |
| M7 — Coop và production | M6 + authority/replication/profiling | Hai client, reconnect/travel/regression có hồ sơ | Không nhân đôi ammo/damage, state khớp, có ngân sách hiệu năng |

## M0: học cả lỗi chuẩn bị dự án

Snapshot này đã cần build custom Editor và ShaderCompileWorker, khôi phục Engine/Content theo manifest, chọn Windows SDK phù hợp và bỏ một mục SM4 lỗi thời trước khi Editor mở được. Những bước này là dữ liệu của môi trường cụ thể, không phải hướng dẫn áp dụng mù cho mọi Unreal version.

Giữ một log môi trường: Engine Build.version, toolchain, target/configuration, plugin list, config thay đổi và kết quả chạy. Đọc [phụ lục môi trường](../06-Catalogs/02-moi-truong-va-gioi-han.md). Không cài lại hoặc rebuild toàn Engine khi một dòng log đã chỉ đúng file config lỗi.

## M1: nghiên cứu trước khi chỉnh

Chọn một khẩu mặc định. Ghi class path, ammo type, attachment, stance, độ nhạy, FOV, frame rate và input device. Chạy một chuỗi ngắn: equip, hip single, ADS single, giữ cò, reload khi còn đạn, reload rỗng, đổi súng giữa action.

Mục tiêu không phải bắn nhiều. Mục tiêu là có một baseline mà hôm sau tái lập lại được. Tập dùng cùng góc camera và khoảng cách target để tránh cảm giác khác chỉ vì bối cảnh.

## M2–M3: thay từng lớp để hiểu dependency

Một cách học tốt là giữ native asset/presentation nhưng tự viết một mô hình state nhỏ; cách khác là giữ native logic nhưng thay lớp presentation trong một bản thử riêng. Hai bài học khác nhau, phải ghi nhãn rõ. Sau đó mới ghép vào một candidate tự sở hữu đầy đủ.

Thứ tự đề xuất:

1. Input và state gates.
2. Inventory/equip cùng quyền sở hữu weapon instance.
3. Ammo/chamber/magazine và thời điểm commit.
4. Trace/projectile/collision và damage giả lập trong game.
5. Reload/cancel cùng các action transitions.
6. Camera/ADS/recoil và animation graph.
7. Audio/VFX/UI và hit feedback.
8. Mở rộng replication/prediction và các ca mạng phức tạp; quyền sở hữu và quyền commit đã phải được thiết kế từ bước 1–3.

**Không để network đến cuối mới nghĩ tới.** Ngay M2–M3, thử một khẩu với server và hai client để kiểm ammo/damage ownership, local feedback và simulated proxy. M7 là vòng mở rộng integration, reconnect/travel và production, không phải lần đầu gắn authority vào game.

Không cần đưa toàn bộ 102 entry vào candidate ngay. Dùng một rifle, một pistol, một shotgun và một nonlethal để phát hiện ranh giới mô hình. Sau khi contract chịu được các khác biệt, mới mở rộng catalog.

## M4–M6: kiểm tra phần “game” chứ không chỉ “gun”

Tạo một nhiệm vụ hai phòng có cửa, một người có thể tuân lệnh, một vật chứng và một điểm kết thúc. Sau đó thêm nhánh phản ứng không hợp tác, failure và retry. Mọi lần bổ sung phải giữ vòng cũ chơi được.

Đừng dựng một map lớn trước khi biết initialization, AI spawn, navigation, objective và result có liên hệ ra sao. Một căn phòng xấu nhưng vòng chơi đúng là công cụ debug tốt hơn một level đẹp mà không biết owner nào đang hỏng.

## M7: kiểm chứng ngoài đường thuận lợi

Chạy standalone, listen server/owning client và remote simulated view theo từng case. Thử frame rate khác, độ trễ mạng, input bị giữ khi travel, owner bị destroy giữa timer và asset không load được. Những bài này thường lộ vấn đề kiến trúc sớm hơn việc thêm khẩu thứ năm mươi.

## Nhật ký học hàng ngày

Mỗi phiên chỉ cần một trang: câu hỏi; source/CDO đã đọc; giả thuyết; thay đổi duy nhất; ca thử; kết quả; điều bị bác bỏ; câu hỏi tiếp theo. Nếu hôm nay không viết code nhưng bác bỏ một giả định sai về chamber hoặc animation base, đó vẫn là tiến bộ có giá trị.

Điểm kết thúc của một mốc là **có artifact + giải thích + bằng chứng**. “Đã xem hết chương” hoặc “AI báo xong” không thay cho ba thứ này.

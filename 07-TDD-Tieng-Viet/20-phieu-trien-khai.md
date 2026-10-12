# Phiếu triển khai một hệ thống gameplay

Sao chép phiếu này khi bắt đầu một feature hoặc một lát cắt của [lộ trình tái tạo](19-lo-trinh-tai-tao.md). Các ô để trống là việc bạn cần quyết định hoặc đo, không phải giá trị mặc định lấy từ Ready or Not. Hoàn thành phiếu không thay thế cho implementation và kiểm tra runtime.

## Mục tiêu và phạm vi

| Trường | Điền cho công việc của bạn |
|---|---|
| Tên hệ thống và hành vi quan sát được | … |
| Người chơi muốn làm gì, vì sao việc đó có ý nghĩa? | … |
| Một ca thành công ngắn nhất | … |
| Một ca thất bại hoặc bị từ chối | … |
| Những phần chưa làm ở lượt này | … |
| Build, engine, commit và config dùng để kiểm | … |
| Chương TDD và trang PDF tham chiếu | … |
| Chương source/lab trong ReadyOrNot-Docs để đối chiếu | … |

Viết mục tiêu bằng hành vi, ví dụ “ra lệnh cho hai SWAT stack trước cửa rồi hủy mà không để ai bị kẹt”. Đừng chỉ ghi “làm command system”; nó không cho biết khi nào bạn đã hoàn thành.

## Bằng chứng và những điều chưa biết

| Nhận định cần dùng | Loại bằng chứng | Địa chỉ cụ thể | Giới hạn |
|---|---|---|---|
| Cơ chế gốc được TDD mô tả | TDD | §…, trang…; symbol… | Snapshot trong PDF |
| Nhánh đã đọc lại trong source | SOURCE | Path, symbol, dòng/hash… | Chưa chạy / đã chạy… |
| Giá trị tuning | CONFIG / CDO | Asset/config/override… | Default hay effective value? |
| Hành vi đang chạy | RUNTIME | Build, fixture, input, log… | Chỉ những ca này… |
| Giải pháp tự thiết kế | PROPOSED | Quyết định và lý do… | Cần phép kiểm… |
| Chưa xác định | UNKNOWN | Câu hỏi cần giải… | Không dùng như sự thật |

Nếu một nhận định chỉ có trong PDF, giữ nhãn TDD dù bên cạnh có đường dẫn source. Một line number là địa chỉ để kiểm lại; nó không chứng minh file trong project hôm nay vẫn giống lần khảo sát gốc.

## Owner và state

| State | Owner và vòng đời | Ai được đổi? | Ai chỉ đọc? | Reset khi nào? |
|---|---|---|---|---|
| … | … | … | … | … |
| … | … | … | … | … |

Ghi rõ state thuộc một người, cả đội, một world, một session hay profile lưu lâu dài. Tuning trong DataAsset và state runtime của từng AI nên được phân biệt. Nếu hai agent dùng cùng asset, chạy bài thử để xác nhận state của người này không làm đổi người kia.

## Hợp đồng action

| Pha | Điều cần ghi |
|---|---|
| Request | Input/event gì, actor/target nào, ID nào? |
| Validate | Điều kiện khoảng cách, state, vật cản, tool, authority? |
| Begin | Khóa những action nào, dựng feedback gì? |
| Commit | State nào thay đổi, thời điểm nào, cách chống commit trùng? |
| Complete | Mở khóa gì, phát event nào cho UI/AI/mục tiêu? |
| Cancel trước commit | Dọn timer/VO/animation thế nào, state trở về đâu? |
| Cancel sau commit | Giữ kết quả gì, bỏ phần trình diễn nào? |
| Target mất hiệu lực | Actor destroy, death, travel hoặc đổi đội được xử lý ra sao? |

Vẽ state machine của riêng feature nếu có nhiều nhánh. Với lệnh đội, bổ sung state của cả lệnh và state từng đội viên; không dùng một boolean “xong” để thay cho tất cả các giai đoạn.

## Dữ liệu và tài nguyên

| Vai trò | File/asset/config | Default | Runtime override | Kiểm tra tương thích |
|---|---|---|---|---|
| Luật và điều kiện | … | … | … | … |
| Mesh, skeleton, socket | … | … | … | … |
| Animation, montage, notify | … | … | … | … |
| Audio, routing, subtitle | … | … | … | … |
| VFX, material, camera | … | … | … | … |
| Widget và nguồn dữ liệu | … | … | … | … |
| Level, room/nav/cover/bake | … | … | … | … |

Nếu dùng fallback, ghi rõ cái gì bị mất. Một montage khác giúp action chạy được nhưng chưa chứng minh đúng nhịp nguồn. Một widget hard-code giúp kiểm layout nhưng chưa chứng minh binding đúng. Bài kiểm asset và bài kiểm hành vi cần kết quả riêng.

## Phụ thuộc và sự kiện

Ghi chuỗi tối thiểu theo mẫu: **producer → event/data → consumer → kết quả**. Cho mỗi event, xác định nó là request, state change hay cosmetic feedback. Khi có mạng, ghi ai phát, ai nhận, có thể lặp/mất/đến muộn không và xử lý thế nào.

Ví dụ đề xuất: `Arrest commit → PersonSecured(personId) → Objective tracker → tăng đúng một người đã xử lý`. Widget chỉ đọc trạng thái objective; animation không tự tăng điểm lần nữa. Cùng personId sau một request trùng phải có kết quả nhất quán.

## Ma trận thử

| Ca | Điều kiện đầu | Input | Kết quả mong đợi | Bằng chứng thực tế |
|---|---|---|---|---|
| Happy path | … | … | … | … |
| Bị từ chối đúng luật | … | … | … | … |
| Hủy trước commit | … | … | … | … |
| Hủy sau commit | … | … | … | … |
| Input lặp hoặc giữ | … | … | … | … |
| Target/owner bị mất | … | … | … | … |
| Reset và chơi lại | … | … | … | … |
| Hai người cùng thao tác | … | … | … | … |
| Server/client/observer | … | … | … | … |
| FPS/hitch trong phạm vi hỗ trợ | … | … | … | … |
| Fixture thiếu dữ liệu | … | … | … | … |
| Bản build ngoài Editor | … | … | … | … |

Chỉ dùng hàng phù hợp với phạm vi đã triển khai, nhưng đánh dấu N/A kèm lý do thay vì ghi PASS cho ca chưa chạy. Nếu feature phụ thuộc âm thanh thì phải nghe output thật; nếu phụ thuộc UI thì phải thử trạng thái empty/error/cancel ngoài ảnh đẹp mặc định.

## Bàn giao và học lại

- **Đã hoàn thành:** hành vi nào, ở build và cấu hình nào?
- **Đã giống nguồn tới đâu:** so với trang TDD, video hoặc bản lab nào; còn sai khác gì?
- **Chưa biết:** điều gì cần source, runtime hoặc asset bổ sung?
- **Hồi quy:** những hệ thống nào đã được kiểm sau thay đổi?
- **Artifact:** level/fixture, input script, log, manifest và ảnh/video nếu cần.
- **Điều rút ra:** vì sao bản trước sai, phép đo nào phân biệt được các giả thuyết?

Bạn đã hiểu feature khi có thể thay một điều kiện, dự đoán kết quả, chạy thử và giải thích vì sao kết quả khác dự đoán. Đó là năng lực có thể chuyển sang một game riêng, ngay cả khi tên class và bộ asset không còn giống Ready or Not.

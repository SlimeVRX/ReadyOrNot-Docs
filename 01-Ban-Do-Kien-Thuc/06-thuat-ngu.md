# Thuật ngữ đủ dùng để đọc bộ sách

| Thuật ngữ | Nghĩa thực hành trong sách | Nhầm lẫn thường gặp |
|---|---|---|
| Class / UClass | Kiểu đối tượng và implementation/reflection của nó | Coi nó là một khẩu súng đã spawn |
| Blueprint class | Lớp có thể kế thừa native/BP, chứa defaults và graph | Coi mọi logic đều nằm ở C++ |
| CDO | Class Default Object, điểm đọc defaults của lớp đã load | Coi default là state runtime cuối cùng |
| Instance | Đối tượng cụ thể của world/lượt chơi | Giữ instance qua travel như dữ liệu bất biến |
| Actor / Component | Thực thể world và phần chức năng gắn vào nó | Tách module đồng nghĩa tách ownership |
| Pawn / Controller | Thân thể được điều khiển và chủ thể quyết định input/AI | Cho UI làm thay authority của pawn |
| GameMode / GameState | Luật phía authoritative và trạng thái chia sẻ của phiên | Cho mọi client tự quyết kết quả |
| GameInstance / PlayerState | Dịch vụ qua world và state người chơi theo lifetime riêng | Ghi mọi dữ liệu vào một singleton |
| Owner | Nơi chịu trách nhiệm quyết định và giữ state | Nhầm với mọi nơi đang có pointer |
| Authority | Quyền quyết định kết quả gameplay trong network model | “Có actor trên client” nghĩa là client được gây damage |
| Owning client | Client điều khiển đối tượng/người chơi tương ứng | Mọi client đều được gọi cùng một Server RPC |
| Replication / OnRep | Đồng bộ state và phản ứng khi giá trị nhận thay đổi | Event chắc chắn đến đúng lúc như local function |
| RPC | Lời gọi đi qua ranh giới máy/vai trò theo luật Unreal | Thay được mọi state synchronization |
| Commit | Điểm hậu quả đã được chấp nhận | Action bắt đầu là đã hoàn tất |
| Cancel / rollback | Ngắt phần còn lại / hoàn tác phần được phép | Hủy montage tự hoàn tác damage đã gây |
| HIP / ADS | Tư thế bắn không ngắm / ngắm qua hệ sights | Chỉ khác một giá trị FOV |
| Recoil / spread | Chuyển động/phản hồi giật / phân bố hướng bắn | Giảm camera pitch sẽ sửa mọi sai lệch điểm chạm |
| Hitscan / projectile | Truy vấn hit theo trace / đối tượng hoặc mô phỏng viên đạn qua thời gian | Một game chỉ có đúng một cơ chế cho mọi item |
| Fire interval | Khoảng thời gian giữa các lần bắn được định thời | Property FireRate luôn là RPM |
| Magazine / chamber | Kho đạn/băng và trạng thái viên sẵn sàng, theo mô hình implementation | Luôn có hai UObject tách biệt mang hai tên này |
| Skeleton / socket | Cấu trúc xương và điểm gắn trong hierarchy | Cùng tên socket là transform/scale chắc chắn đúng |
| AnimGraph / montage / notify | Đồ thị pose / action theo timeline / sự kiện trên animation | Có clip đúng là toàn bộ animation pipeline đúng |
| Additive base | Pose tham chiếu để áp delta animation | Áp delta lên bất kỳ idle nào cũng giống nhau |
| CDO export | Đọc dữ liệu authored của lớp đã load | Một phép benchmark súng ngoài đời hoặc runtime measurement |
| Fixture / lab | Môi trường cô lập biến để thử một nhóm hành vi | Chứng minh game hoàn chỉnh |
| Vertical slice | Lát chơi đầu-cuối nhỏ nhưng có các hệ liên quan | Danh sách prototype độc lập chưa tích hợp |
| Receipt | Bản ghi bước kiểm chứng, kết quả và phạm vi | Một dòng “passed” không có test case |
| Snapshot / provenance | Bộ nguồn cụ thể và đường truy xuất của bằng chứng | Trộn phiên bản/bản retail khác nhau vì cùng tên |
| DDC | Cache dữ liệu dẫn xuất, giảm công việc chuẩn bị khi tải lại | Nội dung authored gốc hoặc save game |
| Asset registry | Chỉ mục metadata của package/asset | Load được graph/CDO và dependency chỉ bằng tên |

Từ tiếng Anh được giữ khi nó là tên API hoặc từ khóa hữu ích để tìm source. Mục tiêu là bạn tìm được đường thực thi, không học thuộc một bảng thuật ngữ.

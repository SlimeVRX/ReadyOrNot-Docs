# Môi trường khảo sát và giới hạn đã thấy

Trang này ghi bối cảnh cụ thể của workspace nghiên cứu ngày 07/10/2026. Nó không phải công thức sửa chung cho mọi bản Unreal hoặc Ready or Not.

## Engine và build

Engine đi kèm là **5.3.2 custom**, Changelist 0, BranchName Unknown, IsLicenseeVersion 1. Lần chuẩn bị đầu chưa có UnrealEditor.exe và ShaderCompileWorker.exe. Hai target được build từ source đi kèm, dùng MSVC 14.36 và Windows SDK 10.0.22621.0.

SDK 10.0.26100.0 đã gây C4756 ở biểu thức INFINITY trong RenderCore. Chuyển sang SDK 22621 đã build qua lỗi đó, không cần sửa implementation RenderCore. Engine-local BuildConfiguration.xml giữ lựa chọn SDK. Đây là bằng chứng của tổ hợp toolchain này; không suy ra rằng mọi SDK mới luôn sai.

Metadata UnrealEditor.modules của hai plugin gốc ban đầu có thuộc tính read-only; bước WriteMetadata cuối cần bỏ thuộc tính trên chính các file generated metadata đó. Không cần mở quyền ghi toàn bộ cây source để xử lý vấn đề này.

## Dữ liệu Engine từng thiếu

Engine/Content vắng mặt ở lần kiểm tra đầu. Dữ liệu được khôi phục theo đúng Commit.gitdeps.xml đi kèm: 27.920 file, khoảng 1,86 GB, kiểm SHA-1 trước khi đưa vào vị trí thiếu. Không dùng Content từ một bản Engine khác để lấp vào.

Các con số này là lịch sử chuẩn bị môi trường. Repository Docs không chứa thư mục Content ấy hoặc công cụ tự ý lấy source game. Script lab giả định bạn đã có project mở được.

## Cấu hình SM4 cũ

Project từng chứa +TargetedRHIs=PCD3D_SM4. Khi custom UE 5.3 đọc danh sách platform shader, mục này dẫn đến platform không hợp lệ và crash ở DataDrivenShaderPlatformInfo. Chỉ dòng SM4 lỗi thời được bỏ, giữ lựa chọn DX12 và mục SM5. Cấu hình gốc được lưu cục bộ trong Saved/Codex/DefaultEngine.before-SM4-fix-20261007.ini.

Điều này giải thích tại sao build thành công nhưng Editor vẫn có thể crash trước khi vào map. Một receipt BUILD không thay cho runtime startup.

## Asset migration và âm thanh/camera

Các lần inspect/native startup có cảnh báo legacy CameraAnim không load được và một số asset reference không tìm thấy. FMOD integration cũng có thể tạo/validate các asset từ dữ liệu bank khi Editor mở. Vì vậy cần đọc log theo ca và thời điểm; một warning cũ không tự mô tả trạng thái sau integration.

Giữ ba câu hỏi riêng:

- Asset nào không load được trong ca cụ thể?
- Action nào thực sự dùng nó?
- Người chơi thấy/nghe sai gì trong điều kiện cố định?

Không tự sửa tất cả warning ngoài phạm vi, không thay bằng một camera shake giả rồi báo native parity đầy đủ. Test matrix của Gun Lab đánh dấu rõ phần visual/audio/manual chưa được xác nhận.

## Những việc bộ sách không tự chứng minh

Chưa có cơ sở để đồng nhất snapshot với bản retail mới nhất; chưa có chứng nhận 1:1 về gunfeel của mọi biến thể; một sân bắn không kiểm tra toàn bộ nhiệm vụ, ROE, Commander hoặc multiplayer. Dữ liệu CDO có thể khác runtime sau attachment/modifier. Bảng catalog không thay full animation/Blueprint graph audit.

Các giới hạn này không ngăn học từ source. Chúng chỉ ra chính xác tầng cần kiểm tra tiếp theo, giúp bạn tránh mất thời gian hiệu chỉnh sai nguyên nhân.

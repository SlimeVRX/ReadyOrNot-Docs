# Catalog: tìm đúng source, dữ liệu và cấu hình súng

Catalog phục vụ tra cứu và tái lập. Nó không thay cho diễn giải trong chương, và một bản ghi không tự chứng minh một tính năng đã chơi thử thành công.

## Chọn điều cần tìm

| Bạn cần | Nguồn tra cứu | Phạm vi |
|---|---|---|
| Một khẩu / biến thể / class | Bảng tương tác dưới đây và [weapons.json](weapons.json) | Metadata và CDO đã load, kèm phạm vi scan/exclusion |
| Quy mô snapshot | [snapshot.json](snapshot.json) | File counts, Engine Build.version, hash descriptor |
| File source và fingerprint | [source-manifest.json](source-manifest.json) | Đường dẫn, số dòng văn bản, SHA-256; không có nội dung source |
| Nhóm source | [source-groups.json](source-groups.json) | Quy mô theo thư mục, không phải graph dependency |
| Nội dung theo thư mục | [content-groups.json](content-groups.json) | Filesystem counts, không tự xác định class |
| Các map | [maps.json](maps.json) | .umap package paths, gồm sublevel/test/internal |
| Plugin của project | [project-plugins.json](project-plugins.json) | Descriptor, modules, toggles; không bao gồm mọi plugin Engine |
| Hai sơ đồ flow | [flow-reference.json](flow-reference.json) | Nhãn từ draw.io nhúng, không phải bằng chứng implementation |
| Tình trạng thực nghiệm | [Gun Lab](../05-Gun-Lab/README.md) | Generation/build/runtime receipts theo từng ca |

## Tra vũ khí

<WeaponCatalog />

Nhấn tên entry để mở JSON. Tên hiển thị có thể giống nhau giữa nhiều Blueprint. **Generated class/package path** mới là định danh cấu hình; lab_index là thứ tự trong manifest của phòng thử súng.

Catalog hiện quét metadata NativeParentClass toàn bộ /Game và load CDO các Blueprint thuộc họ BaseMagazineWeapon. Trong snapshot khảo sát có **102 entry**, gồm bản player, suspect, nonlethal, launcher, breaching và một bản dùng cho test hiệu năng. Hai Blueprint melee khác họ súng có băng đạn được ghi trong coverage/exclusion. Không rút gọn thành “102 mẫu súng bán lẻ”.

## Đọc các field quan trọng

| Field / nhóm | Cách hiểu |
|---|---|
| asset_path / class_path | Package Blueprint và generated class; dùng để mở/tải đúng entry |
| parent_class / native_parent | Cây kế thừa qua metadata và lớp native, không phải category tên thương mại |
| leaf / child count nếu có | Có Blueprint con hay không; parent vẫn có thể spawn, cần phân biệt bản template và variant |
| values.item_name / item_class | Defaults authored để hiển thị/phân nhóm; giữ enum identifier để tìm source |
| values.fire_rate | Khoảng timer theo đường source đã phân tích, đơn vị giây; không phải RPM trực tiếp |
| values.ammo_max | Default của class; không đồng nhất với ammo runtime/chamber hoặc mọi loại magazine |
| animation_data / sound_data | Asset reference, không chứng minh mọi nhánh action/âm thanh đã chơi đúng |
| ammunition_types | Các tên ammo row/loại đạn authored; việc row load và dùng đúng cần kiểm tra riêng |
| recoil / ADS / firemode values | Defaults có thể chịu modifier từ attachment, posture, input và runtime state |
| spawnable_candidate | Được xem là ứng viên để thử; đọc runtime receipt để biết ca đã kiểm chứng |

Các trường null, thiếu hoặc không expose được là **chưa đọc được/không có giá trị theo phép đọc ấy**. Không biến null thành 0, và không điền bằng thông số ngoài đời hoặc một bản retail khác.

## Dữ liệu asset class so với filesystem

File .uasset có thể chứa nhiều loại asset; AssetRegistry cho biết class của asset đã discover. Vì vậy tổng AssetRegistry /Game có thể khác số file .uasset cộng .umap: registry có redirector, generated integration data, scan timing và nội dung do Editor tạo. Script filesystem và script CDO không phải một phép đo, phải đọc scope riêng trong JSON.

## Cập nhật catalog

Chạy script từ repo Docs:

```powershell
node scripts/catalog.mjs D:/Zone9Dev_RON
```

Script chỉ đọc project/Engine và ghi metadata trong 06-Catalogs. Export CDO/native weapon cần custom Editor và script được nêu ở [Gun Lab](../05-Gun-Lab/README.md). Sau export, build lại site; không chỉnh tay một số trong JSON rồi coi nó là kết quả extraction.

Đọc [phạm vi nguồn](01-nguon-va-phuong-phap.md) và [môi trường/giới hạn](02-moi-truong-va-gioi-han.md) trước khi so với game trên máy khác.

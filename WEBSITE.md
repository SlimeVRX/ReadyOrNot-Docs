# Build, cập nhật và xuất bản website

Website dùng VitePress 1.6.4, Mermaid và tìm kiếm toàn văn local. Cách tổ chức gần Fortnite-Docs/Paldark-Docs: Markdown là nội dung gốc, sidebar được tạo từ các chương, website build thành file tĩnh trên GitHub Pages.

## Build cục bộ

```powershell
Set-Location D:/Zone9Dev_RON/ReadyOrNot-Docs
npm.cmd ci
npm.cmd run docs:build
npm.cmd run docs:check
npm.cmd run docs:preview
```

Node.js 22 trở lên. File lock giữ phiên bản dependency; dùng npm ci cho lần tái lập. Thư mục .site là bản đọc công khai được chuẩn bị từ nội dung gốc; .vitepress/dist là artifact website. Không chỉnh tay hai thư mục generated.

Nếu build lại khi preview đang chạy, dừng preview rồi chạy lại trước khi kiểm tra. Preview có thể giữ danh sách file của build trước; lúc đó trang HTML mở được nhưng JavaScript hoặc sơ đồ mới chưa tải đúng.

## Khi cập nhật nội dung

Sửa chương có owner rõ. Nếu thêm feature, thêm source evidence, scope/unknowns và bài thực hành. Tìm kiếm trong sách phụ thuộc văn bản đã build; sơ đồ Mermaid cần render thử ở cả desktop và màn hình hẹp.

Nếu thay source hoặc asset, chạy lại script kiểm kê/CDO đúng phép đọc. Không chỉnh số trong catalog để làm khớp một câu mô tả cũ. Source line references cần đối chiếu symbol/hash sau thay đổi.

## Xuất bản

Repository đích là SlimeVRX/ReadyOrNot-Docs. GitHub Pages dùng GitHub Actions; workflow ở .github/workflows/deploy.yml. Mỗi push main chạy npm ci, build, kiểm link/asset và deploy artifact. Site base là /ReadyOrNot-Docs/.

Chỉ commit Markdown, metadata JSON, theme/build scripts, workflow và original lab harness. .gitignore loại binary game/Engine và output build local. Kiểm git diff trước push, đặc biệt khi copy một receipt mới từ Saved: không đưa access token, credential, full log chứa thông tin máy hoặc asset binary vào repo.

## Các tầng QA

1. Source/citation kiểm trong workspace có project.
2. Markdown/link và build static kiểm trong repo Docs.
3. Browser kiểm navigation, search, Mermaid, knowledge map, weapon catalog và responsive.
4. Unreal kiểm generation/build/runtime ở project riêng.
5. Người dùng kiểm cảm giác và visual/audio bằng test matrix.

Một workflow xanh chỉ chứng minh các bước được cấu hình trong workflow. Nó không tự chạy Unreal hoặc xác nhận gunfeel.

[Biên bản kiểm tra website](06-Catalogs/website-validation.json) ghi revision, deployment, kiểm link và các tương tác đã thử. Các ca Unreal được lưu riêng ở phần Gun Lab.

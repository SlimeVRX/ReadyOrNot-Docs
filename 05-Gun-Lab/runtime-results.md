# Kết quả runtime theo từng vũ khí

> Báo cáo được sinh từ các receipt công khai có cùng fingerprint plugin, gameplay DLL, map và camera mapping. Các giá trị là quan sát runtime, không phải đánh giá cảm giác bắn.

Equip run: **20261008-033324**. Action run: **multipart-20261008-041035-a61efecc**. Map: `ReadyOrNot_GunLab`.

Catalog có **102 Blueprint/variant**. Equip audit ghi **102/102 mục**; action probe ghi **101/101 mục dự kiến**. **1 lớp abstract** không được yêu cầu chạy action.

**Độ phủ batch: đủ hàng dự kiến.** Điều này không có nghĩa mọi súng đã bắn/reload thành công.

Số lab trong bảng bắt đầu từ **1**; `index` trong receipt bắt đầu từ **0**. Script kiểm tra cả index lẫn đường dẫn class trước khi ghép dữ liệu. Một variant không tương đương một mẫu súng độc lập.

## Tổng hợp theo đường chọn súng

| Nhóm | Số mục | Có action | Có tiêu đạn | Không tiêu đạn | ADS có | Reload tăng đạn | Reload chưa tăng đạn |
|---|---:|---:|---:|---:|---:|---:|---:|
| Loadout native | 40 | 40 | 40 | 0 | 40 | 34 | 6 |
| Asset catalog, gồm lớp dùng projectile riêng | 59 | 59 | 59 | 0 | 59 | 29 | 30 |
| Catalog: chưa đủ dữ liệu của cấu hình này | 2 | 2 | 1 | 1 | 2 | 0 | 2 |
| Lớp abstract được loại khỏi action probe | 1 | 0 | 0 | 0 | 0 | 0 | 0 |

## Nguồn của batch ghép nhiều phiên

Action receipt trên là **aggregate của 4 phiên native**, không phải một lần chơi liên tục. **World được tạo mới giữa các phiên.** Trạng thái inventory, mục tiêu và animation không được giữ xuyên qua ranh giới đó.

Wrapper chỉ nhận các cặp equip + action đã hoàn tất, kiểm tra class/index và fingerprint đầu vào. Receipt native từng phiên giữ nguyên; các hàng trong aggregate mang `source_run_id_utc`. Nếu một phiên kết thúc thiếu phần đuôi nhưng exit code 0 và không có lỗi tường minh, phiên mới đo phần còn lại. Điều này không chứng minh chuyển súng liên tục qua điểm dừng đã thành công.

Các asset suspect M37, SawnOff và Trenchgun có dữ liệu reload third-person nhưng thiếu player Reload_Start/Loop/End theo audit source/asset. Trạng thái chặn sau reload là nguyên nhân cần đối chiếu; receipt hiện tại không ghi trực tiếp cờ blocking tại thời điểm kết thúc. Reset world giúp thu thập phần đuôi, không sửa hay chứng minh holster/reload của các asset đó.

| Phiên nguồn | Receipt native / SHA-256 | Lab # đã lấy | Số hàng nhận | Exit code | Phạm vi |
|---|---|---|---:|---|---|
| 20261008-033759 | `action_probe_20261008-033759.json`<br>`0AEE225C91BA621F8DCA361F339BE164502ED00EB0BD3AA20987A193ABFBAE1F` | 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85 | 168 | Chưa ghi trong receipt nguồn | Receipt partial cung cấp để tiếp tục; exit code không có trong native JSON |
| 20261008-041150 | `action_probe_20261008-041150.json`<br>`49D236B4E6EC6DD8748C17A97EE013EF162A5ADCFD906456D60CE3C6E310CE26` | 86, 87, 88, 89, 90, 91 | 12 | 0 | Phiên wrapper: kết thúc thiếu phần yêu cầu; tiếp tục ở world mới |
| 20261008-041433 | `action_probe_20261008-041433.json`<br>`ACDF8FAB18BBE4FC32E610707F5C290B9A191A308D24780BCC52675BD06D6F8C` | 92 | 2 | 0 | Phiên wrapper: kết thúc thiếu phần yêu cầu; tiếp tục ở world mới |
| 20261008-041611 | `action_probe_20261008-041611.json`<br>`2CC5F68342938BF5B8615AD0EA278CDFEDDC32E0C1EDA4BF871D18C4B796BCD8` | 93, 94, 95, 96, 97, 98, 99, 100, 101, 102 | 20 | 0 | Phiên wrapper: đủ phần yêu cầu |

SHA-256 ở đây định danh file native cục bộ đã được wrapper kiểm tra trước khi ghép; trang công khai không suy ra exit code còn thiếu và không biến tổng thời gian của nhiều world thành một timeline liên tục.


“Có/Không” chỉ đếm boolean thực sự có trong receipt. Dữ liệu thiếu hiển thị **—**, không chuyển thành “Không” hoặc “Có”. Nhóm chưa đủ dữ liệu chỉ gồm hai cấu hình đã được audit riêng: S590 Beanbag V2 và Flaregun. Một mảng AmmunitionTypes trống tự nó không đủ để kết luận asset thiếu nội dung; các launcher dùng cấu hình projectile vẫn được giữ trong nhóm catalog.

## Cách đọc các cột

- **Equip A/P:** kết quả equip trong lượt audit / lượt action; “instant” dùng nhánh native bỏ qua holster của asset đang rời, không chứng minh holster thường.
- **Đạn trước → sau → cuối:** trước bắn, khi thả bắn sau khoảng 0,2 giây, cuối probe sau yêu cầu reload. Burst hoặc phát bắn đang chờ có thể tiếp tục sau mốc “sau”.
- **ADS:** trạng thái aiming trước lệnh bắn. **Reload yêu cầu / cho phép / tăng đạn:** lần lượt là lệnh reload đã gọi, CanReload trước lệnh, và đạn cuối lớn hơn đạn ở mốc sau bắn.
- “Không tăng đạn” cần đối chiếu asset/animation và subclass; không tự đồng nghĩa toàn bộ reload logic bị lỗi. F7 refill không được dùng làm bằng chứng reload.
- `catalog_asset` dùng đường xem asset native. Ammo có giảm vẫn không chứng minh projectile, damage, camera, âm thanh hoặc chức năng player loadout hoàn chỉnh.

## Loadout native

| Lab # | Tên / class | Equip A/P | Đạn trước → sau → cuối | ADS | Reload yêu cầu / cho phép / tăng đạn | Mag cuối | Route / ghi chú |
|---:|---|---|---|---|---|---:|---|
| 5 | B1301<br>`Primary_B1301_C` | equipped / equipped | 9 → 8 → 8 | Có | Có / Có / Không | 1 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 6 | B1301 Entryman<br>`Primary_B1301_Entryman_C` | equipped / equipped | 9 → 8 → 8 | Có | Có / Có / Không | 1 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 7 | BCM<br>`Primary_BCM_MK1_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 8 | ARN-180<br>`Primary_BRN180_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 9 | M1014<br>`Primary_Benelli_M4_C` | equipped / equipped | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 10 | F90<br>`Primary_F90_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 12 | SA-58 OSW<br>`Primary_FAL_OSW_C` | equipped / equipped | 20 → 19 → 21 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 13 | G36C<br>`Primary_G36C_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 15 | GA416<br>`Primary_HK416_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 16 | LVAR<br>`Primary_LVAW_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 22 | MCX<br>`Primary_MCX_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 23 | MK18<br>`Primary_MK18_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 25 | MP5/10MM<br>`Primary_MP510_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 26 | MP5A2<br>`Primary_MP5A2_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 27 | MP5A3<br>`Primary_MP5A3_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 28 | MP7<br>`Primary_MP7_C` | equipped / equipped | 40 → 39 → 41 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 29 | MP9<br>`Primary_MP9_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 31 | MPX<br>`Primary_MPX_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 32 | P90<br>`Primary_P90_C` | equipped / equipped | 50 → 49 → 51 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 36 | ARWC<br>`Primary_SBR_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 37 | MK16<br>`Primary_SCARL_v2_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 38 | SLR47<br>`Primary_SLR47_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 39 | SPC9<br>`Primary_SPC9_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 40 | SR-16<br>`Primary_SR16_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 42 | Supernova<br>`Primary_Superbnova_C` | equipped / equipped | 9 → 8 → 8 | Có | Có / Có / Không | 1 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 44 | UMP-45<br>`Primary_UMP45_v2_C` | equipped / equipped | 25 → 24 → 26 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 45 | UMP-9<br>`Primary_UMP9_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 47 | VKS<br>`Primary_VKS_V2_C` | equipped / equipped | 25 → 24 → 25 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 48 | Beanbag Shotgun<br>`Primary_W870LL_C` | equipped / equipped | 7 → 6 → 6 | Có | Có / Có / Không | 1 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 49 | 870CQB<br>`Primary_WCQB_C` | equipped / equipped | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 50 | FiveSeven<br>`Secondary_FiveSeven_V2_C` | equipped / equipped | 20 → 19 → 21 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 54 | G19<br>`Secondary_G19_V2_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 55 | TLE 1911<br>`Secondary_Kimber1911_C` | equipped / equipped | 7 → 6 → 8 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 58 | M45A1<br>`Secondary_M45A1_C` | equipped / equipped | 7 → 6 → 8 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 59 | B92X<br>`Secondary_M92A3_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 61 | M11 Compact<br>`Secondary_P229_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 64 | PC19<br>`Secondary_PFC9_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 65 | .357 Magnum<br>`Secondary_Python_V2_C` | equipped / equipped | 6 → 5 → 6 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |
| 66 | TPL<br>`Secondary_TCR_C` | equipped / equipped | 12 → 11 → 12 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_primary_menu_with_authored_player_ammo |
| 68 | USP45<br>`Secondary_USP_C` | equipped / equipped | 12 → 11 → 13 | Có | Có / Có / Có | 3 | Loadout native; Nguồn action: 20261008-033759; native_secondary_menu_with_authored_player_ammo |

## Asset catalog, gồm lớp dùng projectile riêng

| Lab # | Tên / class | Equip A/P | Đạn trước → sau → cuối | ADS | Reload yêu cầu / cho phép / tăng đạn | Mag cuối | Route / ghi chú |
|---:|---|---|---|---|---|---:|---|
| 1 | 870mcs<br>`Primary_870mcs_C` | equipped / equipped | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 2 | AK102<br>`Primary_AK102_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 3 | AK103<br>`Primary_AK103_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 4 | AR18<br>`Primary_AR18_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 11 | FAL<br>`Primary_FAL_C` | equipped / equipped | 20 → 19 → 21 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 14 | G36C<br>`Primary_G3A3_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 17 | M14<br>`Primary_M14_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 18 | M16A4<br>`Primary_M16A4_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 19 | M4A1<br>`Primary_M4A1_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 20 | M4A1<br>`Primary_M4A1_D_Temp_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 21 | M76<br>`Primary_M76_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 24 | MK18 Simmunition<br>`Primary_MK18_Blue_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 30 | MPL<br>`Primary_MPL_C` | equipped / equipped | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 33 | Pepperball Gun<br>`Primary_Pepperball_MLO_C` | equipped / equipped | 200 → 199 → 199 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 34 | 590S<br>`Primary_S590_Assault_v2_C` | equipped / equipped | 7 → 6 → 6 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 41 | Saiga<br>`Primary_Saiga12_C` | equipped / equipped | 10 → 9 → 11 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 43 | TAC700<br>`Primary_TAC700_C` | equipped / equipped | 200 → 199 → 200 | Có | Có / Có / Có | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 46 | Pepperball Gun<br>`Primary_VKS_C` | equipped / equipped | 15 → 14 → 15 | Có | Có / Có / Có | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 51 | G18<br>`Secondary_G18_C` | equipped / equipped | 19 → 18 → 20 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 52 | G19<br>`Secondary_G19_Blue_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 53 | G19<br>`Secondary_G19_D_Temp_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 56 | M1911<br>`Secondary_M1911_V2_C` | equipped / equipped | 7 → 6 → 8 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 57 | M2011<br>`Secondary_M2011_C` | equipped / equipped | 7 → 6 → 8 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 60 | M9A1<br>`Secondary_M92FS_V2_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 62 | P250<br>`Secondary_P250_C` | equipped / equipped | 13 → 12 → 14 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 63 | G19<br>`Secondary_P99_C` | equipped / equipped | 15 → 14 → 16 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 67 | Taser<br>`Secondary_Taser_V2_C` | equipped / equipped | 1 → 0 → 1 | Có | Có / Có / Có | 8 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 69 | M32A1<br>`Deployable_M32A1_C` | equipped / equipped | 6 → 5 → 5 | Có | Có / Có / Không | 4 | Xem asset catalog; Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng; Native GrenadeLauncher dùng cấu hình projectile; không suy ra thiếu nội dung từ ammo-row trống; Audit riêng M32A1: AnimData không có Reload/ReloadEmpty/Tactical/crouch reload hoặc import montage reload; 36 animation M32 không có reload. GrenadeLauncher kế thừa BaseMagazineWeapon, không override reload; NextMagazine cần notify authored. Đối chiếu số đo Capture riêng, không gộp M320 vào giới hạn này; Nguồn action: 20261008-033759; missing_authored_ammunition_types; catalog inspection only |
| 70 | Breach Shotgun<br>`Device_870MCS_Breach_C` | equipped / equipped | 4 → 3 → 3 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 71 | Breach Shotgun<br>`Device_S590_Breach_v2_C` | equipped / equipped | 4 → 3 → 3 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 72 | S590 Breach<br>`Device_SuperShorty_C` | equipped / equipped | 5 → 4 → 4 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 73 | Taser<br>`Device_Taser7_C` | equipped / equipped | 2 → 1 → 2 | Có | Có / Có / Có | 12 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 75 | M320 Flash<br>`Launcher_M320_Bang_C` | equipped / equipped | 1 → 0 → 1 | Có | Có / Có / Có | 3 | Xem asset catalog; Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng; Native GrenadeLauncher dùng cấu hình projectile; không suy ra thiếu nội dung từ ammo-row trống; Nguồn action: 20261008-033759; missing_authored_ammunition_types; catalog inspection only |
| 76 | M320 Gas<br>`Launcher_M320_Gas_C` | equipped / equipped | 1 → 0 → 1 | Có | Có / Có / Có | 3 | Xem asset catalog; Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng; Native GrenadeLauncher dùng cấu hình projectile; không suy ra thiếu nội dung từ ammo-row trống; Nguồn action: 20261008-033759; missing_authored_ammunition_types; catalog inspection only |
| 77 | M320 Stinger<br>`Launcher_M320_Stinger_C` | equipped / equipped | 1 → 0 → 1 | Có | Có / Có / Có | 3 | Xem asset catalog; Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng; Native GrenadeLauncher dùng cấu hình projectile; không suy ra thiếu nội dung từ ammo-row trống; Nguồn action: 20261008-033759; missing_authored_ammunition_types; catalog inspection only |
| 78 | M24<br>`Sniper_M24_C` | equipped / equipped | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 79 | AK-104<br>`Primary_AK104_C` | equipped / equipped | 30 → 29 → 29 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 80 | AKM<br>`Primary_AKM_C` | instant / instant | 30 → 29 → 29 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 81 | AKS-74U<br>`Primary_AKS74U_C` | instant / instant | 30 → 29 → 29 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 82 | Custom AR<br>`Primary_CAR_C` | instant / instant | 30 → 29 → 29 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 83 | M960<br>`Primary_Calico_C` | instant / instant | 80 → 79 → 79 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 84 | M249<br>`Primary_M249_C` | instant / instant | 100 → 99 → 99 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-033759; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 85 | M37<br>`Primary_M37_C` | instant / instant | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Xem asset catalog; Asset suspect: có reload third-person nhưng thiếu player Reload_Start/Loop/End; một lượt mới reset world không chứng minh transition liên tục khỏi súng này; Nguồn action: 20261008-033759; no_native_player_usable_ammunition; catalog inspection only |
| 86 | AKS74U<br>`Primary_QBZ_C` | instant / equipped | 30 → 29 → 29 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041150; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 87 | RPD<br>`Primary_RPD_C` | instant / instant | 100 → 99 → 99 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041150; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 88 | 870mcs<br>`Primary_S_870mcs_C` | instant / instant | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-041150; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 89 | AK-103<br>`Primary_S_AK103_C` | equipped / equipped | 30 → 29 → 29 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041150; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 90 | G3A3<br>`Primary_S_G3A3_C` | instant / instant | 30 → 29 → 31 | Có | Có / Có / Có | 3 | Xem asset catalog; Nguồn action: 20261008-041150; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 91 | Sawed-Off<br>`Primary_SawnOff_C` | equipped / equipped | 2 → 1 → 1 | Có | Có / Có / Không | 1 | Xem asset catalog; Asset suspect: có reload third-person nhưng thiếu player Reload_Start/Loop/End; một lượt mới reset world không chứng minh transition liên tục khỏi súng này; Nguồn action: 20261008-041150; no_native_player_usable_ammunition; catalog inspection only |
| 92 | M37<br>`Primary_Trenchgun_C` | instant / equipped | 8 → 7 → 7 | Có | Có / Có / Không | 1 | Xem asset catalog; Asset suspect: có reload third-person nhưng thiếu player Reload_Start/Loop/End; một lượt mới reset world không chứng minh transition liên tục khỏi súng này; Nguồn action: 20261008-041433; no_native_player_usable_ammunition; catalog inspection only |
| 93 | UZI<br>`Primary_UZI_C` | instant / equipped | 30 → 27 → 27 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 95 | Makarov<br>`Secondary_Makarov_C` | instant / instant | 12 → 11 → 11 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 96 | Makarov<br>`Secondary_Makarov_2Handed_C` | instant / instant | 12 → 11 → 11 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 97 | G18<br>`Secondary_S_G18_C` | instant / instant | 19 → 18 → 18 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 98 | G19<br>`Secondary_S_G19_C` | instant / instant | 15 → 14 → 14 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 99 | M1911<br>`Secondary_S_M1911_C` | instant / instant | 7 → 6 → 6 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 100 | Colt Python<br>`Secondary_S_Python_C` | instant / instant | 6 → 5 → 5 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 101 | TEC-9<br>`Secondary_Tec9_C` | instant / instant | 32 → 31 → 31 | Có | Có / Có / Không | 4 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |
| 102 | 870mcs<br>`BP_AutoShotgunForPerfTest_C` | instant / instant | 10000 → 9997 → 9982 | Có | Có / Có / Không | 1 | Xem asset catalog; Nguồn action: 20261008-041611; not_in_native_primary_or_secondary_menu; catalog inspection only |

## Catalog: chưa đủ dữ liệu của cấu hình này

| Lab # | Tên / class | Equip A/P | Đạn trước → sau → cuối | ADS | Reload yêu cầu / cho phép / tăng đạn | Mag cuối | Route / ghi chú |
|---:|---|---|---|---|---|---:|---|
| 35 | 590A Less Lethal<br>`Primary_S590_Beanbag_V2_C` | equipped / equipped | 0 → 0 → 0 | Có | Có / Không / Không | 0 | Xem asset catalog; Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng; Cấu hình player đã được audit riêng: chưa đủ dữ liệu; không xác nhận trải nghiệm bắn hoàn chỉnh; Nguồn action: 20261008-033759; missing_authored_ammunition_types; catalog inspection only |
| 94 | Flaregun<br>`Secondary_Flaregun_C` | instant / instant | 1 → 0 → 0 | Có | Có / Có / Không | 4 | Xem asset catalog; Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng; Cấu hình player đã được audit riêng: chưa đủ dữ liệu; không xác nhận trải nghiệm bắn hoàn chỉnh; Nguồn action: 20261008-041611; missing_authored_ammunition_types; catalog inspection only |

## Lớp abstract được loại khỏi action probe

| Lab # | Tên / class | Equip A/P | Đạn trước → sau → cuối | ADS | Reload yêu cầu / cho phép / tăng đạn | Mag cuối | Route / ghi chú |
|---:|---|---|---|---|---|---:|---|
| 74 | M320<br>`Launcher_M320_C` | class_rejected / — | — → — → — | — | — / — / — | — | Không chạy action; Abstract base class; use a concrete child Blueprint |

## Đối chiếu headless và có renderer trên tập được thử lại

Batch toàn bộ phía trên là **Mode Probe: `-nullrhi -nosound`**, aggregate nhiều world **multipart-20261008-041035-a61efecc**. Lượt đối chiếu là **Mode Capture: có renderer, `-nosound`**, run **20261008-040637**. Lượt Capture chỉ có **16 mục được chọn**, với **16 hàng action**; không thay thế hay ghi đè các quan sát của batch toàn bộ.

Các Blueprint/animation native có thể phụ thuộc mesh được render và animation notify. Cần đọc hai lượt cạnh nhau trước khi kết luận reload hỏng. Đây là đối chiếu quan sát; khác biệt thứ tự equip hoặc trạng thái đầu vào giữa hai lượt chưa được loại trừ, nên không quy mọi khác biệt cho renderer.

Trong tập này, Capture ghi nhận tiêu đạn ở **16/16 mục**, tăng đạn sau yêu cầu reload ở **13/16 mục**. **12 mục** không tăng đạn ở Probe nhưng tăng ở Capture; **3 mục** đều chưa tăng đạn ở cả hai lượt. Đây không phải số “súng hoạt động hoàn chỉnh”.

Cột reload vẫn là **yêu cầu / cho phép / tăng đạn**. Cả hai lượt đều tắt âm thanh; Capture có renderer không tự chứng minh chất lượng animation, cảm giác bắn hoặc âm thanh.

| Lab # / class | Đạn Probe: trước → sau → cuối | Đạn Capture: trước → sau → cuối | ADS Probe / Capture | Reload Probe | Reload Capture | Ghi chú |
|---|---|---|---|---|---|---|
| 1 — 870mcs<br>`Primary_870mcs_C` | 8 → 7 → 7 | 8 → 7 → 8 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 5 — B1301<br>`Primary_B1301_C` | 9 → 8 → 8 | 9 → 8 → 9 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 6 — B1301 Entryman<br>`Primary_B1301_Entryman_C` | 9 → 8 → 8 | 9 → 8 → 9 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 9 — M1014<br>`Primary_Benelli_M4_C` | 8 → 7 → 7 | 8 → 7 → 8 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 33 — Pepperball Gun<br>`Primary_Pepperball_MLO_C` | 200 → 199 → 199 | 200 → 199 → 199 | Có / Có | Có / Có / Không | Có / Có / Không | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 34 — 590S<br>`Primary_S590_Assault_v2_C` | 7 → 6 → 6 | 7 → 6 → 7 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 42 — Supernova<br>`Primary_Superbnova_C` | 9 → 8 → 8 | 9 → 8 → 9 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 43 — TAC700<br>`Primary_TAC700_C` | 200 → 199 → 200 | 200 → 199 → 200 | Có / Có | Có / Có / Có | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 48 — Beanbag Shotgun<br>`Primary_W870LL_C` | 7 → 6 → 6 | 7 → 6 → 7 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 49 — 870CQB<br>`Primary_WCQB_C` | 8 → 7 → 7 | 8 → 7 → 8 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 69 — M32A1<br>`Deployable_M32A1_C` | 6 → 5 → 5 | 6 → 5 → 5 | Có / Có | Có / Có / Không | Có / Có / Không | Equip Capture: equipped; Nguồn Probe: 20261008-033759; M32A1 thiếu dữ liệu reload theo audit riêng; M320 có reload authored, không chung giới hạn này |
| 70 — Breach Shotgun<br>`Device_870MCS_Breach_C` | 4 → 3 → 3 | 4 → 3 → 4 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 71 — Breach Shotgun<br>`Device_S590_Breach_v2_C` | 4 → 3 → 3 | 4 → 3 → 4 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 72 — S590 Breach<br>`Device_SuperShorty_C` | 5 → 4 → 4 | 5 → 4 → 5 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 78 — M24<br>`Sniper_M24_C` | 8 → 7 → 7 | 8 → 7 → 8 | Có / Có | Có / Có / Không | Có / Có / Có | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |
| 85 — M37<br>`Primary_M37_C` | 8 → 7 → 7 | 8 → 7 → 7 | Có / Có | Có / Có / Không | Có / Có / Không | Equip Capture: equipped; Nguồn Probe: 20261008-033759 |

[Receipt riêng của lượt Capture](../lab/manifests/rendered_confirmation_receipt.json). Script xác nhận class/index và fingerprint plugin, gameplay DLL, map, camera mapping trùng với batch toàn bộ.

SHA-256 rendered confirmation receipt: `77c155506b2fe5664aca281f280423d820ac8653ceafcfba0ccd592a1d7e3928`.

## Phạm vi bằng chứng

Báo cáo này không xác nhận cảm giác bắn, âm thanh nghe được, hình ảnh/animation đúng theo thời gian, damage/hit feedback, hoặc tương đương bản retail. Batch action thường chạy với `-nullrhi -nosound`. Các kiểm tra trải nghiệm có renderer, fixture, camera và input nằm trong [receipt trải nghiệm](../lab/manifests/experience_receipt.json) và [báo cáo assertion](../lab/manifests/experience_verification.json); chúng cũng có giới hạn riêng.

Không gán loại đạn giả cho asset chưa hoàn chỉnh. `Primary_S590_Beanbag_V2` và `Secondary_Flaregun` được audit riêng là cấu hình player chưa đầy đủ; chúng hiện không khai báo `AmmunitionTypes`. Trải nghiệm beanbag player có sẵn nằm ở `Primary_W870LL` với đạn `12gaBeanbag`. Ngược lại, M32A1 và các M320 cụ thể là lớp `GrenadeLauncher` dùng dữ liệu projectile; ammo-row trống không phải bằng chứng rằng các launcher này hỏng.

## Nguồn và tái tạo báo cáo

- [Selection order](../lab/manifests/selection_order.json): tên, class và số lab bắt đầu từ 1.
- [Equip audit](../lab/manifests/equip_audit_receipt.json) và [action probe](../lab/manifests/action_probe_receipt.json): dữ liệu gốc.
- [Build receipt](../lab/manifests/build_receipt.json): fingerprint bản đã biên dịch.

Sau khi collect receipt của lượt cuối, chạy từ gốc repo:

```powershell
node lab/scripts/report_gunlab_results.mjs
```

Kiểm tra receipt cũ mà không ghi vào trang kết quả hiện tại:

```powershell
node lab/scripts/report_gunlab_results.mjs --history --stdout
```

SHA-256 equip receipt: `473089ec6e5c563bf6822052305e0463a9f2502de6250a6c10e5fc93de415dc6`.

SHA-256 action receipt: `9d6318916e4dfb68539f9d1e3acfa09028bc16871fc63349856ac916204f61ca`.

SHA-256 selection order: `7cbb5e315c5e31b7924196a0489ed36f560b613a7c367f0a05e3a1e438551567`.

Plugin: `6DCA3FA54BFCE51E67E619B154798FE6A29DA46F300F5519B5967145A1B13ED6`.

Gameplay DLL: `0CC3B7CE1703CEEC062B1EE5869B13FB39BBEAD7E1B18FC5951A85213D7CBF1C`.

Map: `36BBE5DF4B518C017ABC94A4A301276923E19BE2B2E77AF0E9FAF4FD5C72322A`.

Camera mapping: `B44CD72D8E8529350B71BB616752292489A40FF55D36F0140E3FDFBEA0D18166`.

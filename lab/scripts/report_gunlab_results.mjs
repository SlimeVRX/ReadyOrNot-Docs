#!/usr/bin/env node
// Render public instrumentation receipts; never launch Unreal or modify gameplay.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const defaultOutput = path.join(repo, '05-Gun-Lab/runtime-results.md');
const finite = value => typeof value === 'number' && Number.isFinite(value);
const bool = value => value === true ? 'Có' : value === false ? 'Không' : '—';
const num = value => finite(value) ? String(value) : '—';
const escape = value => String(value ?? '').replaceAll('|', '\\|').replace(/[\r\n]+/g, ' ').replaceAll('<', '&lt;').replaceAll('>', '&gt;');
const hash = raw => crypto.createHash('sha256').update(raw).digest('hex');
const read = filename => { const raw = fs.readFileSync(filename); return { data: JSON.parse(raw.toString('utf8').replace(/^\uFEFF/, '')), sha256: hash(raw) }; };
const require = (condition, message) => { if (!condition) throw new Error(message); };
const sameHash = (a, b) => typeof a === 'string' && /^[a-f0-9]{64}$/i.test(a) && typeof b === 'string' && a.toLowerCase() === b.toLowerCase();
const equipOK = row => ['equipped', 'equipped_instant_fallback'].includes(row?.status);
const abstractRow = row => row?.status === 'class_rejected' && /abstract/i.test(row.detail ?? '');
const routeName = route => route === 'native_loadout' ? 'Loadout native' : route === 'catalog_asset' ? 'Xem asset catalog' : 'Chưa ghi route';
// These two classes were independently audited as incomplete player setups.
// An empty ammo-row list alone is not enough: launchers may use projectile data.
const auditedIncompleteClasses = new Set([
  '/Game/Blueprints/Items/WeaponsRevised/Primary_S590_Beanbag_V2.Primary_S590_Beanbag_V2_C',
  '/Game/Blueprints/Items/WeaponsSuspect/Secondary_Flaregun.Secondary_Flaregun_C'
]);
const projectileCatalogClasses = new Set([
  'Deployable_M32A1', 'Launcher_M320_Bang', 'Launcher_M320_Gas', 'Launcher_M320_Stinger'
].map(name => `/Game/Blueprints/Items/WeaponsRevised/${name}.${name}_C`));
const suspectShotgunsWithoutPlayerReload = new Set(['Primary_M37', 'Primary_SawnOff', 'Primary_Trenchgun']
  .map(name => `/Game/Blueprints/Items/WeaponsSuspect/${name}.${name}_C`));

function validateReceipt(receipt, mode, catalog) {
  require(receipt?.mode === mode, `Expected ${mode} receipt.`);
  require(receipt.candidate_count === catalog.length, `${mode}: candidate_count differs from selection_order.`);
  require(Array.isArray(receipt.results), `${mode}: missing results array.`);
  const indexed = new Map();
  for (const row of receipt.results) {
    require(Number.isInteger(row.index) && row.index >= 0 && row.index < catalog.length, `${mode}: invalid zero-based index ${row.index}.`);
    require(row.class_path === catalog[row.index].class_path, `${mode}: class mismatch at lab #${row.index + 1}; refusing an index-only join.`);
    require(typeof row.status === 'string' && row.status, `${mode}: missing row status.`);
    if (!indexed.has(row.index)) indexed.set(row.index, []);
    indexed.get(row.index).push(row);
  }
  for (const [index, rows] of indexed) {
    const actions = rows.filter(row => row.status === 'action_probe');
    const equips = rows.filter(equipOK);
    require(actions.length <= 1 && equips.length <= 1, `${mode}: duplicate action/equip at lab #${index + 1}.`);
    if (mode === 'equip_audit') require(rows.length === 1 && actions.length === 0, `${mode}: expected one outcome per lab index.`);
    for (const row of actions) {
      if ([row.ammo_before, row.ammo_after_native_fire].every(finite) && typeof row.ammo_consumed === 'boolean')
        require(row.ammo_consumed === (row.ammo_after_native_fire < row.ammo_before), `${mode}: inconsistent ammo_consumed at lab #${index + 1}.`);
      if ([row.ammo, row.ammo_after_native_fire].every(finite) && typeof row.native_reload_requested === 'boolean' && typeof row.native_reload_replenished === 'boolean')
        require(row.native_reload_replenished === (row.native_reload_requested && row.ammo > row.ammo_after_native_fire), `${mode}: inconsistent reload observation at lab #${index + 1}.`);
    }
  }
  return indexed;
}

function validateMultipart(receipt, catalog) {
  const sources = receipt.source_runs;
  if (sources === undefined) {
    require(receipt.multipart !== true && !receipt.results.some(row => row.source_run_id_utc !== undefined), 'Multipart rows require a source_runs manifest.');
    return null;
  }
  require(receipt.multipart === true && receipt.complete === true, 'Multipart report requires an explicitly completed aggregate.');
  require(Array.isArray(sources) && sources.length > 0, 'Multipart source_runs must be nonempty.');
  require(receipt.world_reset_between_runs === (sources.length > 1), 'Multipart world-reset declaration does not match source run count.');
  const indices = (values, oneBased, label) => {
    require(Array.isArray(values) && values.length > 0 && values.every(i => Number.isInteger(i) && i >= (oneBased ? 1 : 0) && i < catalog.length + (oneBased ? 1 : 0)), `${label}: invalid indices.`);
    require(new Set(values).size === values.length, `${label}: duplicate indices.`);
    return [...values].sort((a, b) => a - b);
  };
  const sameIndices = (a, b) => JSON.stringify([...a].sort((x, y) => x - y)) === JSON.stringify([...b].sort((x, y) => x - y));
  const requested = indices(receipt.requested_indices_one_based, true, 'Multipart requested');
  const claimed = indices(receipt.completed_indices_zero_based, false, 'Multipart completed');
  require(sameIndices(requested.map(i => i - 1), claimed), 'Multipart requested/completed indices disagree.');
  const seenRuns = new Set(), seenIndices = new Set();
  for (const source of sources) {
    require(typeof source.run_id_utc === 'string' && /^\d{8}-\d{6}$/.test(source.run_id_utc) && source.run_id_utc !== receipt.run_id_utc && !seenRuns.has(source.run_id_utc), 'Multipart source run id is invalid or duplicated.');
    seenRuns.add(source.run_id_utc);
    require(source.receipt_file === `action_probe_${source.run_id_utc}.json`, 'Multipart source receipt filename does not match native run id.');
    require(typeof source.receipt_sha256 === 'string' && /^[a-f0-9]{64}$/i.test(source.receipt_sha256), 'Multipart source receipt hash is invalid.');
    require(['provided_native_resume_receipt', 'wrapper_started_native_session'].includes(source.source_kind), 'Unknown multipart source kind.');
    if (source.source_kind === 'wrapper_started_native_session') require(source.process_exit_code === 0, 'Wrapper-started multipart source did not exit zero.');
    else require(source.process_exit_code === null || source.process_exit_code === 0, 'Provided resume source has an explicit nonzero exit.');
    for (const field of ['tested_binary_sha256', 'tested_host_game_binary_sha256', 'tested_map_sha256', 'tested_camera_mapping_sha256'])
      require(sameHash(source[field], receipt[field]), `Multipart source input mismatch: ${field}.`);
    const completed = indices(source.completed_indices_zero_based, false, 'Source completed');
    const sourceRequested = indices(source.requested_indices_one_based, true, 'Source requested');
    require(completed.every(i => sourceRequested.includes(i + 1) && claimed.includes(i)), 'Multipart source includes an unrequested completed index.');
    require(sourceRequested.every(i => requested.includes(i)), 'Multipart source request is outside the aggregate request.');
    require(source.incomplete_for_requested_subset === (completed.length < sourceRequested.length), 'Multipart partial-source declaration disagrees with indices.');
    const sourceRows = receipt.results.filter(row => row.source_run_id_utc === source.run_id_utc);
    require(sourceRows.length === source.accepted_row_count && sourceRows.length === completed.length * 2 && Number.isInteger(source.source_row_count) && source.source_row_count >= sourceRows.length, 'Multipart source row counts are inconsistent.');
    require(sourceRows.every(row => completed.includes(row.index) && (equipOK(row) || row.status === 'action_probe')), 'Multipart source contains unrelated or failed rows.');
    require(sameIndices(sourceRows.filter(row => row.status === 'action_probe').map(row => row.index), completed), 'Multipart source membership disagrees with action rows.');
    require(sameIndices(sourceRows.filter(equipOK).map(row => row.index), completed), 'Multipart source membership disagrees with equip rows.');
    for (const index of completed) { require(!seenIndices.has(index), 'Multipart repeats an action index across sources.'); seenIndices.add(index); }
  }
  require(receipt.results.every(row => seenRuns.has(row.source_run_id_utc)), 'Aggregate row has no valid source_run_id_utc.');
  require(sameIndices([...seenIndices], claimed), 'Multipart source indices do not cover the aggregate.');
  return sources;
}

export function createGunLabReport({ selection, equip, probe, rendered, build, history = false, hashes = {} }) {
  require(Array.isArray(selection) && selection.length > 0, 'selection_order must be a nonempty array.');
  const catalog = [...selection].sort((a, b) => a.index - b.index);
  require(catalog.every((row, index) => row.index === index + 1 && typeof row.class_path === 'string' && row.class_path), 'selection_order indices must be contiguous and one-based.');
  require(new Set(catalog.map(row => row.class_path)).size === catalog.length, 'selection_order contains duplicate classes.');
  const equips = validateReceipt(equip, 'equip_audit', catalog);
  const probes = validateReceipt(probe, 'action_probe', catalog);
  const multipartSources = validateMultipart(probe, catalog);
  const renderedRows = rendered ? validateReceipt(rendered, 'action_probe', catalog) : null;
  if (rendered) {
    for (const field of ['tested_binary_sha256', 'tested_host_game_binary_sha256', 'tested_map_sha256', 'tested_camera_mapping_sha256'])
      require(sameHash(rendered[field], probe[field]), `Rendered confirmation differs from full probe: ${field}.`);
    require(rendered.map === probe.map, 'Rendered confirmation and full probe have different maps.');
    require(typeof rendered.run_id_utc === 'string' && rendered.run_id_utc !== probe.run_id_utc, 'Rendered confirmation must be a separate run, not an alias of the full probe.');
    if (Object.hasOwn(rendered, 'runner_mode')) require(rendered.runner_mode === 'Capture', 'Rendered confirmation runner_mode must be Capture.');
    if (Object.hasOwn(rendered, 'renderer_enabled')) require(rendered.renderer_enabled === true, 'Rendered confirmation explicitly disabled the renderer.');
    if (Object.hasOwn(rendered, 'audio_disabled_by_command_line')) require(rendered.audio_disabled_by_command_line === true, 'This comparison expects the Capture control to use -nosound.');
    for (const index of renderedRows.keys())
      require(probes.get(index)?.some(row => row.status === 'action_probe'), `Rendered confirmation lab #${index + 1} has no matching action in the full probe.`);
    if (!history) require(typeof rendered.collected_at_utc === 'string' && Number.isFinite(Date.parse(rendered.collected_at_utc)), 'Rendered confirmation lacks collection provenance; run collect_results.mjs first.');
  }
  if (!history) {
    require(build?.build_exit_code === 0, 'Final report requires a successful build receipt.');
    for (const [label, receipt] of [['equip', equip], ['probe', probe]]) {
      require(sameHash(receipt.tested_binary_sha256, build.plugin_dll_sha256), `${label}: plugin fingerprint differs from current build. Use --history --stdout for old receipts.`);
      require(sameHash(receipt.tested_host_game_binary_sha256, build.host_game_dll_sha256), `${label}: host gameplay fingerprint differs from current build.`);
      require(typeof receipt.collected_at_utc === 'string' && Number.isFinite(Date.parse(receipt.collected_at_utc)), `${label}: missing collection provenance; run collect_results.mjs first.`);
    }
    require(sameHash(equip.tested_map_sha256, probe.tested_map_sha256), 'Equip and probe did not test the same map fingerprint.');
    require(sameHash(equip.tested_camera_mapping_sha256, probe.tested_camera_mapping_sha256), 'Equip and probe did not test the same camera mapping fingerprint.');
    require(equip.map === probe.map && typeof equip.map === 'string' && equip.map.includes('ReadyOrNot_GunLab'), 'Unexpected or inconsistent lab map.');
  }

  const rows = catalog.map((entry, index) => {
    const audit = equips.get(index)?.[0];
    const batch = probes.get(index) ?? [];
    const action = batch.find(row => row.status === 'action_probe');
    const probeEquip = batch.find(equipOK);
    const observed = action ?? probeEquip ?? audit;
    // A rejected class can leave the previous weapon in hand. Do not attribute
    // its route/ammunition diagnostics to the rejected class.
    const excluded = abstractRow(audit) && !action;
    const route = excluded ? undefined : observed?.selection_route;
    const missingAmmo = route === 'catalog_asset' &&
      (observed?.authored_ammunition_type_count === 0 || /missing_authored_ammunition_types/.test(observed?.selection_route_reason ?? ''));
    const confirmedIncomplete = missingAmmo && auditedIncompleteClasses.has(entry.class_path);
    const group = excluded ? 'abstract' : confirmedIncomplete ? 'incomplete' : route === 'native_loadout' ? 'native' : route === 'catalog_asset' ? 'catalog' : 'unknown';
    const notes = [];
    if (!audit) notes.push('Thiếu equip audit');
    else if (!equipOK(audit)) notes.push(audit.detail || audit.status);
    if (!action && !excluded) notes.push('Chưa có hàng action_probe');
    if (missingAmmo) notes.push('Không khai báo AmmunitionTypes; riêng dữ kiện này chưa chứng minh asset hỏng');
    if (confirmedIncomplete) notes.push('Cấu hình player đã được audit riêng: chưa đủ dữ liệu; không xác nhận trải nghiệm bắn hoàn chỉnh');
    if (projectileCatalogClasses.has(entry.class_path)) notes.push('Native GrenadeLauncher dùng cấu hình projectile; không suy ra thiếu nội dung từ ammo-row trống');
    if (entry.class_path === '/Game/Blueprints/Items/WeaponsRevised/Deployable_M32A1.Deployable_M32A1_C')
      notes.push('Audit riêng M32A1: AnimData không có Reload/ReloadEmpty/Tactical/crouch reload hoặc import montage reload; 36 animation M32 không có reload. GrenadeLauncher kế thừa BaseMagazineWeapon, không override reload; NextMagazine cần notify authored. Đối chiếu số đo Capture riêng, không gộp M320 vào giới hạn này');
    if (route === 'catalog_asset' && suspectShotgunsWithoutPlayerReload.has(entry.class_path)) notes.push('Asset suspect: có reload third-person nhưng thiếu player Reload_Start/Loop/End; một lượt mới reset world không chứng minh transition liên tục khỏi súng này');
    if (action?.source_run_id_utc) notes.push(`Nguồn action: ${action.source_run_id_utc}`);
    if (observed?.selection_route_reason && !excluded) notes.push(observed.selection_route_reason);
    if (action?.held_class && action.held_class !== entry.class_path) notes.push('CẢNH BÁO: lớp đang cầm khác lớp yêu cầu');
    if (action?.native_owner === false) notes.push('CẢNH BÁO: owner không phải native player');
    if (audit?.selection_route && probeEquip?.selection_route && audit.selection_route !== probeEquip.selection_route)
      notes.push(`Route khác giữa hai lượt: ${audit.selection_route} → ${probeEquip.selection_route}`);
    for (const row of batch.filter(row => !equipOK(row) && row.status !== 'action_probe')) notes.push(`${row.status}: ${row.detail ?? ''}`);
    return { ...entry, audit, action, probeEquip, route, group, notes };
  });
  const expected = rows.filter(row => row.group !== 'abstract');
  const completed = rows.filter(row => row.action);
  const groups = [
    ['native', 'Loadout native'], ['catalog', 'Asset catalog, gồm lớp dùng projectile riêng'],
    ['incomplete', 'Catalog: chưa đủ dữ liệu của cấu hình này'], ['unknown', 'Chưa đủ dữ liệu route'],
    ['abstract', 'Lớp abstract được loại khỏi action probe']
  ];
  const tally = items => {
    const measured = items.filter(row => row.action);
    const count = (key, value) => measured.filter(row => row.action[key] === value).length;
    return { total: items.length, action: measured.length, fired: count('ammo_consumed', true), noFire: count('ammo_consumed', false),
      ads: count('native_aiming_state_before_fire', true), reload: count('native_reload_replenished', true),
      noReload: count('native_reload_replenished', false) };
  };
  const lines = [
    '# Kết quả runtime theo từng vũ khí', '',
    history ? '> **DỮ LIỆU LỊCH SỬ — chỉ kiểm thử cách đọc báo cáo. Không chứng minh bản build hiện tại.**' : '> Báo cáo được sinh từ các receipt công khai có cùng fingerprint plugin, gameplay DLL, map và camera mapping. Các giá trị là quan sát runtime, không phải đánh giá cảm giác bắn.', '',
    `Equip run: **${escape(equip.run_id_utc ?? 'chưa ghi')}**. Action run: **${escape(probe.run_id_utc ?? 'chưa ghi')}**. Map: \`${escape(equip.map ?? 'chưa ghi')}\`.`, '',
    `Catalog có **${rows.length} Blueprint/variant**. Equip audit ghi **${equips.size}/${rows.length} mục**; action probe ghi **${completed.length}/${expected.length} mục dự kiến**. **${rows.filter(row => row.group === 'abstract').length} lớp abstract** không được yêu cầu chạy action.`, '',
    expected.every(row => row.action) && equips.size === rows.length
      ? '**Độ phủ batch: đủ hàng dự kiến.** Điều này không có nghĩa mọi súng đã bắn/reload thành công.'
      : '**Độ phủ batch: chưa đủ hàng dự kiến.** Các mục thiếu được giữ trong bảng bên dưới; không tính là thành công.', '',
    'Số lab trong bảng bắt đầu từ **1**; `index` trong receipt bắt đầu từ **0**. Script kiểm tra cả index lẫn đường dẫn class trước khi ghép dữ liệu. Một variant không tương đương một mẫu súng độc lập.', '',
    '## Tổng hợp theo đường chọn súng', '',
    '| Nhóm | Số mục | Có action | Có tiêu đạn | Không tiêu đạn | ADS có | Reload tăng đạn | Reload chưa tăng đạn |',
    '|---|---:|---:|---:|---:|---:|---:|---:|'
  ];
  for (const [key, label] of groups) {
    const items = rows.filter(row => row.group === key); if (!items.length) continue;
    const c = tally(items);
    lines.push(`| ${label} | ${c.total} | ${c.action} | ${c.fired} | ${c.noFire} | ${c.ads} | ${c.reload} | ${c.noReload} |`);
  }
  if (multipartSources) {
    lines.push('', '## Nguồn của batch ghép nhiều phiên', '',
      `Action receipt trên là **aggregate của ${multipartSources.length} phiên native**, không phải một lần chơi liên tục. ${probe.world_reset_between_runs ? '**World được tạo mới giữa các phiên.** Trạng thái inventory, mục tiêu và animation không được giữ xuyên qua ranh giới đó.' : 'Aggregate này hiện chỉ có một phiên nguồn.'}`, '',
      'Wrapper chỉ nhận các cặp equip + action đã hoàn tất, kiểm tra class/index và fingerprint đầu vào. Receipt native từng phiên giữ nguyên; các hàng trong aggregate mang `source_run_id_utc`. Nếu một phiên kết thúc thiếu phần đuôi nhưng exit code 0 và không có lỗi tường minh, phiên mới đo phần còn lại. Điều này không chứng minh chuyển súng liên tục qua điểm dừng đã thành công.', '',
      'Các asset suspect M37, SawnOff và Trenchgun có dữ liệu reload third-person nhưng thiếu player Reload_Start/Loop/End theo audit source/asset. Trạng thái chặn sau reload là nguyên nhân cần đối chiếu; receipt hiện tại không ghi trực tiếp cờ blocking tại thời điểm kết thúc. Reset world giúp thu thập phần đuôi, không sửa hay chứng minh holster/reload của các asset đó.', '',
      '| Phiên nguồn | Receipt native / SHA-256 | Lab # đã lấy | Số hàng nhận | Exit code | Phạm vi |',
      '|---|---|---|---:|---|---|');
    for (const source of multipartSources) {
      const origin = source.source_kind === 'provided_native_resume_receipt' ? 'Receipt partial cung cấp để tiếp tục; exit code không có trong native JSON' : source.incomplete_for_requested_subset ? 'Phiên wrapper: kết thúc thiếu phần yêu cầu; tiếp tục ở world mới' : 'Phiên wrapper: đủ phần yêu cầu';
      lines.push(`| ${escape(source.run_id_utc)} | \`${escape(source.receipt_file)}\`<br>\`${source.receipt_sha256}\` | ${source.completed_indices_zero_based.map(i => i + 1).join(', ')} | ${source.accepted_row_count} | ${source.process_exit_code === null ? 'Chưa ghi trong receipt nguồn' : source.process_exit_code} | ${origin} |`);
    }
    lines.push('', 'SHA-256 ở đây định danh file native cục bộ đã được wrapper kiểm tra trước khi ghép; trang công khai không suy ra exit code còn thiếu và không biến tổng thời gian của nhiều world thành một timeline liên tục.', '');
  }
  lines.push('', '“Có/Không” chỉ đếm boolean thực sự có trong receipt. Dữ liệu thiếu hiển thị **—**, không chuyển thành “Không” hoặc “Có”. Nhóm chưa đủ dữ liệu chỉ gồm hai cấu hình đã được audit riêng: S590 Beanbag V2 và Flaregun. Một mảng AmmunitionTypes trống tự nó không đủ để kết luận asset thiếu nội dung; các launcher dùng cấu hình projectile vẫn được giữ trong nhóm catalog.', '',
    '## Cách đọc các cột', '',
    '- **Equip A/P:** kết quả equip trong lượt audit / lượt action; “instant” dùng nhánh native bỏ qua holster của asset đang rời, không chứng minh holster thường.',
    '- **Đạn trước → sau → cuối:** trước bắn, khi thả bắn sau khoảng 0,2 giây, cuối probe sau yêu cầu reload. Burst hoặc phát bắn đang chờ có thể tiếp tục sau mốc “sau”.',
    '- **ADS:** trạng thái aiming trước lệnh bắn. **Reload yêu cầu / cho phép / tăng đạn:** lần lượt là lệnh reload đã gọi, CanReload trước lệnh, và đạn cuối lớn hơn đạn ở mốc sau bắn.',
    '- “Không tăng đạn” cần đối chiếu asset/animation và subclass; không tự đồng nghĩa toàn bộ reload logic bị lỗi. F7 refill không được dùng làm bằng chứng reload.',
    '- `catalog_asset` dùng đường xem asset native. Ammo có giảm vẫn không chứng minh projectile, damage, camera, âm thanh hoặc chức năng player loadout hoàn chỉnh.', '');
  const status = row => !row ? '—' : row.status === 'equipped' ? 'equipped' : row.status === 'equipped_instant_fallback' ? 'instant' : row.status;
  for (const [key, label] of groups) {
    const items = rows.filter(row => row.group === key); if (!items.length) continue;
    lines.push(`## ${label}`, '', '| Lab # | Tên / class | Equip A/P | Đạn trước → sau → cuối | ADS | Reload yêu cầu / cho phép / tăng đạn | Mag cuối | Route / ghi chú |',
      '|---:|---|---|---|---|---|---:|---|');
    for (const row of items) {
      const a = row.action;
      const className = row.class_path.split('.').at(-1);
      const notes = [row.group === 'abstract' ? 'Không chạy action' : routeName(row.route), ...row.notes];
      lines.push(`| ${row.index} | ${escape(row.name)}<br>\`${escape(className)}\` | ${escape(status(row.audit))} / ${escape(status(row.probeEquip))} | ${num(a?.ammo_before)} → ${num(a?.ammo_after_native_fire)} → ${num(a?.ammo)} | ${bool(a?.native_aiming_state_before_fire)} | ${bool(a?.native_reload_requested)} / ${bool(a?.native_can_reload_before_request)} / ${bool(a?.native_reload_replenished)} | ${num(a?.magazines)} | ${escape(notes.join('; '))} |`);
    }
    lines.push('');
  }
  let renderedSummary = null;
  if (renderedRows) {
    const comparison = [...renderedRows.entries()].sort(([a], [b]) => a - b).map(([index, batch]) => ({
      ...rows[index], renderedAction: batch.find(row => row.status === 'action_probe'),
      renderedEquip: batch.find(equipOK), renderedFailures: batch.filter(row => !equipOK(row) && row.status !== 'action_probe')
    }));
    const measured = comparison.filter(row => row.renderedAction);
    renderedSummary = { testedEntries: comparison.length, completedActions: measured.length,
      fired: measured.filter(row => row.renderedAction.ammo_consumed === true).length,
      reload: measured.filter(row => row.renderedAction.native_reload_replenished === true).length,
      headlessNoReloadRenderedReload: measured.filter(row => row.action.native_reload_replenished === false && row.renderedAction.native_reload_replenished === true).length,
      bothNoReload: measured.filter(row => row.action.native_reload_replenished === false && row.renderedAction.native_reload_replenished === false).length };
    const ammo = action => `${num(action?.ammo_before)} → ${num(action?.ammo_after_native_fire)} → ${num(action?.ammo)}`;
    const reload = action => `${bool(action?.native_reload_requested)} / ${bool(action?.native_can_reload_before_request)} / ${bool(action?.native_reload_replenished)}`;
    lines.push('## Đối chiếu headless và có renderer trên tập được thử lại', '',
      `Batch toàn bộ phía trên là **Mode Probe: \`-nullrhi -nosound\`**, ${multipartSources ? 'aggregate nhiều world' : 'run'} **${escape(probe.run_id_utc)}**. Lượt đối chiếu là **Mode Capture: có renderer, \`-nosound\`**, run **${escape(rendered.run_id_utc)}**. Lượt Capture chỉ có **${comparison.length} mục được chọn**, với **${measured.length} hàng action**; không thay thế hay ghi đè các quan sát của batch toàn bộ.`, '',
      'Các Blueprint/animation native có thể phụ thuộc mesh được render và animation notify. Cần đọc hai lượt cạnh nhau trước khi kết luận reload hỏng. Đây là đối chiếu quan sát; khác biệt thứ tự equip hoặc trạng thái đầu vào giữa hai lượt chưa được loại trừ, nên không quy mọi khác biệt cho renderer.', '',
      `Trong tập này, Capture ghi nhận tiêu đạn ở **${renderedSummary.fired}/${measured.length} mục**, tăng đạn sau yêu cầu reload ở **${renderedSummary.reload}/${measured.length} mục**. **${renderedSummary.headlessNoReloadRenderedReload} mục** không tăng đạn ở Probe nhưng tăng ở Capture; **${renderedSummary.bothNoReload} mục** đều chưa tăng đạn ở cả hai lượt. Đây không phải số “súng hoạt động hoàn chỉnh”.`, '',
      'Cột reload vẫn là **yêu cầu / cho phép / tăng đạn**. Cả hai lượt đều tắt âm thanh; Capture có renderer không tự chứng minh chất lượng animation, cảm giác bắn hoặc âm thanh.', '',
      '| Lab # / class | Đạn Probe: trước → sau → cuối | Đạn Capture: trước → sau → cuối | ADS Probe / Capture | Reload Probe | Reload Capture | Ghi chú |',
      '|---|---|---|---|---|---|---|');
    for (const row of comparison) {
      const a = row.action, r = row.renderedAction;
      const notes = [`Equip Capture: ${status(row.renderedEquip)}`];
      if (a?.source_run_id_utc) notes.push(`Nguồn Probe: ${a.source_run_id_utc}`);
      if (!r) notes.push('Thiếu action Capture');
      if (row.group === 'incomplete') notes.push('Cấu hình được audit riêng là chưa đủ dữ liệu; ammo tồn tại/giảm không chứng minh súng hoàn chỉnh');
      if (row.class_path === '/Game/Blueprints/Items/WeaponsRevised/Deployable_M32A1.Deployable_M32A1_C')
        notes.push('M32A1 thiếu dữ liệu reload theo audit riêng; M320 có reload authored, không chung giới hạn này');
      if (r?.selection_route !== undefined && r.selection_route !== a.selection_route) notes.push(`Route khác: ${a.selection_route ?? '—'} → ${r.selection_route}`);
      if (r?.ammunition_row !== undefined && a.ammunition_row !== undefined && r.ammunition_row !== a.ammunition_row) notes.push(`Loại đạn khác: ${a.ammunition_row} → ${r.ammunition_row}`);
      if (r?.held_class && r.held_class !== row.class_path) notes.push('CẢNH BÁO: class đang cầm khác class yêu cầu');
      for (const failure of row.renderedFailures) notes.push(`${failure.status}: ${failure.detail ?? ''}`);
      lines.push(`| ${row.index} — ${escape(row.name)}<br>\`${escape(row.class_path.split('.').at(-1))}\` | ${ammo(a)} | ${ammo(r)} | ${bool(a?.native_aiming_state_before_fire)} / ${bool(r?.native_aiming_state_before_fire)} | ${reload(a)} | ${reload(r)} | ${escape(notes.join('; '))} |`);
    }
    lines.push('', '[Receipt riêng của lượt Capture](../lab/manifests/rendered_confirmation_receipt.json). Script xác nhận class/index và fingerprint plugin, gameplay DLL, map, camera mapping trùng với batch toàn bộ.', '',
      `SHA-256 rendered confirmation receipt: \`${hashes.rendered ?? 'không cung cấp'}\`.`, '');
  }
  lines.push('## Phạm vi bằng chứng', '',
    'Báo cáo này không xác nhận cảm giác bắn, âm thanh nghe được, hình ảnh/animation đúng theo thời gian, damage/hit feedback, hoặc tương đương bản retail. Batch action thường chạy với `-nullrhi -nosound`. Các kiểm tra trải nghiệm có renderer, fixture, camera và input nằm trong [receipt trải nghiệm](../lab/manifests/experience_receipt.json) và [báo cáo assertion](../lab/manifests/experience_verification.json); chúng cũng có giới hạn riêng.', '',
    'Không gán loại đạn giả cho asset chưa hoàn chỉnh. `Primary_S590_Beanbag_V2` và `Secondary_Flaregun` được audit riêng là cấu hình player chưa đầy đủ; chúng hiện không khai báo `AmmunitionTypes`. Trải nghiệm beanbag player có sẵn nằm ở `Primary_W870LL` với đạn `12gaBeanbag`. Ngược lại, M32A1 và các M320 cụ thể là lớp `GrenadeLauncher` dùng dữ liệu projectile; ammo-row trống không phải bằng chứng rằng các launcher này hỏng.', '',
    '## Nguồn và tái tạo báo cáo', '',
    '- [Selection order](../lab/manifests/selection_order.json): tên, class và số lab bắt đầu từ 1.',
    '- [Equip audit](../lab/manifests/equip_audit_receipt.json) và [action probe](../lab/manifests/action_probe_receipt.json): dữ liệu gốc.',
    '- [Build receipt](../lab/manifests/build_receipt.json): fingerprint bản đã biên dịch.', '',
    'Sau khi collect receipt của lượt cuối, chạy từ gốc repo:', '',
    '```powershell', 'node lab/scripts/report_gunlab_results.mjs', '```', '',
    'Kiểm tra receipt cũ mà không ghi vào trang kết quả hiện tại:', '',
    '```powershell', 'node lab/scripts/report_gunlab_results.mjs --history --stdout', '```', '',
    `SHA-256 equip receipt: \`${hashes.equip ?? 'không cung cấp'}\`.`, '',
    `SHA-256 action receipt: \`${hashes.probe ?? 'không cung cấp'}\`.`, '',
    `SHA-256 selection order: \`${hashes.selection ?? 'không cung cấp'}\`.`, '',
    `Plugin: \`${escape(equip.tested_binary_sha256 ?? 'chưa ghi')}\`.`, '',
    `Gameplay DLL: \`${escape(equip.tested_host_game_binary_sha256 ?? 'chưa ghi')}\`.`, '',
    `Map: \`${escape(equip.tested_map_sha256 ?? 'chưa ghi')}\`.`, '',
    `Camera mapping: \`${escape(equip.tested_camera_mapping_sha256 ?? 'chưa ghi')}\`.`, '');
  return { markdown: lines.join('\n'), summary: { history, candidates: rows.length, equipRows: equips.size, expectedActions: expected.length,
    completedActions: completed.length, completeCoverage: expected.every(row => row.action) && equips.size === rows.length,
    groups: Object.fromEntries(groups.map(([key]) => [key, tally(rows.filter(row => row.group === key))])),
    sourceRunCount: multipartSources?.length ?? 1, worldResetBetweenRuns: probe.world_reset_between_runs === true, renderedConfirmation: renderedSummary } };
}

function main() {
  const args = process.argv.slice(2);
  let manifests = path.join(repo, 'lab/manifests'), output = defaultOutput, history = false, stdout = false;
  while (args.length) {
    const arg = args.shift();
    if (arg === '--history') history = true;
    else if (arg === '--stdout') stdout = true;
    else if (arg === '--manifests' || arg === '--output') {
      const value = args.shift(); require(value && !value.startsWith('--'), `${arg} requires a path.`);
      if (arg === '--manifests') manifests = path.resolve(value); else output = path.resolve(value);
    } else if (arg === '--help') {
      console.log('node lab/scripts/report_gunlab_results.mjs [--manifests DIR] [--output FILE] [--stdout] [--history]\nFinal mode requires matching collected build/map provenance. --history must use --stdout or a different output file.'); return;
    } else throw new Error(`Unknown argument: ${arg}`);
  }
  require(!history || stdout || output.toLowerCase() !== defaultOutput.toLowerCase(), 'History cannot overwrite runtime-results.md. Use --history --stdout or a separate --output.');
  const selection = read(path.join(manifests, 'selection_order.json'));
  const equip = read(path.join(manifests, 'equip_audit_receipt.json'));
  const probe = read(path.join(manifests, 'action_probe_receipt.json'));
  const renderedPath = path.join(manifests, 'rendered_confirmation_receipt.json');
  const rendered = fs.existsSync(renderedPath) ? read(renderedPath) : undefined;
  const build = history ? undefined : read(path.join(manifests, 'build_receipt.json')).data;
  const report = createGunLabReport({ selection: selection.data, equip: equip.data, probe: probe.data, rendered: rendered?.data, build, history,
    hashes: { selection: selection.sha256, equip: equip.sha256, probe: probe.sha256, rendered: rendered?.sha256 } });
  if (stdout) process.stdout.write(report.markdown);
  else { fs.mkdirSync(path.dirname(output), { recursive: true }); fs.writeFileSync(output, report.markdown); }
  console.error(JSON.stringify({ ...report.summary, output: stdout ? 'stdout' : output }, null, 2));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { main(); } catch (error) { console.error(error.message); process.exitCode = 1; }
}

import fs from 'node:fs'
import path from 'node:path'
import {createHash} from 'node:crypto'
import {fileURLToPath} from 'node:url'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const out = path.resolve(root, '.site')
if (path.dirname(out) !== root || path.basename(out) !== '.site') throw new Error('Unexpected generated directory')
fs.mkdirSync(out, {recursive: true})
const pages = []
function collect(folder) {
  for (const e of fs.readdirSync(folder, {withFileTypes: true})) {
    if (e.isSymbolicLink()) continue
    if (e.isDirectory()) collect(path.join(folder, e.name))
    else if (e.name.endsWith('.md')) pages.push(path.relative(root, path.join(folder, e.name)).replaceAll('\\', '/'))
  }
}
for (const name of ['index.md', 'README.md', 'BAT-DAU-TU-DAY.md', '00-MucLuc.md', 'WEBSITE.md']) {
  if (fs.existsSync(path.join(root, name))) pages.push(name)
}
for (const e of fs.readdirSync(root, {withFileTypes: true})) if (e.isDirectory() && /^0[1-6]-/.test(e.name)) collect(path.join(root, e.name))
const pageSet = new Set(pages)
const pageName = n => n === 'README.md' ? 'gioi-thieu.md' : n
// Prune only generated Markdown whose source page was removed or renamed.
const expectedPages = new Set([...pages.map(pageName), 'nguon-cuc-bo.md', '00-MucLuc.md'])
function pruneGeneratedPages(folder) {
  for (const e of fs.readdirSync(folder, {withFileTypes: true})) {
    if (e.isSymbolicLink()) continue
    const target = path.join(folder, e.name)
    if (e.isDirectory() && e.name !== 'public') pruneGeneratedPages(target)
    else if (e.isFile() && e.name.endsWith('.md') && !expectedPages.has(path.relative(out, target).replaceAll('\\', '/'))) fs.unlinkSync(target)
  }
}
pruneGeneratedPages(out)
const sourceRefs = new Map()
const downloads = new Set()
const esc = s => s.replaceAll('&', '&amp;').replaceAll('"', '&quot;').replaceAll('<', '&lt;').replaceAll('>', '&gt;')
function sourceLink(value) {
  const id = 'ref-' + createHash('sha256').update(value).digest('hex').slice(0, 14)
  sourceRefs.set(id, value.replaceAll('\\', '/').replace(/^D:\/Zone9Dev_RON\//i, ''))
  return '/nguon-cuc-bo#' + id
}
function rewrite(text, origin) {
  let inFence = false
  return text.split('\n').map(line => {
    if (/^\s*(`{3,}|~{3,})/.test(line)) {inFence = !inFence; return line}
    if (inFence) return line
    return line.replace(/(?<!!)\[([^\]\n]+)\]\((<[^>\n]+>|[^)\n]+)\)/g, (whole, label, raw) => {
      const target = raw.trim().replace(/^<|>$/g, '').replaceAll('\\', '/')
      if (/^(https?:|mailto:|#)/i.test(target)) return whole
      if (/^[a-z]:\//i.test(target) || target.startsWith('file:')) return '[' + label + '](' + sourceLink(target) + ')'
      if (target.startsWith('/')) return whole
      const index = target.indexOf('#')
      const fragment = index < 0 ? '' : target.slice(index)
      const pathname = decodeURIComponent(index < 0 ? target : target.slice(0, index))
      const rel = path.posix.normalize(path.posix.join(path.posix.dirname(origin), pathname))
      if (pageSet.has(rel)) return '[' + label + '](/' + pageName(rel).replace(/\.md$/, '') + fragment + ')'
      const absolute = path.resolve(root, rel)
      if (!absolute.startsWith(root + path.sep)) return '[' + label + '](' + sourceLink(target) + ')'
      if (/\.(cpp|h|inl|cs|uasset|umap)(:\d+)?$/.test(pathname) && !rel.startsWith('lab/')) return '[' + label + '](' + sourceLink(target) + ')'
      if (!fs.existsSync(absolute) || !fs.statSync(absolute).isFile()) return whole
      if (!/\.(json|md|py|ps1|mjs|cpp|h|cs|uplugin|txt|yaml|yml)$/i.test(rel)) throw new Error('Unexpected download ' + rel)
      downloads.add(rel)
      return '<a href="/ReadyOrNot-Docs/downloads/' + esc(rel) + fragment + '" download>' + esc(label.replaceAll('`', '')) + '</a>'
    })
  }).join('\n')
}
for (const rel of pages) {
  const target = path.join(out, pageName(rel))
  fs.mkdirSync(path.dirname(target), {recursive: true})
  fs.writeFileSync(target, rewrite(fs.readFileSync(path.join(root, rel), 'utf8').replace(/^\uFEFF/, ''), rel))
}
const sourcePage = ['# Đường dẫn nguồn cục bộ', '', 'Đây là địa chỉ bằng chứng trong snapshot, không phải bản phân phối source/asset. Dùng root workspace của bạn, tìm file và symbol ghi trong chương. Số dòng chỉ có ý nghĩa với hash nguồn trong catalog.', '']
for (const [id, value] of [...sourceRefs].sort((a,b) => a[1].localeCompare(b[1]))) {
  sourcePage.push('## ' + path.posix.basename(value).replace(/[<>]/g, '') + ' {#' + id + '}', '', '```text', value, '```', '')
}
fs.writeFileSync(path.join(out, 'nguon-cuc-bo.md'), sourcePage.join('\n'))
function copyTree(source, dest) {
  if (!fs.existsSync(source)) return
  fs.mkdirSync(dest, {recursive: true})
  for (const e of fs.readdirSync(source, {withFileTypes: true})) {
    if (e.isSymbolicLink()) continue
    if (e.isDirectory()) copyTree(path.join(source,e.name), path.join(dest,e.name))
    else fs.copyFileSync(path.join(source,e.name), path.join(dest,e.name))
  }
}
copyTree(path.join(root, 'public'), path.join(out, 'public'))
fs.mkdirSync(path.join(out, 'public/data'), {recursive: true})
for (const file of ['snapshot.json','weapons.json','source-groups.json','content-groups.json','project-plugins.json','maps.json']) {
  const source = path.join(root, '06-Catalogs', file)
  if (fs.existsSync(source)) fs.copyFileSync(source, path.join(out, 'public/data', file))
}
for (const rel of downloads) {
  const target = path.join(out, 'public/downloads', rel)
  fs.mkdirSync(path.dirname(target), {recursive: true})
  fs.copyFileSync(path.join(root, rel), target)
}
const chapters = pages.filter(p => /^0[1-6]-/.test(p)).sort()
const title = p => (fs.readFileSync(path.join(root,p), 'utf8').match(/^#\s+(.+)$/m)?.[1] || p).replace(/[*`]/g,'')
const index = ['# Mục lục toàn bộ', '', 'Đọc tuyến tính khi mới bắt đầu; dùng tìm kiếm theo class, hành động hoặc lỗi khi đang thực hành.', '', ...chapters.map(p => '- [' + title(p) + '](/' + p.replace(/\.md$/, '') + ')'), '']
fs.writeFileSync(path.join(out, '00-MucLuc.md'), index.join('\n'))
fs.writeFileSync(path.join(root, '00-MucLuc.md'), index.join('\n').replaceAll('](/', '](').replace(/\)\n/g, '.md)\n'))
fs.writeFileSync(path.join(out, 'public/.nojekyll'), '')
const report = {date: new Date().toISOString().slice(0, 10), pages: pages.length + 1, chapters: chapters.length, localSourceLinks: sourceRefs.size, downloads: [...downloads].sort(), scope: 'Authored docs and original lab scripts only; no proprietary binary assets or source contents.'}
fs.writeFileSync(path.join(out, 'manifest.json'), JSON.stringify(report,null,2) + '\n')
console.log(JSON.stringify(report))

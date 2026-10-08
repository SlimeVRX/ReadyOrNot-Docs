// Read-only inventory of a supplied local project. Publishes names, counts and
// hashes, never game code, configuration values, binary assets or credentials.
import fs from 'node:fs'
import path from 'node:path'
import {createHash} from 'node:crypto'
import {fileURLToPath} from 'node:url'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const workspace = path.resolve(process.argv[2] || path.join(root, '..'))
const project = path.join(workspace, 'Ready Or Not')
const output = path.join(root, '06-Catalogs')
fs.mkdirSync(output, {recursive: true})
function walk(directory) {
  const files = []
  if (!fs.existsSync(directory)) return files
  for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
    if (entry.isSymbolicLink()) continue
    const full = path.join(directory, entry.name)
    if (entry.isDirectory()) files.push(...walk(full))
    else files.push(full)
  }
  return files
}
const relative = file => path.relative(workspace, file).replaceAll('\\', '/')
const hash = buffer => createHash('sha256').update(buffer).digest('hex')
const write = (name, value) => fs.writeFileSync(path.join(output, name), JSON.stringify(value, null, 2) + '\n')
const source = walk(path.join(project, 'Source'))
  .filter(file => /\.(cpp|h|cs|inl)$/.test(file))
  .map(file => {
    const data = fs.readFileSync(file)
    return {path: relative(file), bytes: data.length, lines: data.toString('utf8').split(/\r?\n/).length,
      sha256: hash(data), group: path.relative(path.join(project, 'Source'), file).split(path.sep).slice(0, 2).join('/')}
  }).sort((a, b) => a.path.localeCompare(b.path))
const groupMap = {}
for (const f of source) {
  const g = groupMap[f.group] ||= {files: 0, lines: 0, bytes: 0}
  g.files++; g.lines += f.lines; g.bytes += f.bytes
}
const assetFiles = walk(path.join(project, 'Content'))
const assetGroups = {}
const extensions = {}
for (const file of assetFiles) {
  const rel = path.relative(path.join(project, 'Content'), file).split(path.sep)
  const ext = path.extname(file).toLowerCase() || '(none)'
  extensions[ext] = (extensions[ext] || 0) + 1
  const group = rel.slice(0, Math.min(2, rel.length - 1)).join('/') || '(root)'
  const g = assetGroups[group] ||= {files: 0, uasset: 0, umap: 0}
  g.files++; g.uasset += Number(ext === '.uasset'); g.umap += Number(ext === '.umap')
}
const descriptorFile = path.join(project, 'ReadyOrNot.uproject')
const descriptor = JSON.parse(fs.readFileSync(descriptorFile, 'utf8'))
const pluginFiles = walk(path.join(project, 'Plugins')).filter(f => f.endsWith('.uplugin'))
const plugins = pluginFiles.map(file => {
  try {
    const p = JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''))
    const name = path.basename(file, '.uplugin')
    const projectEntry = descriptor.Plugins?.find(x => x.Name === name)
    return {name, descriptor: relative(file), projectEnabled: projectEntry?.Enabled ?? null,
      enabledByDefault: p.EnabledByDefault ?? null, canContainContent: p.CanContainContent ?? false,
      modules: (p.Modules || []).map(m => ({name: m.Name, type: m.Type, loadingPhase: m.LoadingPhase || null}))}
  } catch (e) {return {descriptor: relative(file), parseError: String(e)}}
})
const maps = assetFiles.filter(f => f.endsWith('.umap')).map(file => ({
  package: '/Game/' + path.relative(path.join(project, 'Content'), file).replaceAll('\\', '/').replace(/\.umap$/, ''),
  file: relative(file), bytes: fs.statSync(file).size,
}))
const engine = JSON.parse(fs.readFileSync(path.join(workspace, 'Engine/Build/Build.version'), 'utf8'))
const configFiles = walk(path.join(project, 'Config')).filter(f => /\.(ini|eos)$/.test(f))
const metadata = {
  schema: 'ReadyOrNotDocs.SourceSnapshot.v1', observedDate: new Date().toISOString().slice(0, 10),
  sourceRoot: 'Ready Or Not/Source', engineVersion: [engine.MajorVersion, engine.MinorVersion, engine.PatchVersion].join('.'),
  engineChangelist: engine.Changelist, engineBranch: engine.BranchName,
  sourceGitCommit: null, retailVersion: null,
  sourceFiles: source.length, sourceLines: source.reduce((n, f) => n + f.lines, 0),
  sourceLineCountMethod: 'UTF-8 line split; includes comments, declarations and blank lines; not executable LOC',
  projectDescriptorSha256: hash(fs.readFileSync(descriptorFile)),
  sourceManifestSha256: hash(JSON.stringify(source)),
  contentFiles: assetFiles.length, contentExtensions: extensions, mapFiles: maps.length,
  projectPluginDescriptors: plugins.length, projectModules: descriptor.Modules,
  projectPluginToggles: descriptor.Plugins?.map(p => ({name: p.Name, enabled: p.Enabled})),
  configFiles: configFiles.length,
  limits: [
    'Filesystem counts do not prove asset class, runtime use, quality, loadability or retail parity.',
    'No release tag/commit was supplied; this is the local custom UE 5.3.2 snapshot, not every Ready or Not release.',
    'Source contents and binary assets are not copied into the documentation repository.',
  ],
}
write('snapshot.json', metadata)
write('source-manifest.json', source)
write('source-groups.json', groupMap)
write('content-groups.json', assetGroups)
write('project-plugins.json', plugins)
write('maps.json', maps)
console.log(JSON.stringify({sourceFiles: source.length, sourceLines: metadata.sourceLines, contentFiles: assetFiles.length, maps: maps.length, plugins: plugins.length}))

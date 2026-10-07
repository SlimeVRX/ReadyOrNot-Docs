import fs from 'node:fs'
import path from 'node:path'
import {fileURLToPath} from 'node:url'
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const dist=path.join(root,'.vitepress/dist')
const base='/ReadyOrNot-Docs/'
const origin='https://slimevrx.github.io'
const errors=[], files=[]
function walk(dir) {
  for(const e of fs.readdirSync(dir,{withFileTypes:true})) {
    if(e.isSymbolicLink())continue
    const full=path.join(dir,e.name)
    if(e.isDirectory())walk(full)
    else files.push(full)
  }
}
walk(dist)
const htmls=new Map(files.filter(f=>f.endsWith('.html')).map(f=>[f,fs.readFileSync(f,'utf8')]))
let links=0, anchors=0
for(const [file,html] of htmls) {
  const pageUrl=new URL(base+path.relative(dist,file).replaceAll('\\','/'),origin)
  for(const match of html.matchAll(/\b(?:href|src)="([^"]+)"/g)) {
    const raw=match[1].replaceAll('&amp;','&')
    if(/^(data:|mailto:|tel:|javascript:)/i.test(raw))continue
    if(/^[a-z]:[/\\]/i.test(raw)||raw.startsWith('file:')) {errors.push('Local file URL: '+raw);continue}
    let url
    try {url=new URL(raw,pageUrl)} catch {errors.push('Invalid URL '+raw);continue}
    if(url.origin!==origin)continue
    if(!url.pathname.startsWith(base)) {
      if(/^https?:/i.test(raw))continue
      errors.push('Outside Pages base: '+raw+' in '+path.relative(dist,file));continue
    }
    const local=path.join(dist,decodeURIComponent(url.pathname.slice(base.length)))
    const target=[local,local+'.html',path.join(local,'index.html')].find(f=>fs.existsSync(f)&&fs.statSync(f).isFile())
    links++
    if(!target) {errors.push('Missing '+raw+' in '+path.relative(dist,file));continue}
    if(url.hash && htmls.has(target)) {
      anchors++
      const id=decodeURIComponent(url.hash.slice(1))
      if(!htmls.get(target).includes('id="'+id+'"'))errors.push('Missing anchor '+raw+' in '+path.relative(dist,file))
    }
  }
}
const forbidden=files.filter(f=>/\.(uasset|umap|pak|ucas|utoc|dll|pdb|exe|bank|wav|mp4)$/i.test(f))
if(forbidden.length)errors.push(...forbidden.map(f=>'Unexpected binary in publication: '+path.relative(dist,f)))
const report={status:errors.length?'failed':'passed',htmlPages:htmls.size,internalLinksAndAssets:links,anchors,forbiddenBinaries:forbidden.length,errors:[...new Set(errors)],scope:'Static rendered HTML link/asset/anchor/base checks. Does not certify browser interactions, Unreal runtime or subjective gunfeel.'}
fs.mkdirSync(path.join(root,'.site'),{recursive:true})
fs.writeFileSync(path.join(root,'.site/site-validation.json'),JSON.stringify(report,null,2)+'\n')
console.log(JSON.stringify(report,null,2))
if(errors.length)process.exitCode=1

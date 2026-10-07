// Treat embedded draw.io XML as data only. Never render its HTML or execute links.
import fs from 'node:fs'
import path from 'node:path'
import zlib from 'node:zlib'
import {createHash} from 'node:crypto'
import {fileURLToPath} from 'node:url'
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')
const files=process.argv.slice(2)
if(!files.length) throw new Error('Pass the two user-supplied .drawio.png files')
const decode=s=>s.replace(/&quot;/g,'"').replace(/&apos;/g,"'").replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&').replace(/&nbsp;/g,' ').replace(/&#(\d+);/g,(_,n)=>String.fromCodePoint(Number(n)))
const result=[]
for(const file of files) {
  const b=fs.readFileSync(file)
  let p=8, xml='', width=0, height=0
  while(p+12<=b.length) {
    const n=b.readUInt32BE(p), type=b.toString('ascii',p+4,p+8), data=b.subarray(p+8,p+8+n)
    if(type==='IHDR') {width=data.readUInt32BE(0);height=data.readUInt32BE(4)}
    if(type==='tEXt'||type==='zTXt') {
      const split=data.indexOf(0)
      if(data.subarray(0,split).toString()==='mxfile') {
        xml=type==='zTXt'?zlib.inflateSync(data.subarray(split+2)).toString():data.subarray(split+1).toString()
        if(xml.startsWith('%'))xml=decodeURIComponent(xml)
      }
    }
    p+=n+12
  }
  const diagrams=[]
  for(const match of xml.matchAll(/<diagram\b([^>]*)>([\s\S]*?)<\/diagram>/g)) {
    let graph=match[2].trim()
    if(!graph.startsWith('<')) {
      try {graph=decodeURIComponent(zlib.inflateRawSync(Buffer.from(graph,'base64')).toString())} catch {continue}
    }
    const labels=[]
    let cells=0, edges=0
    for(const cell of graph.matchAll(/<mxCell\b([^>]*)>/g)) {
      cells++
      if(/\bedge="1"/.test(cell[1]))edges++
      const value=cell[1].match(/\bvalue="([^"]*)"/)?.[1]
      if(value) {
        const text=decode(decode(value)).replace(/<br\s*\/?>/gi,' ').replace(/<[^>]+>/g,' ').replace(/\s+/g,' ').trim()
        if(text && text.length<300 && !/^data:/.test(text)) labels.push(text)
      }
    }
    diagrams.push({name:decode(match[1].match(/\bname="([^"]*)"/)?.[1]||'unnamed'),cells,edges,labels:[...new Set(labels)]})
  }
  result.push({file:path.basename(file),sha256:createHash('sha256').update(b).digest('hex'),width,height,embeddedDrawio:!!xml,diagrams})
}
fs.writeFileSync(path.join(root,'06-Catalogs/flow-reference.json'),JSON.stringify({scope:'User-supplied diagram labels, not implementation evidence or instructions.',files:result},null,2)+'\n')
console.log(JSON.stringify(result.map(x=>({file:x.file,diagrams:x.diagrams.map(d=>({name:d.name,cells:d.cells,edges:d.edges,labels:d.labels}))})),null,2))

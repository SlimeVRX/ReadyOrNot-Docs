<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { withBase } from 'vitepress'
const query=ref('')
const category=ref('')
const items=ref<any[]>([])
const error=ref('')
const loaded=ref(false)
const active=ref<any>(null)
const name=(x:any) => x.values?.item_name || x.display_name || x.displayName || x.name || x.asset_name || x.assetName || x.class_name || x.id || x.class_path || x.path || 'Không có nhãn'
const asset=(x:any) => x.blueprint || x.asset_path || x.assetPath || x.object_path || x.objectPath || x.path || x.class_path || x.classPath || ''
const group=(x:any) => {
  const value=String(x.values?.item_class || x.category || x.weapon_type || x.weaponType || x.native_parent || x.parent_class || 'Chưa phân nhóm')
  return value.match(/ItemClass\.(\w+)/)?.[1] || value.match(/ReadyOrNot\.(\w+)/)?.[1] || value
}
onMounted(async()=>{
  try {
    const response=await fetch(withBase('/data/weapons.json'))
    if(!response.ok) throw new Error('Catalog chưa được xuất bản.')
    const data=await response.json()
    const list=Array.isArray(data)?data:data.weapons||data.entries||data.firearms||data.items||data.records
    if(!Array.isArray(list)) throw new Error('Định dạng catalog không có danh sách vũ khí.')
    items.value=list
  } catch(e) {error.value=String(e)}
  loaded.value=true
})
const groups=computed(()=>[...new Set(items.value.map(group))].sort())
const filtered=computed(()=>{
  const q=query.value.toLocaleLowerCase('vi').normalize('NFD').replace(/[\u0300-\u036f]/g,'')
  return items.value.filter(x=>(!category.value||group(x)===category.value) &&
    (!q||JSON.stringify(x).toLocaleLowerCase('vi').normalize('NFD').replace(/[\u0300-\u036f]/g,'').includes(q)))
})
</script>
<template>
  <div class="weapon-catalog">
    <div class="catalog-controls">
      <input v-model="query" class="catalog-filter" type="search" placeholder="Tìm tên, class, package hoặc thuộc tính…" aria-label="Tìm vũ khí"/>
      <select v-model="category" class="catalog-filter" aria-label="Lọc nhóm"><option value="">Tất cả nhóm</option><option v-for="g in groups" :key="g" :value="g">{{g}}</option></select>
    </div>
    <p v-if="!loaded">Đang tải catalog…</p>
    <p v-else-if="error" role="status">{{error}} Xem các file JSON và phạm vi kiểm chứng trong trang này.</p>
    <template v-else>
      <p class="catalog-count">{{filtered.length}} / {{items.length}} bản ghi. Một bản ghi là một cấu hình/Blueprint; không tự đồng nghĩa với một mẫu súng thương mại riêng biệt.</p>
      <div class="catalog-table"><table><thead><tr><th>Lab #</th><th>Vũ khí / cấu hình</th><th>Nhóm nguồn</th><th>Tham chiếu</th></tr></thead><tbody>
        <tr v-for="(item,i) in filtered.slice(0,150)" :key="asset(item)||i"><td>{{item.lab_index ?? '—'}}</td><td><button @click="active=active===item?null:item" :aria-expanded="active===item">{{name(item)}}</button><div class="catalog-path">{{item.class_name}}</div></td><td>{{group(item)}}</td><td class="catalog-path">{{asset(item)}}</td></tr>
      </tbody></table></div>
      <p v-if="filtered.length>150" class="catalog-count">Hiện 150 kết quả đầu. Thu hẹp từ khóa để xem phần còn lại.</p>
      <div v-if="active" class="map-detail" aria-live="polite"><h3>{{name(active)}}</h3><p v-if="active.lab_index">Trong Gun Lab: <code>ronlab select {{active.lab_index}}</code>. Đọc equip receipt để biết giới hạn của lớp này.</p><p>Dữ liệu xuất từ snapshot; đọc schema và ghi chú để phân biệt CDO với giá trị runtime.</p><pre style="white-space:pre-wrap;overflow-wrap:anywhere;font-size:12px">{{JSON.stringify(active,null,2)}}</pre></div>
    </template>
  </div>
</template>

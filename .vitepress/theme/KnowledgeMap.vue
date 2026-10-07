<script setup lang="ts">
import { ref } from 'vue'
import { withBase } from 'vitepress'
const selected = ref(0)
function moveTab(event: KeyboardEvent, index: number) {
  const delta = event.key === 'ArrowRight' ? 1 : event.key === 'ArrowLeft' ? -1 : 0
  if (!delta && event.key !== 'Home' && event.key !== 'End') return
  event.preventDefault()
  selected.value = event.key === 'Home' ? 0 : event.key === 'End' ? layers.length - 1 : (index + delta + layers.length) % layers.length
  const buttons = (event.currentTarget as HTMLElement).parentElement?.querySelectorAll<HTMLButtonElement>('[role="tab"]')
  buttons?.[selected.value]?.focus()
}
const layers = [
  {name:'Quan sát', title:'01 · Hiểu điều người chơi đang quyết định', question:'Người chơi biết gì, chọn gì, nhận hậu quả gì?', text:'Vẽ một vòng nhiệm vụ và một tình huống nhỏ: tiếp cận cửa, ra lệnh, phản ứng của nghi phạm, sử dụng vũ lực, thu vật chứng. Ghi cả nhánh thất bại. Chưa bắt đầu bằng danh sách class.', output:'Sơ đồ trạng thái quan sát được + video/timecode + điều chưa biết.', route:'/01-Ban-Do-Kien-Thuc/02-doc-mot-feature', next:'Gameplay, mục tiêu, ROE và AI'},
  {name:'Hợp đồng', title:'02 · Viết điều một feature phải bảo đảm', question:'Ai sở hữu state? Điều kiện bắt đầu và kết thúc là gì?', text:'Fire không chỉ là phát montage. Nó cần súng đang cầm, trạng thái cho phép, loại đạn, cadence, authority, commit và feedback. Mỗi cạnh trong sơ đồ trở thành một contract có điều kiện và kết quả.', output:'Feature dossier: input, state, owner, commit, hủy, mạng và acceptance.', route:'/01-Ban-Do-Kien-Thuc/03-hop-dong-feature', next:'Đọc owner trong source'},
  {name:'Unreal', title:'03 · Nắm vòng đời và chỗ đặt trách nhiệm', question:'GameMode, Controller, Pawn, Component và asset khác nhau thế nào?', text:'C++ class đặt luật; Blueprint/CDO và config có thể đổi dữ liệu; instance giữ state của lượt chơi. Server, owning client, simulated proxy và editor không cùng một vai trò. Đọc BeginPlay/EndPlay, RPC, replication và animation update.', output:'Sơ đồ owner và lifecycle có đường dẫn source thực.', route:'/04-Kien-Truc-Va-Quy-Trinh/README', next:'Dữ liệu và phụ thuộc'},
  {name:'Dữ liệu', title:'04 · Từ asset rời đến một cấu hình dùng được', question:'Giá trị đến từ constructor, Blueprint cha, CDO con hay runtime?', text:'Tên file chưa chứng minh đó là vũ khí dùng được. Cần asset class, ancestry, CDO, dependencies, skeleton, animation graph, socket, ammo, attachment, sound event và material. Giữ đường dẫn package và hash snapshot.', output:'Catalog có provenance + dependency checklist + load test.', route:'/06-Catalogs/README', next:'Một luồng native trọn vẹn'},
  {name:'Thực thi', title:'05 · Lần một hành động từ input đến hậu quả', question:'Đúng một phát đạn đi qua những hàm nào?', text:'Theo input → nhân vật → inventory/equip → weapon action → ammo commit → trace/projectile → hit/damage → AI reaction. Đặt breakpoint/log ở ranh giới state; không đọc hết code theo thứ tự thư mục.', output:'Trace của một phát đạn và một reload có bằng chứng.', route:'/03-Gun-Gameplay/README', next:'Cảm giác và thời gian'},
  {name:'Cảm giác', title:'06 · Ghép camera, tay, súng, tiếng và va chạm', question:'Luật đúng nhưng tại sao bắn vẫn khác?', text:'Tách camera recoil khỏi weapon/arm pose, HIP khỏi ADS, world FOV khỏi presentation, authored animation khỏi graph blending. So sánh cùng weapon, attachment, posture, frame rate và input; mỗi lần chỉ đổi một biến.', output:'Phiếu HIP/ADS single/burst/reload + số đo + đánh giá trực tiếp.', route:'/05-Gun-Lab/README', next:'Tích hợp hệ thống'},
  {name:'Tích hợp', title:'07 · Cho feature sống trong nhiệm vụ và mạng', question:'Nó còn đúng khi bị thương, đứng sát tường, bị hủy và có client khác?', text:'Kết nối weapon với movement, obstruction, interaction, AI perception, damage/ROE, save/load và multiplayer. Mỗi dependency có một owner. Test negative paths, không chỉ đường thuận lợi.', output:'Một lát nhiệm vụ chơi được từ đầu tới kết quả và chơi lại.', route:'/02-Gameplay-Va-AI/README', next:'Kiểm chứng và vận hành'},
  {name:'Kiểm chứng', title:'08 · Biết chính xác mình đã chứng minh điều gì', question:'Build pass, map mở, weapon equip và gunfeel có cùng nghĩa không?', text:'Không. Giữ receipt cho từng tầng: source evidence, compile, asset load, map generation, runtime equip/fire, two-client test và cảm nhận người chơi. Dữ liệu chưa kiểm tra phải ở trạng thái chưa kiểm tra.', output:'Acceptance matrix + regression + bản tái lập được + giới hạn rõ.', route:'/01-Ban-Do-Kien-Thuc/05-lo-trinh-thuc-hanh', next:'Lặp lại với feature tiếp theo'},
]
</script>
<template>
  <div class="knowledge-map">
    <div class="map-tabs" role="tablist" aria-label="Tám tầng kiến thức">
      <button v-for="(layer,i) in layers" :key="layer.name" :id="'knowledge-tab-'+i" role="tab" :tabindex="selected===i ? 0 : -1" :aria-selected="selected===i" aria-controls="knowledge-detail" @click="selected=i" @keydown="moveTab($event,i)">{{i+1}}. {{layer.name}}</button>
    </div>
    <section id="knowledge-detail" class="map-detail" role="tabpanel" :aria-labelledby="'knowledge-tab-'+selected" aria-live="polite">
      <div class="map-kicker">Từ quan sát đến tái tạo có bằng chứng</div>
      <h3>{{layers[selected].title}}</h3>
      <p><strong>{{layers[selected].question}}</strong></p>
      <p>{{layers[selected].text}}</p>
      <div class="map-outputs"><strong>Sản phẩm cần làm:</strong> {{layers[selected].output}}</div>
      <p><a :href="withBase(layers[selected].route)">Đi vào tầng này →</a></p>
    </section>
  </div>
</template>

import { readdirSync, readFileSync, existsSync } from 'node:fs'
import { resolve, join } from 'node:path'
import { defineConfig, type DefaultTheme } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

const root = resolve(import.meta.dirname, '..')
const labels: Record<string, string> = {
  '01': '01 · Bắt đầu & bản đồ kiến thức',
  '02': '02 · Gameplay & trí tuệ nhân tạo',
  '03': '03 · Giải phẫu Gun Gameplay',
  '04': '04 · Kiến trúc & quy trình dự án',
  '05': '05 · Native Gun Lab',
  '06': '06 · Catalog & bằng chứng',
}
function title(file: string) {
  return (readFileSync(file, 'utf8').match(/^#\s+(.+)$/m)?.[1] || file)
    .replace(/[*`]/g, '').trim()
}
const sections: DefaultTheme.SidebarItem[] = readdirSync(root, {withFileTypes: true})
  .filter(d => d.isDirectory() && /^0[1-6]-/.test(d.name))
  .sort((a, b) => a.name.localeCompare(b.name))
  .map(d => ({
    text: labels[d.name.slice(0, 2)] || d.name,
    collapsed: true,
    items: readdirSync(join(root, d.name)).filter(n => n.endsWith('.md'))
      .sort((a, b) => a === 'README.md' ? -1 : b === 'README.md' ? 1 : a.localeCompare(b, 'vi', {numeric: true}))
      .map(n => ({text: title(join(root, d.name, n)), link: '/' + d.name + '/' + n.replace(/\.md$/, '')})),
  }))

export default withMermaid(defineConfig({
  lang: 'vi-VN',
  title: 'ReadyOrNot Docs',
  titleTemplate: ':title · ReadyOrNot Docs',
  description: 'Từ bản đồ gameplay đến từng phát đạn: học dự án lớn qua source, asset, bằng chứng và native gun lab.',
  base: '/ReadyOrNot-Docs/',
  srcDir: '.site',
  cleanUrls: true,
  vite: { publicDir: resolve(root, '.site/public') },
  head: [
    ['link', { rel: 'icon', href: '/ReadyOrNot-Docs/favicon.svg', type: 'image/svg+xml' }],
    ['meta', { name: 'theme-color', content: '#14201d' }],
    ['meta', { property: 'og:site_name', content: 'ReadyOrNot Docs' }],
    ['meta', { property: 'og:locale', content: 'vi_VN' }],
  ],
  markdown: { lineNumbers: true, image: { lazyLoading: true } },
  mermaid: { startOnLoad: false, securityLevel: 'strict', theme: 'neutral' },
  themeConfig: {
    logo: '/favicon.svg',
    siteTitle: 'ReadyOrNot Docs',
    nav: [
      {text: 'Bắt đầu', link: '/BAT-DAU-TU-DAY'},
      {text: 'Bản đồ', link: '/01-Ban-Do-Kien-Thuc/01-ban-do'},
      {text: 'Gun Gameplay', link: '/03-Gun-Gameplay/README'},
      {text: 'Gun Lab', link: '/05-Gun-Lab/README'},
      {text: 'Tra cứu', link: '/06-Catalogs/README'},
    ],
    sidebar: [
      {text: 'Cách dùng bộ sách', items: [
        {text: 'Bắt đầu từ đây', link: '/BAT-DAU-TU-DAY'},
        {text: 'Mục lục toàn bộ', link: '/00-MucLuc'},
        {text: 'Phạm vi & mức bằng chứng', link: '/gioi-thieu'},
      ]},
      ...sections,
    ],
    outline: {label: 'Trong chương này', level: [2, 3]},
    search: {
      provider: 'local',
      options: {
        miniSearch: {options: {processTerm: (term: string) => term.normalize('NFD').replace(/[\u0300-\u036f]/g, '').replace(/[đĐ]/g, 'd').toLowerCase()}},
        locales: {root: {translations: {
          button: {buttonText: 'Tìm trong sách', buttonAriaLabel: 'Tìm trong sách'},
          modal: {noResultsText: 'Không tìm thấy', resetButtonTitle: 'Xóa từ khóa', backButtonTitle: 'Đóng', displayDetails: 'Hiện đoạn văn', footer: {selectText: 'chọn', navigateText: 'di chuyển', closeText: 'đóng'}},
        }}},
      },
    },
    socialLinks: [{icon: 'github', link: 'https://github.com/SlimeVRX/ReadyOrNot-Docs'}],
    docFooter: {prev: 'Chương trước', next: 'Chương tiếp'},
    darkModeSwitchLabel: 'Giao diện',
    sidebarMenuLabel: 'Mục lục',
    returnToTopLabel: 'Về đầu trang',
    skipToContentLabel: 'Đi đến nội dung',
    footer: {message: 'Nghiên cứu độc lập từ snapshot cục bộ · Học, đối chiếu, thực hành', copyright: 'ReadyOrNot Docs · SlimeVRX'},
  },
}))

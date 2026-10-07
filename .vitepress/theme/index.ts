import DefaultTheme from 'vitepress/theme'
import type { Theme } from 'vitepress'
import KnowledgeMap from './KnowledgeMap.vue'
import WeaponCatalog from './WeaponCatalog.vue'
import './custom.css'

export default {
  extends: DefaultTheme,
  enhanceApp({app}) {
    app.component('KnowledgeMap', KnowledgeMap)
    app.component('WeaponCatalog', WeaponCatalog)
  },
} satisfies Theme

#!/usr/bin/env node
/* Regenerate the LVGL font from actual application strings.
   Usage: node tools/prepare_niuma_font.cjs FONT.otf LV_FONT_CONV.js */
const fs=require('fs'),path=require('path'),cp=require('child_process');
const [font,converter]=process.argv.slice(2);
if(!font||!converter){console.error('Usage: prepare_niuma_font.cjs FONT.otf LV_FONT_CONV.js');process.exit(1);}
const root=path.resolve(__dirname,'..');
const files=fs.readdirSync(path.join(root,'main')).filter(n=>n.startsWith('niuma_')&&/\.[ch]$/.test(n));
const text=files.map(n=>fs.readFileSync(path.join(root,'main',n),'utf8')).join('');
const symbols=[...new Set([...text].filter(c=>c.codePointAt(0)>127))].sort().join('');
const dest=path.join(root,'assets/fonts/niuma_font.c');
fs.mkdirSync(path.dirname(dest),{recursive:true});
cp.execFileSync(process.execPath,[converter,'--font',path.resolve(font),'--size','14','--bpp','4','--format','lvgl','--range','0x20-0x7e','--symbols',symbols,'--no-compress','--lv-font-name','niuma_font','-o',dest],{stdio:'inherit'});
console.log(`Generated ${dest}: ${symbols.length} non-ASCII characters plus ASCII`);
fs.writeFileSync(dest,fs.readFileSync(dest,'utf8').trimEnd()+'\n');

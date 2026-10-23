import './styles.css';
import './product.css';
import './brand.css';
import { mountAnchorNavigation } from './core/anchor-navigation';
import { mountReticle } from './core/reticle';
const motion=matchMedia('(prefers-reduced-motion: reduce)');
const reduced=motion.matches||new URLSearchParams(location.search).has('reduce-motion');
document.documentElement.classList.toggle('reduce-motion',reduced);
document.documentElement.dataset.motion=reduced?'reduced':'full';
const events=new AbortController(),options={signal:events.signal};
const page=document.querySelector<HTMLElement>('.page-scroll')!;
if(location.pathname==='/'||location.pathname==='/index.html')document.querySelectorAll<HTMLAnchorElement>('a[href^="/#"]').forEach(a=>a.setAttribute('href',a.hash));
const navigation=mountAnchorNavigation(page);
// Deep links expose the requested answer before the anchor is aligned.
function revealAnswer(hash:string){
 let target:HTMLElement|null=null;
 try{target=document.getElementById(decodeURIComponent(hash.slice(1)));}catch{}
 if(target instanceof HTMLDetailsElement)target.open=true;
}
revealAnswer(location.hash);
window.addEventListener('hashchange',()=>revealAnswer(location.hash),options);
document.querySelectorAll<HTMLAnchorElement>('a[href^="#"]').forEach(link=>link.addEventListener('click',()=>revealAnswer(link.hash),options));

motion.addEventListener('change',()=>location.reload(),options);
document.querySelectorAll('[data-year]').forEach(n=>n.textContent=String(new Date().getFullYear()));
document.querySelectorAll<HTMLElement>('[data-tabs]').forEach(group=>{
 const tabs=[...group.querySelectorAll<HTMLButtonElement>('[role=tab]')];
 function select(index:number,focus=false){
  tabs.forEach((tab,i)=>{tab.setAttribute('aria-selected',String(i===index));tab.tabIndex=i===index?0:-1;document.getElementById(tab.getAttribute('aria-controls')!)!.hidden=i!==index;});
  if(focus){
   const tab=tabs[index];tab.focus({preventScroll:true});
   if(group.scrollWidth>group.clientWidth){const left=tab.offsetLeft,right=left+tab.offsetWidth;if(left<group.scrollLeft)group.scrollLeft=left;else if(right>group.scrollLeft+group.clientWidth)group.scrollLeft=right-group.clientWidth;}
  }
 }
 tabs.forEach((tab,index)=>{
  tab.addEventListener('click',()=>select(index),options);
  tab.addEventListener('keydown',e=>{
   let next=index;if(e.key==='ArrowRight')next=(index+1)%tabs.length;else if(e.key==='ArrowLeft')next=(index+tabs.length-1)%tabs.length;else if(e.key==='Home')next=0;else if(e.key==='End')next=tabs.length-1;else return;
   e.preventDefault();select(next,true);
  },options);
 });select(0);
});
const toggle=document.querySelector<HTMLButtonElement>('.menu-toggle')!;
const menu=document.querySelector<HTMLDialogElement>('#site-menu')!;
const viewer=document.querySelector<HTMLDialogElement>('#image-viewer')!;
let previousFocus:HTMLElement|null=null,navigationTarget:HTMLElement|null=null;
function open(dialog:HTMLDialogElement){previousFocus=document.activeElement as HTMLElement;dialog.showModal();page.classList.add('is-locked');dialog.querySelector<HTMLElement>('h2')!.focus({preventScroll:true});if(dialog===menu)toggle.setAttribute('aria-expanded','true');}
for(const dialog of [menu,viewer]){
 dialog.querySelector('[data-close]')!.addEventListener('click',()=>dialog.close(),options);
 dialog.addEventListener('close',()=>{page.classList.remove('is-locked');if(dialog===menu)toggle.setAttribute('aria-expanded','false');if(dialog===viewer)viewer.querySelector('img')!.removeAttribute('src');if(navigationTarget){navigationTarget.setAttribute('tabindex','-1');navigationTarget.focus({preventScroll:true});navigationTarget=null;}else previousFocus?.focus({preventScroll:true});},options);
 dialog.addEventListener('click',e=>{if(e.target!==dialog)return;const r=dialog.getBoundingClientRect();if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom)dialog.close();},options);
 dialog.addEventListener('keydown',e=>{if(e.key!=='Tab')return;const controls=[...dialog.querySelectorAll<HTMLElement>('button,a[href],[tabindex="0"]')],first=controls[0],last=controls.at(-1)!;if(e.shiftKey&&(document.activeElement===first||document.activeElement===dialog.querySelector('h2'))){e.preventDefault();last.focus();}else if(!e.shiftKey&&document.activeElement===last){e.preventDefault();first.focus();}},options);
}
toggle.addEventListener('click',()=>open(menu),options);
menu.querySelectorAll<HTMLAnchorElement>('a').forEach(link=>link.addEventListener('click',()=>{if(link.origin===location.origin&&link.pathname===location.pathname&&link.hash)navigationTarget=document.getElementById(decodeURIComponent(link.hash.slice(1)));page.classList.remove('is-locked');menu.close();},options));
document.querySelectorAll<HTMLAnchorElement>('.screenshot-link').forEach(link=>link.addEventListener('click',e=>{if(e.button||e.metaKey||e.ctrlKey||e.shiftKey||e.altKey)return;e.preventDefault();const large=document.querySelector<HTMLImageElement>('#viewer-image')!;large.src=link.href;large.alt=link.querySelector('img')!.alt;document.querySelector('#image-title')!.textContent=link.dataset.imageTitle??'Product screenshot';open(viewer);},options));
const host=document.querySelector<HTMLElement>('.point-field');
const reticle=host?mountReticle(host,reduced):null;
const copy=document.querySelector<HTMLButtonElement>('.copy-button');
copy?.addEventListener('click',async()=>{
 const code=document.querySelector<HTMLElement>('#launch-options')!,status=document.querySelector<HTMLElement>('#copy-status')!;
 copy.disabled=true;
 try{await navigator.clipboard.writeText(code.textContent!);status.textContent='Launch options copied.';}
 catch{const range=document.createRange();range.selectNodeContents(code);const selection=window.getSelection();selection?.removeAllRanges();selection?.addRange(range);status.textContent='Launch options selected. Copy with your keyboard or selection menu.';}
 finally{copy.disabled=false;}
},options);
document.documentElement.classList.remove('no-js');
const metrics:PerformanceObserver[]=[];let cls=0;document.documentElement.dataset.cls='0';
try{const o=new PerformanceObserver(list=>{for(const e of list.getEntries())document.documentElement.dataset.lcp=e.startTime.toFixed(1);});o.observe({type:'largest-contentful-paint',buffered:true});metrics.push(o);}catch{}
try{const o=new PerformanceObserver(list=>{for(const e of list.getEntries()){const s=e as PerformanceEntry&{hadRecentInput:boolean;value:number};if(!s.hadRecentInput)cls+=s.value;}document.documentElement.dataset.cls=cls.toFixed(4);});o.observe({type:'layout-shift',buffered:true});metrics.push(o);}catch{}
if(import.meta.hot)import.meta.hot.dispose(()=>{events.abort();reticle?.destroy();navigation.destroy();metrics.forEach(o=>o.disconnect());for(const d of [menu,viewer])if(d.open)d.close();page.classList.remove('is-locked');});

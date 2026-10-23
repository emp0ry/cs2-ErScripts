import test from 'node:test';
import assert from 'node:assert/strict';
import { mountReticle } from '../src/core/reticle.ts';

function environment(available=true) {
    const callbacks=new Map<number,FrameRequestCallback>(),classes=new Set<string>();
    let id=0,now=1000,draws=0,disconnects=0,top=0,visible!:IntersectionObserverCallback;
    const scrollport=Object.assign(new EventTarget(),{getBoundingClientRect:()=>({top:0,height:800})});
    const control=Object.assign(new EventTarget(),{hidden:true});
    const context={clearRect(){draws++;},fillStyle:'',globalAlpha:1,beginPath(){},rect(){},fill(){},setTransform(){}};
    const canvas={width:0,height:0,dataset:{} as Record<string,string>,getContext:()=>available?context:null};
    const host={querySelector:(selector:string)=>selector==='canvas'?canvas:control,closest:()=>scrollport,
        getBoundingClientRect:()=>({left:0,top,width:500,height:460}),
        classList:{add:(name:string)=>classes.add(name),remove:(name:string)=>classes.delete(name)}};
    const document=Object.assign(new EventTarget(),{hidden:false});
    const window=new EventTarget();
    const globals={document,window,location:{search:''},innerWidth:1280,devicePixelRatio:1,
        requestAnimationFrame:(callback:FrameRequestCallback)=>{callbacks.set(++id,callback);return id;},cancelAnimationFrame:(id:number)=>callbacks.delete(id),
        ResizeObserver:class{observe(){}disconnect(){disconnects++;}},
        IntersectionObserver:class{constructor(callback:IntersectionObserverCallback){visible=callback;}observe(){}disconnect(){disconnects++;}},
    };
    const previous=Object.fromEntries(Object.keys(globals).map(key=>[key,Object.getOwnPropertyDescriptor(globalThis,key)]));
    Object.entries(globals).forEach(([key,value])=>Object.defineProperty(globalThis,key,{configurable:true,writable:true,value}));
    return {callbacks,canvas,classes,control,document,window,get draws(){return draws;},get disconnects(){return disconnects;},
        mount:(reduced=false)=>mountReticle(host as unknown as HTMLElement,reduced),
        step(){const pending=[...callbacks.values()];callbacks.clear();now+=34;pending.forEach(callback=>callback(now));},
        visibility(value:boolean){visible([{isIntersecting:value}] as IntersectionObserverEntry[],{} as IntersectionObserver);},
        scroll(value:number){top=value;scrollport.dispatchEvent(new Event('scroll'));},
        restore(){for(const [key,value]of Object.entries(previous)){if(value)Object.defineProperty(globalThis,key,value);else Reflect.deleteProperty(globalThis,key);}},
    };
}
test('the reticle pauses offscreen/hidden and releases its observers, frames, controls and input',()=>{
    const e=environment(),mark=e.mount();
    try{
        assert(e.classes.has('is-ready'));assert(!e.control.hidden);assert.equal(e.callbacks.size,1);
        const first=e.draws;e.step();assert(e.draws>first);
        e.visibility(false);assert.equal(e.callbacks.size,0);assert.equal(e.canvas.dataset.state,'paused');
        e.visibility(true);assert.equal(e.callbacks.size,1);
        e.document.hidden=true;e.document.dispatchEvent(new Event('visibilitychange'));assert.equal(e.callbacks.size,0);
        e.document.hidden=false;e.document.dispatchEvent(new Event('visibilitychange'));assert.equal(e.callbacks.size,1);
        mark.destroy();assert.equal(e.callbacks.size,0);assert.equal(e.disconnects,2);assert(e.control.hidden);assert(!e.classes.has('is-ready'));
        e.window.dispatchEvent(new Event('pageshow'));assert.equal(e.callbacks.size,0);
    }finally{mark.destroy();e.restore();}
});
test('native scrolling pauses and resumes the reticle even when observer updates are delayed',()=>{
    const e=environment(),mark=e.mount();
    try{
        e.scroll(-500);assert.equal(e.canvas.dataset.state,'paused');assert.equal(e.callbacks.size,0);
        const draws=e.draws;e.step();assert.equal(e.draws,draws);
        e.scroll(100);assert.equal(e.canvas.dataset.state,'running');assert.equal(e.callbacks.size,1);
    }finally{mark.destroy();e.restore();}
});
test('reduced motion draws one fixed pose with no decorative control or animation frame',()=>{
    const e=environment(),mark=e.mount(true);
    try{assert.equal(e.draws,1);assert.equal(e.callbacks.size,0);assert(e.control.hidden);assert.equal(e.canvas.dataset.state,'static');}
    finally{mark.destroy();e.restore();}
});
test('unavailable canvas leaves the SVG fallback in place',()=>{
    const e=environment(false),mark=e.mount();
    try{assert.equal(e.draws,0);assert.equal(e.callbacks.size,0);assert(!e.classes.has('is-ready'));assert(e.control.hidden);}
    finally{mark.destroy();e.restore();}
});

import { createReticlePoints, sampleReticle, LOOP_SECONDS, type ReticleFrame } from './reticle-field.ts';
import { createPulseTrain } from './pulses.ts';

export function mountReticle(host: HTMLElement, reducedMotion: boolean) {
    const canvas = host.querySelector<HTMLCanvasElement>('canvas')!;
    const control = host.querySelector<HTMLButtonElement>('.reticle-control')!;
    const events = new AbortController(), options = { signal: events.signal };
    const waves = createPulseTrain(1.7);
    const points = createReticlePoints(innerWidth < 700 ? 82 : 100);
    const sample = new Float32Array(4);
    const projected = new Float32Array(points.length);
    let ctx: CanvasRenderingContext2D | null = null;
    if (!new URLSearchParams(location.search).has('no-canvas')) try { ctx = canvas.getContext('2d'); } catch {}
    if (!ctx) return { destroy() {} };
    const context = ctx;
    const scrollport = host.closest<HTMLElement>('.page-scroll');
    let width=1,height=1,scale=1,ratio=1,frame=0,previous=0,elapsed=0,intro=reducedMotion?1:0;
    let visible=true,destroyed=false,draws=0,lastDraw=0,lastMetric=0;
    let px=0,py=0,strength=0,targetX=0,targetY=0,targetStrength=0;
    const costs:number[]=[],intervals:number[]=[];
    const state: ReticleFrame = { phase:0,intro:1,pointerX:0,pointerY:0,strength:0,waves:waves.frame };
    function draw() {
        const started = performance.now();
        context.clearRect(0,0,width,height);
        state.phase = elapsed/LOOP_SECONDS; state.intro=intro; state.pointerX=px; state.pointerY=py; state.strength=strength;
        for(let i=0;i<points.length;i+=4) {
            sampleReticle(points,i,state,sample);
            projected.set(sample,i);
        }
        // Batch similar opacity into eight paths rather than changing state per point.
        for (let bucket=0;bucket<8;bucket++) {
            context.fillStyle=bucket>5?'#e0d1f5':'#b7a0e5';
            context.globalAlpha=(bucket+.5)/8;
            context.beginPath();
            for(let i=0;i<points.length;i+=4) {
                if (Math.min(7,Math.floor(projected[i+2]*8))!==bucket) continue;
                const size=projected[i+3]*(width<400?1.05:1.15);
                context.rect(width/2+projected[i]*scale-size/2,height/2-projected[i+1]*scale-size/2,size,size);
            }
            context.fill();
        }
        context.globalAlpha=1; draws++;
        costs.push(performance.now()-started);
        if (costs.length>90) costs.shift();
        canvas.dataset.draws=String(draws);
        canvas.dataset.activePulses=String(waves.frame.count);
    }
    function stop() { cancelAnimationFrame(frame);frame=0;previous=0;lastDraw=0; }
    function tick(now:number) {
        if(destroyed)return;
        const delta=previous?Math.min(.05,(now-previous)/1000):0;previous=now;
        elapsed+=delta;intro=Math.min(1,intro+delta/1.3);
        const follow=1-Math.exp(-delta/0.14);
        px+=(targetX-px)*follow;py+=(targetY-py)*follow;strength+=(targetStrength-strength)*follow;
        waves.advance(delta);
        if(!lastDraw||now-lastDraw>=1000/30-1) {
            if(lastDraw)intervals.push(now-lastDraw);
            if(intervals.length>90)intervals.shift();
            lastDraw=now;draw();
            if(now-lastMetric>1200) {
                const sorted=[...costs].sort((a,b)=>a-b),timing=[...intervals].sort((a,b)=>a-b);
                canvas.dataset.drawP95=(sorted[Math.floor(sorted.length*.95)]??0).toFixed(2);
                canvas.dataset.frameMedian=(timing[Math.floor(timing.length*.5)]??0).toFixed(1);
                lastMetric=now;
            }
        }
        frame=requestAnimationFrame(tick);
    }
    function sync() {
        const running=!reducedMotion&&visible&&!document.hidden&&!destroyed;
        canvas.dataset.state=running?'running':reducedMotion?'static':'paused';
        if(running&&!frame)frame=requestAnimationFrame(tick);else if(!running)stop();
    }
    function updateVisibility() {
        const rect = host.getBoundingClientRect();
        const viewport = scrollport?.getBoundingClientRect();
        visible = rect.top + rect.height > (viewport?.top ?? 0) && rect.top < (viewport ? viewport.top + viewport.height : innerHeight);
        sync();
    }
    function resize() {
        const rect=host.getBoundingClientRect();width=Math.max(1,rect.width);height=Math.max(1,rect.height);
        ratio=Math.min(devicePixelRatio||1,2);scale=Math.min(width,height)*.40;
        canvas.width=Math.round(width*ratio);canvas.height=Math.round(height*ratio);
        context.setTransform(ratio,0,0,ratio,0,0);draw();
        host.classList.add('is-ready');
        updateVisibility();
    }
    function coordinates(event:PointerEvent) {
        const rect=host.getBoundingClientRect();return [(event.clientX-rect.left-width/2)/scale,-(event.clientY-rect.top-height/2)/scale];
    }
    control.addEventListener('pointermove',event=>{
        if(reducedMotion||event.pointerType==='touch')return;
        [targetX,targetY]=coordinates(event);targetStrength=1;
    },options);
    control.addEventListener('pointerleave',()=>{targetStrength=0;},options);
    control.addEventListener('pointerdown',event=>{
        if(reducedMotion||event.button!==0)return;
        const [x,y]=coordinates(event);waves.push(x,y);
    },{...options,passive:true});
    control.addEventListener('click',event=>{if(!reducedMotion&&event.detail===0)waves.push(targetX,targetY);},options);
    control.addEventListener('keydown',event=>{
        if(reducedMotion)return;
        if(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown'].includes(event.key)) {
            event.preventDefault();targetStrength=1;
            if(event.key==='ArrowLeft')targetX-=.15;if(event.key==='ArrowRight')targetX+=.15;
            if(event.key==='ArrowUp')targetY+=.15;if(event.key==='ArrowDown')targetY-=.15;
            targetX=Math.max(-1,Math.min(1,targetX));targetY=Math.max(-1,Math.min(1,targetY));
        }else if(event.key==='Escape'){targetStrength=0;targetX=targetY=0;waves.clear();}
    },options);
    control.addEventListener('blur',()=>{targetStrength=0;},options);
    document.addEventListener('visibilitychange',updateVisibility,options);
    window.addEventListener('pagehide',stop,options);window.addEventListener('pageshow',updateVisibility,options);
    // Explicit scroll-layer bounds also cover engines that defer observer updates
    // inside a fixed, clipped scroll container.
    scrollport?.addEventListener('scroll',updateVisibility,{...options,passive:true});
    const resizeObserver=new ResizeObserver(resize);
    const observer=new IntersectionObserver(entries=>{visible=entries[0].isIntersecting;if(visible)updateVisibility();else sync();},{root:scrollport});
    resizeObserver.observe(host);observer.observe(host);resize();updateVisibility();control.hidden=reducedMotion;
    return { destroy(){destroyed=true;stop();events.abort();resizeObserver.disconnect();observer.disconnect();waves.clear();host.classList.remove('is-ready');control.hidden=true;canvas.width=canvas.height=1;} };
}

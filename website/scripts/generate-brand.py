"""Generate outlined Geist/point-reticle brand assets. Requires fonttools[woff] and ImageMagick."""
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.varLib.instancer import instantiateVariableFont
import subprocess, json

source = TTFont('src/public/site/fonts/Geist.woff2')
cmap = source.getBestCmap(); upm = source['head'].unitsPerEm
fonts = {w: instantiateVariableFont(source, {'wght': w}, inplace=False).getGlyphSet() for w in [500,600,900]}
def text(value, x, y, size, weight=500, fill='#f4f2f7', tracking=-.025):
    glyphs=fonts[weight]; advance=0; paths=[]
    for character in value:
        name=cmap[ord(character)]; pen=SVGPathPen(glyphs); glyphs[name].draw(pen)
        paths.append(f'<path transform="translate({advance:.2f})" d="{pen.getCommands()}"/>')
        advance += glyphs[name].width + tracking*upm
    return f'<g fill="{fill}" transform="translate({x} {y}) scale({size/upm} {-size/upm})">'+''.join(paths)+'</g>'
code = """import {createReticlePoints,sampleReticle} from './src/core/reticle-field.ts';
const p=createReticlePoints(62),out=new Float32Array(4),data=[];
const f={phase:0,intro:1,pointerX:0,pointerY:0,strength:0,waves:{data:new Float32Array(96),count:0}};
for(let i=0;i<p.length;i+=4){sampleReticle(p,i,f,out);data.push(Array.from(out));}
process.stdout.write(JSON.stringify(data));"""
points=json.loads(subprocess.check_output(['node','--input-type=module','-e',code],text=True))
def reticle(x,y,scale,radius):
    return '<g fill="#b7a0e5">'+''.join(f'<circle cx="{x+p[0]*scale:.2f}" cy="{y-p[1]*scale:.2f}" r="{p[3]*radius:.2f}" opacity="{p[2]:.2f}"/>' for p in points)+'</g>'
Path('src/public/site/reticle.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" width="600" height="600" viewBox="0 0 600 600">'+reticle(300,300,240,.65)+'</svg>')
parts=['<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="630" viewBox="0 0 1200 630"><title>ErScripts — Your game. In focus.</title><rect width="1200" height="630" fill="#09090d"/>',
       text('emp0ry.',64,68,27,900,tracking=-.07),text('/ ERSCRIPTS',193,66,12,500,fill='#aaa6b5',tracking=.03),
       reticle(918,292,180,.75),text('ErScripts',64,254,92,600,tracking=-.055),text('Your game.',68,327,37),text('In focus.',68,375,37,fill='#b7a0e5'),
       '<path d="M64 508H1136" stroke="#c7bcda" stroke-opacity=".24"/>',
       text('Windows / Counter-Strike 2',68,554,16,fill='#aaa6b5'),text('erscripts.emp0ry.com',919,554,15,fill='#cbb6ef'),'</svg>']
Path('design/social-preview.svg').write_text('\n'.join(parts))
subprocess.run(['magick','-background','#09090d','design/social-preview.svg','-depth','8','src/public/site/social-preview.png'],check=True)
print('Generated SVG reticle fallback and 1200×630 social PNG.')

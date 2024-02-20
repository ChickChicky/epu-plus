'use strict';

// -- HTMLGE -- //

const canvas = document.querySelector('canvas');
const ctx = canvas.getContext('2d');

const RESCALE_PREC = 1;

const env = {
    w : 64,
    h : 64,
    /** @type {number} */
    s : 1,
    _s : null,
    /** @type {ImageData} the raw screen image data */
    screen : undefined,
    /** @type {ImageData} the raw screen image data */
    screen_back : undefined,
    mx : -1,
    my : -1,
    k : {},
};

canvas.onmousemove =
    e => {
        env.mx = e.offsetX;
        env.my = e.offsetY;
    }
;

document.onkeydown =
    e => {
        env.k[e.key] = true;
    }
;

document.onkeyup =
    e => {
        delete env.k[e.key];
    }
;

{
    let w = -1;
    let h = -1;
    function update() {
        env.s = env._s;
        if (env._s == null)
            env.s = Math.floor(Math.min(window.innerHeight/env.h,window.innerWidth/env.w)*RESCALE_PREC)/RESCALE_PREC;
        if (w != env.w || h != env.h || !env.screen) {
            w = env.w;
            h = env.h;
            let ns = ctx.createImageData(env.w,env.h);
            if (env.screen) {
                for (let x = 0; x < ns.width && x < env.screen.width; x++) {
                    for (let y = 0; y < ns.height && y < env.screen.height; y++) {
                        let esi = (env.screen.width*y+x)*4;
                        let nsi = (ns.width*y+x)*4;
                        ns.data[nsi] = env.screen.data[esi];
                        ns.data[nsi+1] = env.screen.data[esi+1];
                        ns.data[nsi+2] = env.screen.data[esi+2];
                    }
                }
            }
            if (env.screen_back) {
                for (let x = 0; x < ns.width && x < env.screen_back.width; x++) {
                    for (let y = 0; y < ns.height && y < env.screen_back.height; y++) {
                        let esi = (env.screen_back.width*y+x)*4;
                        let nsi = (ns.width*y+x)*4;
                        ns.data[nsi] = env.screen_back.data[esi];
                        ns.data[nsi+1] = env.screen_back.data[esi+1];
                        ns.data[nsi+2] = env.screen_back.data[esi+2];
                    }
                }
            }
            for (let i = 0; i < ns.width*ns.height; i++) ns.data[i*4+3] = 255;
            env.screen = ns;
            env.screen_back = ns;
        }
        canvas.width = env.w;
        canvas.height = env.h;
        canvas.style.width = `${env.w*env.s}px`;
        canvas.style.height = `${env.h*env.s}px`;
        ctx.putImageData(env.screen,0,0);
        requestAnimationFrame(update);
    }
    update();
    env.update = update;
}

class ColorValue {
    constructor (r,g,b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }
    asRGB() {
        return this.r | (this.g << 8) | (this.b << 16)
    }
}

/** @class @returns {ColorValue} */ function Color(r,g,b) { return new ColorValue(r,g,b); } // small utility

const ALPHA = 0xff000000;

/**
 * Sets the size of the screen
 * @param {number?} w the new width of the screen
 * @param {number?} h the new height of the screen
 */
function screenSize( w, h ) {
    if (w != undefined && h != undefined) {
        env.w = w;
        env.h = h;
        env.update();
    }
    return [env.w,env.h];
}

/**
 * Sets the scale of the screen
 * `null` can also be passed to always get the biggest scale possible
 * @param {number|null} s the new scale of the screen
 */
function screenScale( s ) {
    env._s = s;
    env.update();
}

/**
 * The color
 * @param {number} x 
 * @param {number} y 
 * @param {ColorValue} color 
 */
function screenSet( x, y, color ) {
    const i = (Math.floor(y)*env.w+Math.floor(x))*4;
    env.screen.data[i]   = color.r;
    env.screen.data[i+1] = color.g;
    env.screen.data[i+2] = color.b;
    env.screen.data[i+3] = 255;
}

/**
 * Retrieves the position of the mouse
 * @returns {[number,number]}
 */
function mousePos() {
    return [Math.max(0,Math.min(env.w-1,env.mx/env.s)),Math.max(0,Math.min(env.h-1,env.my/env.s))];
}
/**
 * Retrieves the position of the mouse as an integer
 * @returns {[number,number]}
 */
function mousePosI() {
    return [Math.floor(Math.max(0,Math.min(env.w-1,env.mx/env.s))),Math.floor(Math.max(0,Math.min(env.h-1,env.my/env.s)))];
}

function keyPressed( k ) {
    return !!env.k[k];
}

// -- THE "GAME" -- //

screenSize(256,256);

/** @type {WebAssembly.Instance} */
var instance;
/** @type {Uint8Array}  */
var memory;
/** @type {DataView} */
var memory_view;

const WasmLib = {
    'env': {
        print: (...args) => {
            console.log(args);
        },

        ge_screen_size: (w,h) => {
            screenSize(w,h);
        },

        ge_screen_set: (data,x,y,w,h) => {
            const [ww,hh] = screenSize();
            for (let xx = 0; xx < w && xx+x < ww; xx++) {
                for (let yy = 0; yy < h && yy+y < hh; yy++) {
                    let i = (x+xx+(y+yy)*ww)*3+data;
                    let j = (xx+yy*ww)*4;
                    env.screen.data[j+0] = memory[i+0];
                    env.screen.data[j+1] = memory[i+1];
                    env.screen.data[j+2] = memory[i+2];
                    env.screen.data[j+3] = 255;
                }
            }
        },

        ge_screen_push: () => {
            /* not implemented yet :p */
        },

        epu_load_floppy: (id,data_ptr,size_ptr) => {
            const floppy = floppies.get(id);
            if (!floppy) {
                return 0;
            }
            if (data_ptr) memory.set(floppy,data_ptr);
            if (size_ptr) memory_view.setUint32(size_ptr,floppy.length,true);
            return 1;
        }
    },
};

/** @type {Map<number,Uint8Array>} */
const floppies = new Map();

;(async()=>{

    try {
        const boot_floppy = await fetch('boot.img');
        if (boot_floppy.ok) {
            floppies.set(0,new Uint8Array(await boot_floppy.arrayBuffer()));
        }

        const wasm = await fetch('epu.wasm');
        ( { instance } = await WebAssembly.instantiate(await wasm.arrayBuffer(),WasmLib) );
        memory = new Uint8Array(instance.exports.memory.buffer);
        memory_view = new DataView(instance.exports.memory.buffer);
        
        instance.exports.main();
    } catch (e) {
        console.error(e);
        const [w,h] = screenSize();
        let xa = 0, ya = 0,
            xb = w, yb = h;
        let x = 0,
            y = 0;
        let d = 0;
        let i = 0;
        while (xa != xb && ya != yb) {
            screenSet(x,y,new Color(i/(w*h)*250,i/(w*h)*50,i/(w*h)*20));
            if (d == 0) {
                x++;
                if (x >= xb) {
                    x--;
                    xb--;
                    d++;
                }
            }
            if (d == 1) {
                y++;
                if (y >= yb) {
                    y--;
                    yb--;
                    d++;
                }
            }
            if (d == 2) {
                x--;
                if (x <= xa) {
                    x++;
                    xa++;
                    d++;
                }
            }
            if (d == 3) {
                y--;
                if (y <= ya) {
                    y++;
                    ya++;
                    d++;
                }
            }
            d = d%4;
            i++;
            if (!Math.floor(i%(w*h/1000))) await new Promise(r=>setInterval(r,1));
        }
    }

})();
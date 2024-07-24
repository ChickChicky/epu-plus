'use strict';

// -- HTMLGE -- //

const canvas = document.querySelector('canvas');
const ctx = canvas.getContext('2d');

const RESCALE_PREC = 1;

const env = {
    w : 64,
    h : 64,
    /** @type {number} */
    s  : 1,
    _s : null,
    /** @type {ImageData} the raw screen image data */
    screen : undefined,
    /** @type {ImageData} the raw screen image data */
    screen_back : undefined,
    mx : -1,
    my : -1,
    k  : {},
    ks : [],
};

canvas.onmousemove =
    e => {
        env.mx = e.offsetX;
        env.my = e.offsetY;
    }
;

document.onkeydown =
    e => {
        env.k[e.code] = true;
        if (!e.repeat)
            env.ks.push(e.key.toUpperCase());
    }
;

document.onkeyup =
    e => {
        delete env.k[e.code];
    }
;

{
    let w = -1;
    let h = -1;
    function update() {
        env.s = env._s;
        if (env._s == null)
            // Scale is forced to 5 for now
            env.s = 5 || Math.floor(Math.min(window.innerHeight/env.h,window.innerWidth/env.w)*RESCALE_PREC)/RESCALE_PREC;
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

const Key = {
    mapping: [
        [ 'KeyA', 'A', 65, 0, 0,  ],
        [ 'KeyB', 'B', 66, 0, 1,  ],
        [ 'KeyC', 'C', 67, 0, 2,  ],
        [ 'KeyD', 'D', 68, 0, 3,  ],
        [ 'KeyE', 'E', 69, 0, 4,  ],
        [ 'KeyF', 'F', 70, 0, 5,  ],
        [ 'KeyG', 'G', 71, 0, 6,  ],
        [ 'KeyH', 'H', 72, 0, 7,  ],
        [ 'KeyI', 'I', 73, 0, 8,  ],
        [ 'KeyJ', 'J', 74, 0, 9,  ],
        [ 'KeyK', 'K', 75, 0, 10, ],
        [ 'KeyL', 'L', 76, 0, 11, ],
        [ 'KeyM', 'M', 77, 0, 12, ],
        [ 'KeyN', 'N', 78, 0, 13, ],
        [ 'KeyO', 'O', 79, 0, 14, ],
        [ 'KeyP', 'P', 80, 0, 15, ],
        [ 'KeyQ', 'Q', 81, 0, 16, ],
        [ 'KeyR', 'R', 82, 0, 17, ],
        [ 'KeyS', 'S', 83, 0, 18, ],
        [ 'KeyT', 'T', 84, 0, 19, ],
        [ 'KeyU', 'U', 85, 0, 20, ],
        [ 'KeyV', 'V', 86, 0, 21, ],
        [ 'KeyW', 'W', 87, 0, 22, ],
        [ 'KeyX', 'X', 88, 0, 23, ],
        [ 'KeyY', 'Y', 89, 0, 24, ],
        [ 'KeyZ', 'Z', 90, 0, 25, ]
    ],
    code(k) {
        for (const [key, char, code, keyGroup, keyId] of this.mapping) {
            if (k == key || k == char) {
                return code;
            }
        }
    },
    key(k) {
        for (const [key, char, code, keyGroup, keyId] of this.mapping) {
            if (k == key || k == char) {
                return [keyGroup,keyId];
            }
        }
    },
};

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
            if (!floppy) return 0;
            if (data_ptr) memory.set(floppy,data_ptr);
            if (size_ptr) memory_view.setUint32(size_ptr,floppy.length,true);
            return 1;
        },

        ge_keys_last: () => {
            const [c] = env.ks.splice(0,1);
            if (typeof c == 'string') {
                const code = Key.code(c);
                return code ? code : 0;
            }
            return 0;
        },

        ge_keys_pressed: (ptr) => {
            let a = 0;
            let b = 0;
            for (const k in env.k) if (env.k[k]) {
                const v = Key.key(k);
                if (!v) continue;
                const [id,bit] = v;
                const u = 1 << bit;
                if (id == 0)
                    a |= u;
                if (id == 1)
                    b |= u;
            }
            memory_view.setInt32(ptr,a,true);
            memory_view.setInt32(ptr+4,b,true);
        },

        memset: (ptr,v,c) => {
            memory.fill(v,ptr,ptr+c);
        },

        debug: (...args) => {
            console.debug(...args);
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
        
        let init = instance.exports.init();
        
        if ( !init ) {
            /*while (true) {
                let status = instance.exports.loop();
                if (status) {
                    console.log('execution finished with status',status);
                    break;
                }
                await new Promise( r=>setTimeout(r,1) );
            }*/
           const i = setInterval(()=>{
            let status = instance.exports.loop(1024);
                if (status) {
                    console.log('execution finished with status',status);
                    clearInterval(i);
                }
           },0);
        } else {
            console.log('`init` failed with status',init);
        }
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
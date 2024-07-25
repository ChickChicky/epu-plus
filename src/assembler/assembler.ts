import fs from "node:fs";
import { parse, tokenize, Source, ParserNode, InstructionNode, ArgumentNode, evalError, Token, LabelNode } from "./asm-parser";

enum Mnem {
    reg  = 0,
    regp = 1,
    imdp = 2,
    imd  = 3,
};

enum Reg {
    ra = 0x00, ua = 0x08,
    rb = 0x01, ub = 0x09,
    rc = 0x02, uc = 0x0A,
    rd = 0x03, ud = 0x0B,
    re = 0x04, ue = 0x0C,
    rf = 0x05, uf = 0x0D,
    rg = 0x06, ug = 0x0E,
    rh = 0x07, uh = 0x0F,

    pc = 0x10, 
    sp = 0x11,
};

/** A number indicating the power of two corresponding to a size ( (2^n)*8 -> bits ) */
type SzA = 0 |  1 |  2 |  3;
/** A number indicating the amount of bytes corresponding to a size ( n*8 -> bits ) */
type SzS = 1 |  2 |  4 |  8;
/** A number indicating the amount of bits corresponding to a size */
type SzB = 8 | 16 | 32 | 64;

type Endian = 'le' | 'be';

type Val = {
    loc?: {
        addr: number,
        size: number,
        buffer: Buff
    },
    build?: (v: Val) => void,
    prebuild?: (v: Val) => void,
    val?: number,
    kind: Mnem,
};

class Buff {
    private ptr : number;
    private buff : Buffer;
    private vals : Val[];
    public endian : Endian;

    public constructor ( endian: Endian = 'le' ) {
        this.buff = Buffer.alloc(0);
        this.ptr = 0;
        this.endian = endian;
        this.vals = [];
    }

    public getSize() : number {
        return this.buff.length;
    }

    public build() : Buffer {
        for (const val of this.vals) {
            if (val.prebuild) {
                val.prebuild(val);
            }
            if (val.build && val.val !== undefined && val.loc) {
                val.build(val);
            }
        }
        const buff = Buffer.from(this.buff);
        return buff;
    }

    /**
     * Ensures n bytes are allocated at the given address, extending the internal buffer otherwise
     * (the new data may contain anything )
     */
    private _res( idx: number, len: number ) { // REServe
        if (idx + len > this.buff.length) {
            this.buff = Buffer.concat([this.buff,Buffer.allocUnsafe(idx+len-this.buff.length)]);
        }
    }

    /**
     * Ensures n bytes are allocated at the given address, extending the internal buffer otherwise
     */
    private _ress( idx: number, len: number ) { // REServe Safe
        if (idx + len > this.buff.length) {
            this.buff = Buffer.concat([this.buff,Buffer.alloc(idx+len-this.buff.length)]);
        }
    }

    public pushBuffer( b: Buffer ) : this {
        this._res(this.ptr,b.length);
        b.copy(this.buff,this.ptr,0,b.length);
        this.ptr += b.length;
        return this;
    }

    public pushUSized( sz: SzA, v: number ) : this {
        this._res(this.ptr,1<<sz);
        this.buff[`writeUInt${this.endian.toUpperCase()}`](v,this.ptr,1<<sz);
        this.ptr += 1<<sz;
        return this;
    }

    public pushU8( v: number ) : this {
        return this.pushUSized( 0, v );
    }

    public pushU16( v: number ) : this {
        return this.pushUSized( 1, v );
    }

    public pushU32( v: number ) : this {
        return this.pushUSized( 2, v );
    }

    public pushVal( v: Val, len: number ) : this {
        this._res(this.ptr,len);
        v.loc = {
            addr: this.ptr,
            size: len,
            buffer: this,
        };
        this.ptr += len;
        this.vals.push(v);
        return this;
    }

    public setBuffer( addr: number, b: Buffer ) : this {
        this._res(addr,b.length);
        b.copy(this.buff,addr,0,b.length);
        return this;
    }

    public setUSized( addr: number, sz: SzA, v: number ) : this {
        this._res(addr,1<<sz);
        this.buff[`writeUInt${this.endian.toUpperCase()}`](v,addr,1<<sz);
        return this;
    }

    public setSized( addr: number, sz: SzA, v: number ) : this {
        this._res(addr,1<<sz);
        this.buff[`writeInt${this.endian.toUpperCase()}`](v,addr,1<<sz);
        return this;
    }

    public setU8( addr: number, v: number ) : this {
        return this.setUSized( addr, 0, v );
    }

    public setU16( addr: number, v: number ) : this {
        return this.setUSized( addr, 1, v );
    }

    public setU32( addr: number, v: number ) : this {
        return this.setUSized( addr, 2, v );
    }

    public setVal( addr: number, v: Val, len: number ) : this {
        this._res(addr,len);
        v.loc = {
            addr,
            size: len,
            buffer: this,
        };
        this.vals.push(v);
        return this;
    }
}

interface Op {
    mnemonics : Mnem[][],
    build: ( buffer: Buff, size: SzA, args: Val[] ) => void,
}

const instructions : { [op: string]: {[k:string|number|symbol]:any}&Op } = {
    'mov' : {
        mnemonics : [
            [ Mnem.reg,  Mnem.reg  ],
            [ Mnem.reg,  Mnem.regp ],
            [ Mnem.reg,  Mnem.imdp ],
            [ Mnem.reg,  Mnem.imd  ],
            [ Mnem.regp, Mnem.reg  ],
            [ Mnem.regp, Mnem.imd  ],
            [ Mnem.imdp, Mnem.reg  ],
            [ Mnem.imdp, Mnem.imd  ]
        ],
        build ( buff, size, args ) {
            /* let sz = (([
                [ [ 'reg',  'reg'  ], 1             ],
                [ [ 'reg',  'reg*' ], 1             ],
                [ [ 'reg',  'imd*' ], 5             ],
                [ [ 'reg',  'imd'  ], 1 + (1<<size) ],
                [ [ 'reg*', 'reg'  ], 1             ],
                [ [ 'reg*', 'imd'  ], 1 + (1<<size) ],
                [ [ 'imd*', 'reg'  ], 5             ],
                 [ [ 'imd*', 'imd'  ], 4 + (1<<size) ]
            ] as [[string,string],number][]).find(([m])=>m.every((m,i)=>args[i].kind==m))||[])[1];*/
            buff
                .pushU8(0x02)
                .pushU8(size&15)
                .pushU8((args[0].kind)|((args[1].kind)<<4))
            ;
            let [a,b] = args;
            if (b.kind == Mnem.reg || b.kind == Mnem.regp) {
                buff.pushVal(b,1);
                b.build = () => { if (!b.loc || b.val==undefined || a.val==undefined) return;
                    buff.setU8(b.loc.addr,b.val|((a.val<<4)*+(a.kind==Mnem.reg||a.kind==Mnem.regp)));
                };
            }
            if (b.kind == Mnem.imdp) {
                buff.pushVal(b,4);
                b.build = () => { if (!b.loc || b.val==undefined) return;
                    buff.setU32(b.loc.addr,b.val);
                };
            }
            if (b.kind == Mnem.imd) {
                buff.pushVal(b,(1<<size));
                b.build = () => { if (!b.loc || b.val==undefined) return;
                    buff.setUSized(b.loc.addr,size,b.val);
                };
            }

            if ((a.kind == Mnem.reg || a.kind == Mnem.regp) && !(b.kind == Mnem.reg || b.kind == Mnem.regp)) {
                buff.pushVal(a,1);
                a.build = () => { if (!a.loc || a.val==undefined) return;
                    buff.setU8(a.loc.addr,a.val);
                };
            }
            if (a.kind == Mnem.imdp) {
                buff.pushVal(a,4);
                a.build = () => { if (!a.loc || a.val==undefined) return;
                    buff.setU32(a.loc.addr,a.val);
                };
            }
        }
    },
    'int' : {
        mnemonics : [
            [ Mnem.imd ]
        ],
        build ( buff, size, args ) {
            const [int] = args;
            buff
                .pushU8(0x06)
                .pushU8(0x00)
                .pushVal(int,4)
            ;
            int.build = () => { if (!int.loc || int.val == undefined) return;
                buff.setU32(int.loc.addr,int.val);
            }
        }
    },
    ...Object.fromEntries(([
        ['add', 0x00],
        ['sub', 0x01],
        ['mul', 0x02],
        ['div', 0x03],
        ['and', 0x04],
        ['or',  0x05],
        ['xor', 0x06],
        ['shl', 0x07],
        ['shr', 0x08],
    ] as [string,number][]).map(
        ([op,id]) => {
            return [
                op,
                {
                    mnemonics : [
                        [ Mnem.reg, Mnem.reg ],
                        [ Mnem.reg, Mnem.imd ],
                    ],
                    build ( buff, size, args ) {
                        const [a,b] = args;
                        buff
                            .pushU8(0x01)
                            .pushU8(size|(+(b.kind==Mnem.imd)<<4))
                            .pushU8(id)
                            .pushVal(a,1)
                        ;
                        if (b.kind == Mnem.imd)
                            buff.pushVal(b,(1<<size));
                        a.build = () => { if (!a.loc || !b.loc || a.val == undefined || b.val == undefined) return;
                            if (b.kind == Mnem.imd) {
                                buff.setU8(a.loc.addr,a.val);
                                buff.setUSized(b.loc.addr,size,b.val);
                            } else
                                buff.setU8(a.loc.addr,a.val|(b.val<<4));
                        }
                    }
                }
            ];
        }
    )),
    'cmp' : {
        mnemonics : [
            [ Mnem.imd,  Mnem.reg  ],
            [ Mnem.imd,  Mnem.regp ],
            [ Mnem.imd,  Mnem.imdp ],
            [ Mnem.imd,  Mnem.imd  ],
            [ Mnem.reg,  Mnem.reg  ],
            [ Mnem.reg,  Mnem.regp ],
            [ Mnem.reg,  Mnem.imdp ],
            [ Mnem.reg,  Mnem.imd  ],
            [ Mnem.regp, Mnem.reg  ],
            [ Mnem.regp, Mnem.imd  ],
            [ Mnem.imdp, Mnem.reg  ],
            [ Mnem.imdp, Mnem.imd  ]
        ],
        build ( buff, size, args ) {
            buff
                .pushU8(0x05)
                .pushU8(size&15)
                .pushU8((args[0].kind)|((args[1].kind)<<4))
            ;
            let [a,b] = args;
            if (b.kind == Mnem.reg || b.kind == Mnem.regp) {
                buff.pushVal(b,1);
                b.build = () => { if (!b.loc || b.val==undefined || a.val==undefined) return;
                    buff.setU8(b.loc.addr,b.val|((a.val<<4)*+(a.kind==Mnem.reg||a.kind==Mnem.regp)));
                };
            }
            if (b.kind == Mnem.imdp) {
                buff.pushVal(b,4);
                b.build = () => { if (!b.loc || b.val==undefined) return;
                    buff.setU32(b.loc.addr,b.val);
                };
            }
            if (b.kind == Mnem.imd) {
                buff.pushVal(b,(1<<size));
                b.build = () => { if (!b.loc || b.val==undefined) return;
                    buff.setUSized(b.loc.addr,size,b.val);
                };
            }

            if ((a.kind == Mnem.reg || a.kind == Mnem.regp) && !(b.kind == Mnem.reg || b.kind == Mnem.regp)) {
                buff.pushVal(a,1);
                a.build = () => { if (!a.loc || a.val==undefined) return;
                    buff.setU8(a.loc.addr,a.val);
                };
            }
            if (a.kind == Mnem.imdp) {
                buff.pushVal(a,4);
                a.build = () => { if (!a.loc || a.val==undefined) return;
                    buff.setU32(a.loc.addr,a.val);
                };
            }
            if (a.kind == Mnem.imd) {
                buff.pushVal(a,(1<<size));
                a.build = () => { if (!a.loc || a.val==undefined) return;
                    buff.setUSized(a.loc.addr,size,a.val);
                };
            }
        }
    },
    ...Object.fromEntries(([
       ['jmp', 0b10000],
       ['jeq', 0b001], ['jne', 0b110],
       ['jgt', 0b010], ['jge', 0b011],
       ['jlt', 0b100], ['jle', 0b101],
    ] as [string,number][]).map(
        ([name,mask]) => {
            return [name,{
                mnemonics : [
                    [ Mnem.imd ],
                    [ Mnem.imdp ],
                    [ Mnem.reg ],
                    [ Mnem.regp ],
                ],
                build ( buff, size, args ) {
                    const [src] = args;
                    const base = buff.getSize();
                    buff
                        .pushU8(0x04)
                        .pushU8(0)
                        .pushU8(src.kind)
                        .pushU8(mask)
                        .pushVal(src,1<<size)
                    ;
                    src.build = () => { if (!src.loc || src.val == undefined) return;
                        const diff = src.val - base; // TODO: Fix with variable address
                        const addr = src.loc.addr;
                        buff.setUSized(addr,size,Math.abs(diff));
                        buff.setU8(addr-3,size|(diff<0?32:0));
                    }
                }
            }];
        }
    )),
    'cal' : {
        mnemonics : [
            [ Mnem.imd ],
            [ Mnem.imdp ],
            [ Mnem.reg ],
            [ Mnem.regp ],
        ],
        build ( buff, size, args ) {
            const [src] = args;
            buff
                .pushU8(0x07)
                .pushU8(size)
                .pushU8(src.kind)
                .pushVal(src,1<<size)
            ;
            src.build = () => { if (!src.loc || src.val == undefined) return;
                const addr = (src.val&0xFFFFFF) | 0xFF000000; // TODO: Support for userspace
                buff.setSized(src.loc.addr,size,addr);
            }
        }
    },
    'ret' : {
        mnemonics : [],
        build ( buff, size, args ) {
            buff
                .pushU8(0x08)
                .pushU8(0)
            ;
        }
    }
};

type Instruction = 
    { type : 'opcode',
        name : Token,
        args : Val[],
        size : SzA
    } | { type : 'label',
        name : string,
    }
;

const [,,asmpath,outpath] = process.argv;

if (!asmpath) {
    console.error(
        `\x1b[31;1mERROR\x1b[39;22m: Missing assembly source argument`
    );
    process.exit(1);
}

/*let asmsource: string;
try {
    asmsource = fs.readFileSync(asmpath,'utf-8');
} catch (e) {
    if (e.code == 'ENOENT') {
        console.error(
            `\x1b[31;1mERROR\x1b[39;22m: Could not find file \`${e.path}\``
        );
        process.exit(1);
    } else {
        throw e;
    }
}*/

const vars: {[k:string]:Val} = {
    'ra' : { kind: Mnem.reg, val: 0x00 }, 'ua' : { kind: Mnem.reg, val: 0x08 },
    'rb' : { kind: Mnem.reg, val: 0x01 }, 'ub' : { kind: Mnem.reg, val: 0x09 },
    'rc' : { kind: Mnem.reg, val: 0x02 }, 'uc' : { kind: Mnem.reg, val: 0x0A },
    'rd' : { kind: Mnem.reg, val: 0x03 }, 'ud' : { kind: Mnem.reg, val: 0x0B },
    're' : { kind: Mnem.reg, val: 0x04 }, 'ue' : { kind: Mnem.reg, val: 0x0C },
    'rf' : { kind: Mnem.reg, val: 0x05 }, 'uf' : { kind: Mnem.reg, val: 0x0D },
    'rg' : { kind: Mnem.reg, val: 0x06 }, 'ug' : { kind: Mnem.reg, val: 0x0E },
    'rh' : { kind: Mnem.reg, val: 0x07 }, 'uh' : { kind: Mnem.reg, val: 0x0F },
    'pc' : { kind: Mnem.reg, val: 0x10 }, 
    'sp' : { kind: Mnem.reg, val: 0x11 },
};

const prog: Instruction[] = [];

// console.log(tokenize(Source.fromFile(asmpath)));
const ast = parse(Source.fromFile(asmpath));
// console.dir(ast,{depth:50,customInspect:true});

const refs: {name:string,obj:Val}[] = [];

function resolve(arg: ArgumentNode): Val {
    const argv = arg.value;
    if (argv.value.type == 'identifier') {
        const name = argv.value.token.val;
        const base = vars[name];
        const kind = base ? base.kind : Mnem.imd;
        const val = base ? base.val : undefined;
        const obj: Val = {
            kind,
            val,
            prebuild(v) {
                const value = vars[name];
                if (!value) {
                    throw evalError(argv.value.token.loc,'Unknown variable');
                }
                v.kind = value.kind;
                v.val = value.val;
            }
        };
        refs.push({name,obj});
        return obj;
    }
    else if (argv.value.type == 'number') {
        return {
            kind: argv.ptr ? Mnem.imdp : Mnem.imd,
            val: argv.value.value
        };
    }
    else {
        throw Error(`Unsupported argument type \`${argv.value.type}\``);
    }
}

function explore(node: ParserNode): Instruction | undefined {
    if (node instanceof InstructionNode) {
        const inst: Instruction = {
            type: 'opcode',
            name: node.name,
            args: [],
            size: (node.s == undefined ? 2 : {8:0,16:1,32:2}[node.s]) as SzA,
        };
        for (const arg of node.args)
            inst.args.push(resolve(arg));
        return inst;
    }
    else if (node instanceof LabelNode) {
        const label: Instruction = {
            type: 'label',
            name: node.name.val
        };
        return label;
    }
    else {
        throw Error(`Node type ${node.constructor.name} is not implemented`);
    }
}

for (const node of ast.children) {
    let r = explore(node);
    if (r)
        prog.push(r);
}

for (const inst of prog) if (inst.type == 'label') {
    vars[inst.name] = {
        kind: Mnem.imd
    };
}

for (const inst of prog) if (inst.type == 'opcode') {
    for (const arg of inst.args) {
        if (arg.prebuild) {
            arg.prebuild(arg);
            delete arg.prebuild;
        }
    }
}

// console.dir(prog,{depth:10});

const buff = new Buff();

for (const inst of prog) {
    if (inst.type == 'opcode') {
        const i = instructions[inst.name.val];
        if (!i)
            throw evalError(inst.name.loc,'Unimplemented instruction');
        i.build(buff,inst.size,inst.args);
    }
    else if (inst.type == 'label') {
        vars[inst.name].val = buff.getSize();
        for (const ref of refs) if (ref.name == inst.name) { // TODO: Extract in function
            ref.obj.val = vars[inst.name].val;
        }
    }
}

// const result = Buffer.concat([buff.build(),Buffer.from(Array(100).fill(0).map(()=>Math.random()*256))]);
const result = buff.build();

if (outpath) {
    fs.writeFileSync(outpath,result);
}

else {
    console.log('     \x1b[96m00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\x1b[39m \x1b[95mTEXT\x1b[39m');
    for (let off = 0; off < result.length; off += 16)
        console.log(
            '\x1b[94m' +
            off.toString(16).padStart(4,'0') + 
            '\x1b[39m' + 
            (
                ' ' + 
                Array.from(result.subarray(off,off+16))
                    .map(
                        c => c
                            .toString(16)
                            .padStart(2,'0')
                    )
                    .join(' ')
            ).padEnd(49,' ') + 
            Array.from(result.subarray(off,off+16))
                .map(
                    c => {
                        if (c <= 0x22 && c != 0x20)
                            // return String.fromCodePoint(c+0x2400);
                        return `\x1b[90m${String.fromCodePoint(c+0x2400)}\x1b[39m`;
                        if (c >= 0x7D)
                            return '\x1b[90m.\x1b[39m';
                        return String.fromCodePoint(c);
                    }
                ).join('')
        );
}

/*const buff = new Buff();

instructions.mov.build( buff, 0, [
    { kind: Mnem.reg, val: 0x00 },
    { kind: Mnem.imd, val: 0x42 },
] );

const program : Instruction[] = [

];

console.log(buff.build());*/
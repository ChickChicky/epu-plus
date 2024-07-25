import fs from 'node:fs';
import path from 'node:path';
import { inspect } from 'node:util';

export class Source {
    name: string;
    body: string;

    constructor (name:string, body:string) {
        this.name = name;
        this.body = body.replace(/\r\n/g,'\n');
    }

    static fromFile(path:string) : Source {
        const body: string = fs.readFileSync(path,'utf-8');
        return new Source(path,body);
    }
}

export class Loc {
    src: Source;
    col: number;
    row: number;
    len: number;

    constructor (src:Source, col:number, row:number, len:number) {
        this.src = src;
        this.col = col;
        this.row = row;
        this.len = len;
    }

    [inspect.custom]() {
        return `\x1b[36m${this.src.name}:${this.row+1}:${this.col+1}\x1b[39m`;
    }
}

export type TokenKind = 'symbol' | 'identifier' | 'string' | 'eof' | 'newline' | 'number' | 'charlit';

export type TkVal = { token: Token } & (
    { type: 'identifier' } |
    { type: 'string', value: string } |
    { type: 'number', value: number, hint: string|undefined }
);

export class Token {
    kind: TokenKind;
    val: string;
    loc: Loc;
    
    public constructor (col:number, row:number, val:string, src:Source, kind:TokenKind) {
        this.val = val;
        this.loc = new Loc(src,col,row,val.length);
        this.kind = kind;
    }

    public value(): TkVal|undefined {
        if (this.kind == 'string') {
            return {
                type: 'string',
                value: this.val.slice(1,-1),
                token: this,
            };
        }
        if (this.kind == 'number') {
            let value = 0;
            let hint: string|undefined;
            const {isInt,int,intHint} = this.val.match(/^(?<isInt>(?<int>(\d+)|(0x[0-9a-f]+)|(0b[01]+))(?<intHint>(((u|i)?l?l?)|(i8|u8|i16|u16|i32|u32))?))$/i)?.groups??{};
            if (isInt) {
                const {dec,hex,bin} = int.match(/^(?<dec>\d+$)|(0x(?<hex>[0-9a-f]+$))|(0b(?<bin>[01]+$))/i)?.groups ?? {};
                if (dec)
                    value = +dec;
                if (hex)
                    value = Number.parseInt(hex,16);
                if (bin)
                    value = Number.parseInt(bin,2);
                hint = intHint;
            }
            return {
                type: 'number',
                value, hint,
                token: this, 
            };
        }
        if (this.kind == 'identifier') {
            return {
                type: 'identifier',
                token: this,
            };
        }
        if (this.kind == 'charlit') {
            const cp = this.val.codePointAt(1);
            if (!cp)
                throw parseError(this.loc,'Invalid character literal');
            return {
                type: 'number',
                value: cp, 
                hint: undefined, 
                token: this
            };
        }
    }
}

export class Parser {
    root: ProgramNode;
    stack: ParserNode[];
    tokens: Token[];
    i: number;

    constructor (tokens: Token[]) {
        this.root = new ProgramNode();
        this.stack = [this.root];
        this.tokens = tokens;
        this.i = 0;
    }

    next(): void {
        this.stack[this.stack.length-1].feed(this,this.tokens[this.i++]);
    }
}

export class ParserNode {
    feed(parser: Parser, token: Token): void { throw Error('This should not have happened.'); }
}

export class ProgramNode extends ParserNode {
    children: (InstructionNode|LabelNode)[];

    constructor () {
        super();
        this.children = [];
    }

    feed(parser: Parser, token: Token): void {
        if (token.kind == 'identifier') {
            const ops = Object.fromEntries([
                'mov',
                'cmp',
                'int',
                'cal',
                'ret',
                
                'add',
                'sub',
                'mul',
                'div',
                'and',
                'or',
                'xor',
                'shl',
                'shr',

                'jmp',
                'jeq', 'jne',
                'jgt', 'jge',
                'jlt', 'jnl',
            ].map(o=>[o,null]));
            if (token.val in ops) {
                const node = new InstructionNode(token);
                this.children.push(node);
                parser.stack.push(node);
            }
            else if (parser.tokens[parser.i].val == ':') {
                this.children.push(new LabelNode(token));
                parser.i++;
            }
            else {
                throw parseError(token.loc,'Expected keyword or opcode');
            }
        }
        else if (token.kind != 'newline' && token.kind != 'eof') {
            throw parseError(token.loc,'Expected identifier ');
        }
    }
}

export class ArgumentNode extends ParserNode {
    tokens?: Token[];
    value: {value:TkVal,ptr:boolean};

    constructor () {
        super();
        this.tokens = [];
    }

    feed(parser: Parser, token: Token): void {
        if (!this.tokens) return void parser.stack.pop();
        if ((token.kind == 'symbol' && token.val == ',') || token.kind == 'newline' || token.kind == 'eof') {
            if (this.tokens.length == 1) {
                const value = this.tokens[0].value();
                if (!value) {
                    throw parseError(this.tokens[0].loc,'Unexpected token');
                }
                this.value = { value, ptr: false };
            }
            if (this.tokens.length == 2) {
                if (
                    this.tokens[0].kind != 'symbol' ||
                    this.tokens[0].val != '*'
                ) {
                    if (this.tokens[0].kind == 'identifier' || this.tokens[0].kind == 'number') {
                        throw parseError(this.tokens[1].loc,'Unexpected token');
                    }
                    throw parseError(this.tokens[0].loc,'Unexpected token');
                }
                if (
                    this.tokens[1].kind != 'identifier' &&
                    this.tokens[1].kind != 'number'
                ) {
                    throw parseError(this.tokens[1].loc,'Expected identifier');
                }
                const value = this.tokens[1].value();
                if (value) {
                    this.value = { value, ptr: true };
                } else {
                    throw parseError(this.tokens[1].loc,'Could not retreive value');
                }
            }
            delete this.tokens;
            parser.stack.pop();   
            parser.i--;
        } else {
            this.tokens.push(token);
        }
    }
}

export class InstructionNode extends ParserNode {
    name: Token;
    args: ArgumentNode[];
    s_?: number;
    s?: 8 | 16 | 32;

    constructor (name: Token) {
        super();
        this.name = name;
        this.s_ = 0;
        this.args = [];
        delete this.s;
    }

    feed(parser: Parser, token: Token): void {
        if (this.s_ == 0) {
            if (token.val == ':') {
                this.s_ = 1;
                return;
            } else
                delete this.s_;
        } else if (this.s_ == 1) {
            if (token.kind == 'number') {
                let v = token.value();
                if (v && v.type == 'number' && (v.value == 8 || v.value == 16 || v.value == 32))
                    this.s = v.value;
                else
                    throw parseError(token.loc,'Expected valid instruction size');
            }
            else
                throw parseError(token.loc,'Expected instruction size after `:`');
            delete this.s_;
            return;
        }
        if (token.kind == 'newline' || token.kind == 'eof') {
            parser.stack.pop();
        } else {
            if (token.kind != 'symbol' || token.val != ',')
                parser.i--;
            const arg = new ArgumentNode();
            this.args.push(arg);
            parser.stack.push(arg);
        }
    }
}

export class LabelNode extends ParserNode {
    name: Token;

    constructor (name: Token) {
        super();
        this.name = name;
    }

    feed(parser: Parser, token: Token): void { throw Error('That should not have happened.'); }
}

export function parse( source: Source ): ProgramNode {
    let parser = new Parser(tokenize(source));

    while (parser.stack.length && parser.i < parser.tokens.length)
        parser.next();

    return parser.root;
}

export function parseError( loc:Loc, message:string, hint?:string ) {
    const trueline = loc.src.body.split(/\n/g)[loc.row];
    const line = trueline.replace(/^\s+/g,'').replace(/\s+$/g,'');
    const col = loc.col - trueline.length + line.length;
    return SyntaxError(
        `\x1b[31;1m${message}\x1b[39;22m \x1b[90m@ ${loc.src.name}:${loc.row+1}:${loc.col+1}\x1b[39m\n` +
        `    ${line}\n`+
        `    ${' '.repeat(col)}\x1b[33m${'^'.repeat(loc.len)}\x1b[39m` +
        ( hint ? `\nHint: ${hint}` : '' )
    );
}

export function evalError( loc:Loc, message:string, hint?:string ) {
    const trueline = loc.src.body.split(/\n/g)[loc.row];
    const line = trueline.replace(/^\s+/g,'').replace(/\s+$/g,'');
    const col = loc.col - trueline.length + line.length;
    return EvalError(
        `\x1b[31;1m${message}\x1b[39;22m \x1b[90m@ ${loc.src.name}:${loc.row+1}:${loc.col+1}\x1b[39m\n` +
        `    ${line}\n`+
        `    ${' '.repeat(col)}\x1b[33m${'^'.repeat(loc.len)}\x1b[39m` +
        ( hint ? `\nHint: ${hint}` : '' )
    );
}

export function tokenize( source: Source ) : Token[] {
    const tokens: Token[] = [];

    let t: string = '';

    let col: number = 0;
    let row: number = 0;

    function pushtk(tk: string, kind: TokenKind) {
        tokens.push(new Token(col-Math.max(0,tk.length-1),row,tk,source,kind));
    }

    const SYMBOL_REGEX = /^[.,;:!?$+\-/*()[\]{}<>#]+$/i;

    function pushtoken(tt:string='') {
        let k = tt+t;
        if (k.length) {
            if (k.startsWith('"') && k.endsWith('"')) {
                pushtk(k,'string');
            }
            else if (/^(((\d+)|(0x[0-9a-f]+)|(0b[01]+))((((u|i)?)|(i8|u8|i16|u16|i32|u32))?))|(((\d+\.\d+)|(\.\d+)|(\d+\.))(f|d|f32|f64)?)$/i.test(k)) {
                pushtk(k,'number');
            }
            else if (k == '\n') {
                pushtk(k,'newline');
            }
            else if (/^[0-9a-z_]+$/i.test(k)) {
                pushtk(k.toLowerCase(),'identifier');
            }
            else if (SYMBOL_REGEX.test(k)) {
                pushtk(k,'symbol');
            }
            else if (k.startsWith('\'')) {
                pushtk(k,'charlit');
            }
            else {
                throw parseError(new Loc(source,col-k.length+1,row,k.length),'Invalid token');
            }
        }
        t = '';
    }

    for (let i = 0; i < source.body.length; i++) {
        const chr: string = source.body[i];

        if (t[0] == '"') {
            if (chr == '"') {
                pushtoken(chr);
            } else {
                t += chr;
            }
        } else {
            if (SYMBOL_REGEX.test(chr)) {
                col--;
                pushtoken();
                col++;
                pushtoken(chr);
            }
            else if (/\s/.test(chr)) {
                col--;
                pushtoken();
                col++;
            }
            else {
                t += chr;
            }
        }

        if (chr == '\n') {
            pushtoken();
            pushtoken(chr);
            col = 0;
            row++;
        } else {
            col++;
        }
    }

    pushtoken();

    tokens.push(new Token(col,row,'',source,'eof'));

    for (let i = 0; i < tokens.length; i++) {
        const tk = tokens[i];
        if (tk.val == ';') {
            while (tokens[i].kind != 'eof' && tokens[i].kind != 'newline')
                tokens.splice(i,1);
        }
    }

    return tokens;
}
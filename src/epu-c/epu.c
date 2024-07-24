#define string_impl
#define fat_impl

#include "inttypes.h"
#include "memory.h"
#include "string.h"

#include "fat16.h"

#include "ge.h"
#include "epu.h"
#include "data/boot-logos.h"
#include "data/font.h"

//// Macros / Constants ////

// Size to bitmask
#define SZ2MASK(sz) (uint32_t)(0xFFFFFFFFu>>(32u-(1u<<(sz))*8u))

#define STATUS_MASK_STOP    0b00000000000000000000000000011111
#define STATUS_BITS_DONE    0b00000000000000000000000000000001
#define STATUS_BITS_HALT    0b00000000000000000000000000000010
#define STATUS_BITS_READERR 0b00000000000000000000000000000100
#define STATUS_BITS_WRITERR 0b00000000000000000000000000001000
#define STATUS_BITS_ILLINST 0b00000000000000000000000000010000

#define CMP_BITS_EQ 0b00000001
#define CMP_BITS_GT 0b00000010
#define CMP_BITS_LT 0b00000100

#define SCHED_MAX_INSTRUCTIONS 16

#define BOOT_FLOPPY_SIZE 67108864
#define MEM_SEGMENT_SIZE 16777216

#define WIDTH  256
#define HEIGHT 168

#define ARRSIZE(a) (sizeof(a)/sizeof((a)[0]))

//// Types ////

typedef struct epu_ctx_t {
    uint8_t alive;
    uint32_t c; // Execution Count ( for scheduler )
    uint32_t s; // Context Space ( 0: kernel, >0: userspace )

    uint32_t ra; // Register A
    uint32_t rb; // Register B
    uint32_t rc; // Register C
    uint32_t rd; // Register D
    uint32_t re; // Register E
    uint32_t rf; // Register F
    uint32_t rg; // Register G
    uint32_t rh; // Register H

    uint32_t ua; // User Register A
    uint32_t ub; // User Register B
    uint32_t uc; // User Register C
    uint32_t ud; // User Register D
    uint32_t ue; // User Register E
    uint32_t uf; // User Register F
    uint32_t ug; // User Register G
    uint32_t uh; // User Register H

    uint32_t pc; // Program Counter
    uint32_t sp; // Stack Pointer
    uint32_t cp; // Callstack Pointer

    uint32_t flags; // Status Flags

    uint8_t cmp; // Last Comparision Result | TODO: Move into flags

    float fa; // FPU Register A
    float fb; // FPU Register B
    float fc; // FPU Register C
    float fd; // FPU Register D
} epu_ctx;

typedef struct ctx_memory_t {
    uint8_t data[65536]; // RW process RAM
    uint8_t code[65536]; // R  process machine code
    uint8_t ropd[65536]; // R  process data
} ctx_memory;

//// Global Vars ////

int boot_floppy_size;
fat_disk boot_floppy;
int boot_program_size;

color screen[WIDTH*HEIGHT];

unsigned char boot_floppy_data[BOOT_FLOPPY_SIZE];
unsigned char boot_program[MEM_SEGMENT_SIZE];

epu_ctx contexts[256];
ctx_memory proc_memory[256];

uint8_t curr_context = 0;
uint16_t instruction;

uint32_t graphics_palette[256] = {
    [0x00] = 0x000000, [0x08] = 0x808080,
    [0x01] = 0x000080, [0x09] = 0x0000FF,
    [0x02] = 0x008000, [0x0A] = 0x00FF00,
    [0x03] = 0x008080, [0x0B] = 0x00FFFF,
    [0x04] = 0x800000, [0x0C] = 0xFF0000,
    [0x05] = 0x800080, [0x0D] = 0xFF00FF,
    [0x06] = 0x808000, [0x0E] = 0xFFFF00,
    [0x07] = 0xC0C0C0, [0x0F] = 0xFFFFFF,
    
    [0x10] = 0xFF000000,
    [0x11] = 0xFF550000,
    [0x12] = 0xFFAA0000,
    [0x13] = 0xFFFF0000,
    [0x14] = 0xFF005500,
    [0x15] = 0xFF555500,
    [0x16] = 0xFFAA5500,
    [0x17] = 0xFFFF5500,
    [0x18] = 0xFF00AA00,
    [0x19] = 0xFF55AA00,
    [0x1A] = 0xFFAAAA00,
    [0x1B] = 0xFFFFAA00,
    [0x1C] = 0xFF00FF00,
    [0x1D] = 0xFF55FF00,
    [0x1E] = 0xFFAAFF00,
    [0x1F] = 0xFFFFFF00,
    [0x20] = 0xFF000055,
    [0x21] = 0xFF550055,
    [0x22] = 0xFFAA0055,
    [0x23] = 0xFFFF0055,
    [0x24] = 0xFF005555,
    [0x25] = 0xFF555555,
    [0x26] = 0xFFAA5555,
    [0x27] = 0xFFFF5555,
    [0x28] = 0xFF00AA55,
    [0x29] = 0xFF55AA55,
    [0x2A] = 0xFFAAAA55,
    [0x2B] = 0xFFFFAA55,
    [0x2C] = 0xFF00FF55,
    [0x2D] = 0xFF55FF55,
    [0x2E] = 0xFFAAFF55,
    [0x2F] = 0xFFFFFF55,
    [0x30] = 0xFF0000AA,
    [0x31] = 0xFF5500AA,
    [0x32] = 0xFFAA00AA,
    [0x33] = 0xFFFF00AA,
    [0x34] = 0xFF0055AA,
    [0x35] = 0xFF5555AA,
    [0x36] = 0xFFAA55AA,
    [0x37] = 0xFFFF55AA,
    [0x38] = 0xFF00AAAA,
    [0x39] = 0xFF55AAAA,
    [0x3A] = 0xFFAAAAAA,
    [0x3B] = 0xFFFFAAAA,
    [0x3C] = 0xFF00FFAA,
    [0x3D] = 0xFF55FFAA,
    [0x3E] = 0xFFAAFFAA,
    [0x3F] = 0xFFFFFFAA,
    [0x40] = 0xFF0000FF,
    [0x41] = 0xFF5500FF,
    [0x42] = 0xFFAA00FF,
    [0x43] = 0xFFFF00FF,
    [0x44] = 0xFF0055FF,
    [0x45] = 0xFF5555FF,
    [0x46] = 0xFFAA55FF,
    [0x47] = 0xFFFF55FF,
    [0x48] = 0xFF00AAFF,
    [0x49] = 0xFF55AAFF,
    [0x4A] = 0xFFAAAAFF,
    [0x4B] = 0xFFFFAAFF,
    [0x4C] = 0xFF00FFFF,
    [0x4D] = 0xFF55FFFF,
    [0x4E] = 0xFFAAFFFF,
    [0x4F] = 0xFFFFFFFF,
};

uint8_t graphics_font_source[128*8] = SOURCE_FONT_DATA;

typedef struct graphics_char_t {
    uint32_t character;
    uint8_t data[8];
} graphics_char;

graphics_char graphics_chars[1024] = {};

//// Functions ////

void clear_screen() {
    for (int i = 0; i < WIDTH*HEIGHT; i++) {
        screen[i].r =
        screen[i].g =
        screen[i].b = 0;
    }
}

void send_video() {
    ge_screen_set(screen, 0, 0, WIDTH, HEIGHT);
    ge_screen_push();
}

void blit_image(image* img, int ox, int oy) {
    for (int x = 0; x < img->w; x++) {
        for (int y = 0; y < img->h; y++) {
            const int v = img->img[x+y*img->w];
            if (v&1) {
                screen[x+ox+(y+oy)*WIDTH].r = ((img->img[x+y*img->w]>>5)&3)*85;
                screen[x+ox+(y+oy)*WIDTH].g = ((img->img[x+y*img->w]>>3)&3)*85;
                screen[x+ox+(y+oy)*WIDTH].b = ((img->img[x+y*img->w]>>1)&3)*85;
            }
        }
    }
}

int read_data( epu_ctx* ctx, uint32_t* addr, uint32_t size, void* dest ) {
    uint8_t p = *addr >> 24;
    uint8_t s = *addr >> 16;
    if ( p >= 0x00 && p <= 0x0F ) { // Bound RAM
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = proc_memory[ctx->s].data[((*addr)++)&0xFFFF];
            *addr &= 0xFFFF; *addr |= (((uint32_t)p)<<24)|(((uint32_t)s)<<16);
        }
        return 0;
    }
    else if ( p == 0x10 ) { // Bound Code
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = proc_memory[ctx->s].code[((*addr)++)&0xFFFF];
            *addr &= 0xFFFF; *addr |= (((uint32_t)p)<<24)|(((uint32_t)s)<<16);
        }
        return 0;
    }
    else if (p == 0x11 ) { // Bound Data
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = proc_memory[ctx->s].data[((*addr)++)&0xFFFF];
            *addr &= 0xFFFF; *addr |= (((uint32_t)p)<<24)|(((uint32_t)s)<<16);
        }
        return 0;
    }
    else if ( !ctx->s && p >= 0xD0 && p <= 0xDF ) { // Specific RAM
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = proc_memory[s].data[((*addr)++)&0xFFFF];
            *addr &= 0xFFFF; *addr |= (((uint32_t)p)<<24)|(((uint32_t)s)<<16);
        }
        return 0;
    }
    else if ( !ctx->s && p == 0xE0 ) { // Specific Code
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = proc_memory[s].code[((*addr)++)&0xFFFF];
            *addr &= 0xFFFF; *addr |= (((uint32_t)p)<<24)|(((uint32_t)s)<<16);
        }
        return 0;
    }
    else if ( !ctx->s && p == 0xE1 ) { // Specific Data
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = proc_memory[s].data[((*addr)++)&0xFFFF];
            *addr &= 0xFFFF; *addr |= (((uint32_t)p)<<24)|(((uint32_t)s)<<16);
        }
        return 0;
    }
    else if ( !ctx->s && p == 0xFF ) { // Boot Code
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = boot_program[((*addr)++)&0xFFFFFF];
            *addr &= 0xFFFFFF; *addr |= (((uint32_t)p)<<24);
        }
        return 0;
    }
    ctx->flags |= STATUS_BITS_READERR;
    return 1;
}

int peek_data( epu_ctx* ctx, uint32_t addr, uint32_t size, void* dest ) {
    return read_data( ctx, &addr, size, dest );
}

int write_data( epu_ctx* ctx, uint32_t addr, uint32_t size, void* data ) {
    uint8_t p = addr >> 24;
    uint8_t s = addr >> 16;
    if ( p >= 0x00 && p <= 0x0F ) { // Bound RAM
        for (size_t i = 0; i < size; i++) {
            proc_memory[ctx->s].data[addr++] = ((uint8_t*)data)[i];
            addr &= 0xFFFF;
        }
        return 0;
    }
    else if ( !ctx->s && p >= 0xD0 && p <= 0xDF ) { // Specific RAM
        for (size_t i = 0; i < size; i++) {
            proc_memory[s].data[addr++] = ((uint8_t*)data)[i];
            addr &= 0xFFFF;
        }
        return 0;
    }
    ctx->flags |= STATUS_BITS_WRITERR;
    return 1;
}

uint32_t* getCPUReg( epu_ctx* ctx, uint8_t reg ) {    
    if (reg == 0)
        return &ctx->ra;
    if (reg == 1)
        return &ctx->rb;
    if (reg == 2)
        return &ctx->rc;
    if (reg == 3)
        return &ctx->rd;
    if (reg == 4)
        return &ctx->re;
    if (reg == 5)
        return &ctx->rf;
    if (reg == 6)
        return &ctx->rg;
    if (reg == 7)
        return &ctx->rh;

    if (reg == 8)
        return &ctx->ua;
    if (reg == 9)
        return &ctx->ub;
    if (reg == 10)
        return &ctx->uc;
    if (reg == 11)
        return &ctx->ud;
    if (reg == 12)
        return &ctx->ue;
    if (reg == 13)
        return &ctx->uf;
    if (reg == 14)
        return &ctx->ug;
    if (reg == 15)
        return &ctx->uh;

    if (reg == 16)
        return &ctx->pc;
    if (reg == 17)
        return &ctx->sp;
    if (reg == 18)
        return &ctx->cp;

    ctx->flags |= STATUS_BITS_READERR;

    return 0;
}

float* getFPUReg( epu_ctx* ctx, uint8_t reg ) {
    if (reg == 0)
        return &ctx->fa;
    if (reg == 1)
        return &ctx->fb;
    if (reg == 2)
        return &ctx->fc;
    if (reg == 3)
        return &ctx->fd;

    ctx->flags |= STATUS_BITS_READERR;

    return 0;
}

void push( epu_ctx* ctx, uint32_t size, uint32_t* addr, void* data ) {
    write_data(ctx,*addr,size,data);
    *addr += size;
}

void pop( epu_ctx* ctx, uint32_t size, uint32_t* addr, void* data ) {
    *addr -= size;
    peek_data(ctx,*addr,size,data);
}

int init() {
    ge_screen_size(WIDTH,HEIGHT);
    blit_image(&boot_logo,0,0);
    send_video();
    
    /// Loads The Boot Code ///

    epu_load_floppy(0,0,&boot_floppy_size);
    if (boot_floppy_size != BOOT_FLOPPY_SIZE) {
        blit_image(&floppy_logo,21,3);
        send_video();
        return 1;
    } else if (!epu_load_floppy(0,boot_floppy_data,0)) {
        blit_image(&floppy_bad_logo,21,3);
        send_video();
        return 1;
    }

    boot_floppy = (fat_disk){
        .data = boot_floppy_data
    };
    fat_read_boot_sector(&boot_floppy);

    if (fat_boot_file(&boot_floppy,boot_program,&boot_program_size)) {
        blit_image(&floppy_corr_logo,21,3);
        send_video();
        return 1;
    }

    /// Sets Up The Processor ///

    memset(contexts,0,256*sizeof(epu_ctx));
    memset(proc_memory,0,256*sizeof(ctx_memory));

    contexts[0] = (epu_ctx){
        .alive = 1,
        .c = 0,
        .s = 0,

        .ra = 0, .re = 0,
        .rb = 0, .rf = 0,
        .rc = 0, .rg = 0,
        .rd = 0, .rh = 0,

        .ua = 0, .ue = 0,
        .ub = 0, .uf = 0,
        .uc = 0, .ug = 0,
        .ud = 0, .uh = 0,

        .flags = 0,

        .pc = 0xFF000000,
        .sp = 0,
        .cp = 0,

        .fa = 1.f, .fb = 2.f,
        .fc = 0.f, .fd = 0.f,
    };

    // Copy Boot Code into the kernel's code space ( not necessary )
    // memcpy(&proc_memory[0].code,boot_program,(size_t)boot_program_size<sizeof(proc_memory[0].code)?(size_t)boot_program_size:sizeof(proc_memory[0].code));

    curr_context = 0;

    /// Loads The Font ///

    for (size_t i = 0; i < ARRSIZE(graphics_font_source)/8; i++) {
        graphics_chars[i].character = i;
        for (size_t j = 0; j < 8; j++) {
            graphics_chars[i].data[7-j] = graphics_font_source[i*8+j];
        }
    }

    return 0;
}

int loop(size_t steps) { for (size_t it = 0; it < steps; it++) {
    epu_ctx* context = &contexts[curr_context];

    read_data(context,&context->pc,2,&instruction);

    uint8_t opcode = instruction&255;
    uint8_t opflag = instruction>>8;

    // debug((context->pc-2)>>16,(context->pc-2)&0xffff,opcode);

    if (opcode == 0) { // HLT
        context->flags |= STATUS_BITS_HALT;
    }

    else if (opcode == 1) { // ALU
        const uint32_t sz = SZ2MASK(opflag&15);
        const uint32_t tz = 1<<(opflag&15);

        uint8_t op;
        read_data(context,&context->pc,1,&op);

        uint8_t io;
        read_data(context,&context->pc,1,&io);

        uint32_t b = 0;
        if ( opflag&16 ) // Imd
            read_data(context,&context->pc,tz,&b);
        else // Reg
            b = (*getCPUReg(context,io>>4)) & sz;

        uint32_t* a = getCPUReg(context,io&15);

             if (op == 0) *a = ((*a) + b) & sz;
        else if (op == 1) *a = ((*a) - b) & sz;
        else if (op == 2) *a = ((*a) * b) & sz;
        else if (op == 3) *a = ((*a) / b) & sz;
        else if (op == 4) *a = ((*a) & b) & sz;
        else if (op == 5) *a = ((*a) | b) & sz;
        else if (op == 6) *a = ((*a) ^ b) & sz;
    }

    else if (opcode == 2) { // MOV
        uint8_t io;
        read_data(context,&context->pc,1,&io);
        
        const uint8_t i = io>>4;
        const uint8_t o = io&15;
        const uint32_t sz = SZ2MASK(opflag&15);
        const uint32_t tz = 1<<(opflag&15);

        if ( ( (i == 1 || i == 2) && ( o == 1 || o == 2 ) ) || o == 3 ) { // Illegal memory to memory move or illegal destination
            context->flags |= STATUS_BITS_ILLINST;
            goto instuction_end;
        }
        
        uint32_t src = 0;
        uint32_t p = 0;
        uint32_t q = 0;

        if ( i == 0 ) { // Reg
            read_data(context,&context->pc,1,&p);
            src = *getCPUReg(context,p&15) & sz;
        }
        if ( i == 1 ) { // *Reg
            read_data(context,&context->pc,1,&p);
            q = *getCPUReg(context,p&15);
            peek_data(context,q,tz,&src);
        }
        if ( i == 2 ) { // *Imd
            read_data(context,&context->pc,4,&p);
            peek_data(context,p,tz,&src);
        }
        if ( i == 3 ) { // Imd
            read_data(context,&context->pc,tz,&src);
        }

        if ( o == 0 ) { // Reg
            if ( i == 0 || i == 1 )
                p >>= 4;
            else
                read_data(context,&context->pc,1,&p);
            *getCPUReg(context,p&15) = src;
        }
        if ( o == 1 ) { // *Reg
            if ( i == 0 )
                p >>= 4;
            else
                read_data(context,&context->pc,1,&p);
            write_data(context,*getCPUReg(context,p&15),tz,&src);
        }
        if ( o == 2 ) { // *Imd
            read_data(context,&context->pc,4,&p);
            write_data(context,p,tz,&src);
        }
    }
    
    else if (opcode == 3) { // FPU
        uint8_t io;
        read_data(context,&context->pc,1,&io);
        if ( (opflag&7) == 0 ) { // REG -> FPU
            *getFPUReg(context,io&15) = (float)*getCPUReg(context,io>>4);
        }
        else if ( (opflag&7) == 1 ) { // FPU -> REG
            *getCPUReg(context,io&15) = (uint32_t)*getFPUReg(context,io>>4);
        }
        else if ( (opflag&7) == 2 ) { // OP
            uint8_t op = opflag>>3;
            float* a = getFPUReg(context,io&15);
            float* b = getFPUReg(context,io>>4);
            if      (op == 0) *a = *a + *b;
            else if (op == 1) *a = *a - *b;
            else if (op == 2) *a = *a * *b;
            else if (op == 3) *a = *a / *b;
        }
    }

    else if (opcode == 4) { // JMP
        const uint32_t sz = SZ2MASK(opflag&15);
        const uint32_t tz = 1<<(opflag&15);

        uint32_t base = context->pc-2;

        uint8_t src;
        read_data(context,&context->pc,1,&src);

        uint8_t cond;
        read_data(context,&context->pc,1,&cond);
        
        uint32_t addr_off = 0;
        if ( (src&15) == 0 ) { // Reg
            addr_off = *getCPUReg(context,src>>4) & sz;
        }
        if ( (src&15) == 1 ) { // *Reg
            peek_data(context,*getCPUReg(context,src>>4),tz,&addr_off);
        }
        if ( (src&15) == 2 ) { // *Imd
            read_data(context,&context->pc,4,&addr_off);
            peek_data(context,addr_off,tz,&addr_off);
        }
        if ( (src&15) == 3 ) { // Imd
            read_data(context,&context->pc,tz,&addr_off);
        }

        uint32_t addr = (opflag&16 ? 0 : base) + (opflag&32 ? -addr_off : addr_off);

        if ( (cond & 0xF0) == 0x00 ) {
            if ( (context->cmp & (cond&0x0F)) != 0 )
                context->pc = addr;
        }

        else if ( (cond & 0xF0) == 0x10 ) {
            if ( (context->cmp & (cond&0x0F)) == 0 )
                context->pc = addr;
        }

        else
            context->flags |= STATUS_BITS_ILLINST;
    }

    else if (opcode == 5) { // CMP
        uint8_t ab_kind;
        read_data(context,&context->pc,1,&ab_kind);
        
        const uint8_t a_kind = ab_kind>>4;
        const uint8_t b_kind = ab_kind&15;
        const uint32_t sz = SZ2MASK(opflag&15);
        const uint32_t tz = 1<<(opflag&15);

        if ( (a_kind == 1 || a_kind == 2) && ( b_kind == 1 || b_kind == 2 ) ) { // Illegal memory-memory comparision
            context->flags |= STATUS_BITS_ILLINST;
            goto instuction_end;
        }
        
        uint32_t a;
        uint32_t b;
        uint32_t p;
        uint32_t q;

        if ( a_kind == 0 ) { // Reg
            read_data(context,&context->pc,1,&p);
            a = *getCPUReg(context,p&15);
        }
        if ( a_kind == 1 ) { // *Reg
            read_data(context,&context->pc,1,&p);
            q = *getCPUReg(context,p&15);
            peek_data(context,q,tz,&a);
        }
        if ( a_kind == 2 ) { // *Imd
            read_data(context,&context->pc,4,&p);
            peek_data(context,p,tz,&a);
        }
        if ( a_kind == 3 ) { // Imd
            read_data(context,&context->pc,tz,&a);
        }

        if ( b_kind == 0 ) { // Reg
            if ( a_kind == 0 || a_kind == 1 )
                p >>= 4;
            else
                read_data(context,&context->pc,1,&p);
            b = *getCPUReg(context,p&15);
        }
        if ( b_kind == 1 ) { // *Reg
            if ( a_kind == 0 || a_kind == 1 )
                p >>= 4;
            else
                read_data(context,&context->pc,1,&p);
            p = *getCPUReg(context,p&15);
            peek_data(context,p,tz,&b);
        }
        if ( b_kind == 2 ) { // *Imd
            read_data(context,&context->pc,4,&p);
            peek_data(context,p,tz,&b);
        }
        if ( b_kind == 3 ) { // Imd
            read_data(context,&context->pc,tz,&b);
        }

        a &= sz;
        b &= sz;

        if ( !( opflag & 32 ) ) // Clear Compare Status
            contexts->cmp = 0;

        if ( opflag & 16 ) { // Signed Compare
            int32_t sa = (opflag&15)==0 ? *(int8_t*)&a : (opflag&15)==1 ? *(int16_t*)&a : (opflag&15)==2 ? *(int32_t*)&a : 0;
            int32_t sb = (opflag&15)==0 ? *(int8_t*)&b : (opflag&15)==1 ? *(int16_t*)&b : (opflag&15)==2 ? *(int32_t*)&b : 0;
            if ( sa == sb )
                context->cmp |= CMP_BITS_EQ;
            if ( sa < sb )
                context->cmp |= CMP_BITS_LT;
            if ( sa > sb )
                context->cmp |= CMP_BITS_GT;
        } else { // Unsigned Compare
            if ( a == b )
                context->cmp |= CMP_BITS_EQ;
            if ( a < b )
                context->cmp |= CMP_BITS_LT;
            if ( a > b )
                context->cmp |= CMP_BITS_GT;
        }
    }

    else if (opcode == 6) { // INT
        uint32_t interrupt;
        read_data(context,&context->pc,4,&interrupt);

        if (interrupt >= 0xFF00 && interrupt <= 0xFFFF) {
            uint8_t cmd = interrupt&255;
            switch (cmd) {
                case 0: { // Draw Character
                    /*
                        RC : Character
                        RD : 
                            &1 ->
                                0 => RA : X, RB : Y
                                1 => RA : idx
                            &2 ->
                                0 => Grid coordinate
                                2 => Pixel coordinates
                        RE : FG color
                        RF : BG color
                    */
                    uint32_t x = context->ra;
                    uint32_t y = context->rb;
                    if (context->rd&1) {
                        y = (x/(WIDTH/8))*8;
                        x = (x%(WIDTH/8))*8;
                    }
                    else if (context->rd&2) {
                        x *= 8;
                        y *= 8;
                    }

                    const uint32_t c = context->rc;
                    uint32_t fg = graphics_palette[context->re];
                    uint32_t bg = graphics_palette[context->rf];
                    
                    uint8_t* chardata = graphics_chars[0].data;
                    for (size_t i = 0; i < sizeof(graphics_chars); i++) {
                        if (graphics_chars[i].character == c) {
                            chardata = graphics_chars[i].data;
                            break;
                        }
                    }
                    
                    for (size_t dy = 0; dy < 8; dy++) {
                        uint8_t l = chardata[dy];
                        for (size_t dx = 0; dx < 8; dx++) {
                            const uint32_t pcolor = l&(1<<(7-dx)) ? fg : bg;
                            color* pix = &screen[(y+dy)*WIDTH+(x+dx)];
                            pix->r = pcolor&255;
                            pix->g = (pcolor>>8)&255;
                            pix->b = (pcolor>>16)&255;
                        }
                    }
                } break;
                case 1: { // Draw Pixel
                    uint32_t x = context->ra;
                    uint32_t y = context->rb;
                    uint32_t c = graphics_palette[context->rc];

                    color* pix = &screen[y*WIDTH+x];

                    pix->r = c&255;
                    pix->g = (c>>8)&255;
                    pix->b = (c>>16)&255;
                } break;
                case 15: { // Send Video
                    send_video();
                } break;
                case 16: { // Pull Character
                    context->ra = ge_keys_last();
                } break;
                case 17: { // Get pressed
                    keys pressed;
                    ge_keys_pressed(&pressed);
                    context->ra = pressed.a;
                    context->rb = pressed.b;
                } break;
                default:
                    // TODO: illegal instruction?
                    break;
            }
        }
    }

    else if (opcode == 7) { // Call
        const uint32_t sz = SZ2MASK(opflag&15);
        const uint32_t tz = 1<<(opflag&15);

        uint8_t src;
        read_data(context,&context->pc,1,&src);
        
        uint32_t addr = 0;
        if ( (src&15) == 0 ) { // Reg
            addr = *getCPUReg(context,src>>4) & sz;
        }
        if ( (src&15) == 1 ) { // *Reg
            peek_data(context,*getCPUReg(context,src>>4),tz,&addr);
        }
        if ( (src&15) == 2 ) { // *Imd
            read_data(context,&context->pc,4,&addr);
            peek_data(context,addr,tz,&addr);
        }
        if ( (src&15) == 3 ) { // Imd
            read_data(context,&context->pc,tz,&addr);
        }

        debug(tz,src&15);

        push(context,4,&context->cp,&context->pc);
        context->pc = addr;
    }

    else if (opcode == 8) { // Ret
        uint32_t addr;
        pop(context,4,&context->cp,&addr);
        context->pc = addr;
    }

    instuction_end:

    if ( !contexts[0].alive )
        return 1;
    
    if ( ++context->c >= SCHED_MAX_INSTRUCTIONS || context->flags & STATUS_MASK_STOP ) {
        context->c = 0;
        if ( context->flags & STATUS_MASK_STOP ) {
            context->alive = 0;
        }
        if ( !contexts[0].alive )
            return 1;
        while ( !contexts[++curr_context].alive ) ;
    }
} return 0;}
#define string_impl
#define fat_impl

#include "string.h"
#include "inttypes.h"
#include "fat16.h"

/* Sets the size of the screen */
extern void ge_screen_size( int width, int height );
/* Writes data to the screen */
extern void ge_screen_set( void* data, int x, int y, int width, int height );
/* Pushes the back buffer to the screen if is enabled */
extern void ge_screen_push();
/* Returns the size of the screen */
// extern void ge_screen_get_size( int* width, int* height );
/* Returns whether a key is pressed */
extern void ge_key_pressed( int key, int* pressed );
/* Returns the position of the mouse */
extern void ge_mouse_pos( int* x, int* y );

/* Calls a peripheral */
extern int epu_call_peripheral(int address, int a, int b, int c);
/* Loads a specific floppy disk */
extern int epu_load_floppy(int index, void* data, int* size);

typedef struct color_t {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color;


typedef struct image_t {
    unsigned char* img;
    int w;
    int h;
} image;

#define BOOT_FLOPPY_SIZE 1048576
#define MEM_SEGMENT_SIZE 16777216

#define WIDTH  512
#define HEIGHT 342

color screen[WIDTH*HEIGHT];

unsigned char boot_logo_img[] = {
    0b00000001, 0b00001001, 0b00010001, 0b00011001, 0b00000000, 0b01111111, 0b01111111, 0b01111111, 0b00000000, 0b01111111, 0b01111111, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00100001, 0b00101001, 0b00110001, 0b00111001, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01000001, 0b01001001, 0b01010001, 0b01011001, 0b00000000, 0b01111111, 0b01111111, 0b00000000, 0b00000000, 0b01111111, 0b01111111, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01100001, 0b01101001, 0b01110001, 0b01111001, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000011, 0b00001011, 0b00010011, 0b00011011, 0b00000000, 0b01111111, 0b01111111, 0b01111111, 0b00000000, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b01111111, 0b01111111, 0b01111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00100011, 0b00101011, 0b00110011, 0b00111011, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01000011, 0b01001011, 0b01010011, 0b01011011, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01100011, 0b01101011, 0b01110011, 0b01111011, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000101, 0b00001101, 0b00010101, 0b00011101, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00100101, 0b00101101, 0b00110101, 0b00111101, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01000101, 0b01001101, 0b01010101, 0b01011101, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01100101, 0b01101101, 0b01110101, 0b01111101, 0b00000000, 0b01111001, 0b01111001, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b01111001, 0b01111001,
    0b00000111, 0b00001111, 0b00010111, 0b00011111, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b00000000,
    0b00100111, 0b00101111, 0b00110111, 0b00111111, 0b00000000, 0b01111001, 0b01111001, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b01111001, 0b01111001,
    0b01000111, 0b01001111, 0b01010111, 0b01011111, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001,
    0b01100111, 0b01101111, 0b01110111, 0b01111111, 0b00000000, 0b01111001, 0b00000000, 0b00000000, 0b00000000, 0b01111001, 0b01111001, 0b01111001, 0b00000000, 0b01111001, 0b01111001, 0b01111001, 0b00000000, 0b01111001, 0b01111001, 0b01111001,
};
image boot_logo = {
    .img = boot_logo_img,
    .w = 20,
    .h = 16,
};

unsigned char floppy_logo_img[] = {
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000, 
    0b00000001, 0b00001101, 0b00000101, 0b01010101, 0b01010101, 0b00000011, 0b01010101, 0b00001101, 0b00000001, 0b00000001, 
    0b00000001, 0b00001101, 0b00000101, 0b01010101, 0b01010101, 0b00000011, 0b01010101, 0b00001101, 0b00001101, 0b00000001, 
    0b00000001, 0b00001101, 0b00001101, 0b00001101, 0b00001101, 0b00001101, 0b00001101, 0b00001101, 0b00001101, 0b00000001, 
    0b00000001, 0b00001101, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b00001101, 0b00000001, 
    0b00000001, 0b00001101, 0b01111111, 0b01010101, 0b01010101, 0b01010101, 0b01010101, 0b01111111, 0b00001101, 0b00000001, 
    0b00000001, 0b00001101, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b00001101, 0b00000001, 
    0b00000001, 0b00001101, 0b01111111, 0b01010101, 0b01010101, 0b01010101, 0b01010101, 0b01111111, 0b00001101, 0b00000001, 
    0b00000001, 0b00001101, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b01111111, 0b00001101, 0b00000001, 
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 
};
image floppy_logo = {
    .img = floppy_logo_img,
    .w = 10,
    .h = 10,
};

unsigned char floppy_bad_logo_img[] = {
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000, 
    0b00000001, 0b00100011, 0b00100011, 0b01101011, 0b01101011, 0b00000011, 0b01101011, 0b00100011, 0b00000001, 0b00000001, 
    0b00000001, 0b00100011, 0b00100011, 0b01101011, 0b01101011, 0b00000011, 0b01101011, 0b00100011, 0b00100011, 0b00000001, 
    0b00000001, 0b00100011, 0b00100011, 0b00100011, 0b00100011, 0b00100011, 0b00100011, 0b00100011, 0b00100011, 0b00000001, 
    0b00000001, 0b00100011, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b00100011, 0b00000001, 
    0b00000001, 0b00100011, 0b01110101, 0b01101011, 0b01101011, 0b01101011, 0b01101011, 0b01110101, 0b00100011, 0b00000001, 
    0b00000001, 0b00100011, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b00100011, 0b00000001, 
    0b00000001, 0b00100011, 0b01110101, 0b01101011, 0b01101011, 0b01101011, 0b01101011, 0b01110101, 0b00100011, 0b00000001, 
    0b00000001, 0b00100011, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b01110101, 0b00100011, 0b00000001, 
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 
};
image floppy_bad_logo = {
    .img = floppy_bad_logo_img,
    .w = 10,
    .h = 10,
};

unsigned char floppy_corr_logo_img[] = {
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000000, 
    0b00000001, 0b00101011, 0b00101011, 0b01111011, 0b01111011, 0b00000011, 0b01111011, 0b00101011, 0b00000001, 0b00000001, 
    0b00000001, 0b00101011, 0b00101011, 0b01111011, 0b01111011, 0b00000011, 0b01111011, 0b00101011, 0b00101011, 0b00000001, 
    0b00000001, 0b00101011, 0b00101011, 0b00101011, 0b00101011, 0b00101011, 0b00101011, 0b00101011, 0b00101011, 0b00000001, 
    0b00000001, 0b00101011, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b00101011, 0b00000001, 
    0b00000001, 0b00101011, 0b01111101, 0b01111011, 0b01111011, 0b01111011, 0b01111011, 0b01111101, 0b00101011, 0b00000001, 
    0b00000001, 0b00101011, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b00101011, 0b00000001, 
    0b00000001, 0b00101011, 0b01111101, 0b01111011, 0b01111011, 0b01111011, 0b01111011, 0b01111101, 0b00101011, 0b00000001, 
    0b00000001, 0b00101011, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b01111101, 0b00101011, 0b00000001, 
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 
};
image floppy_corr_logo = {
    .img = floppy_corr_logo_img,
    .w = 10,
    .h = 10,
};

unsigned char boot_floppy_data[BOOT_FLOPPY_SIZE];
unsigned char boot_program[MEM_SEGMENT_SIZE];

void clear_screen() {
    for (int i = 0; i < WIDTH*HEIGHT; i++) {
        screen[i].r =
        screen[i].g =
        screen[i].b = 0;
    }
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

void send_video() {
    ge_screen_set(screen, 0, 0, WIDTH, HEIGHT);
    ge_screen_push();
}

int main() {
    ge_screen_size(WIDTH,HEIGHT);
    blit_image(&boot_logo,0,0);
    send_video();
    
    int boot_floppy_size;
    if (!epu_load_floppy(0,boot_floppy_data,&boot_floppy_size)) {
        blit_image(&floppy_logo,21,3);
        send_video();
        return 1;
    } else if (boot_floppy_size != BOOT_FLOPPY_SIZE) {
        blit_image(&floppy_bad_logo,21,3);
        send_video();
        return 1;
    }

    fat_disk floppy;
    floppy.data = boot_floppy_data;
    fat_read_boot_sector(&floppy);

    int boot_program_size;
    if (fat_boot_file(&floppy,boot_program,&boot_program_size)) {
        blit_image(&floppy_corr_logo,21,3);
        send_video();
        return 1;
    }
    
    return 0;
}
/* Sets the size of the screen */
extern void ge_screen_size( int width, int height );
/* Writes data to the screen */
extern void ge_screen_set( void* data, int x, int y, int width, int height );
/* Pushes the back buffer to the screen if is enabled */
extern void ge_screen_push( void );
/* Returns the size of the screen */
extern void ge_screen_get_size( int* width, int* height );
/* Returns the position of the mouse */
extern void ge_mouse_pos( int* x, int* y );
/* Returns a bitmask of the held keys */
extern void ge_keys_pressed( void* pressed );
/* Returns the last character pressed */
extern int ge_keys_last( void );
/* Returns a random 32-bit number */
extern int32_t ge_random( void );

extern void debug();

typedef struct keys_t {
    uint32_t a;
    uint32_t b;
} keys;

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
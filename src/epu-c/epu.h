/* Calls a peripheral */
extern int epu_call_peripheral(int address, int a, int b, int c, int d);
/* Loads a specific floppy disk */
extern int epu_load_floppy(int index, void* data, int* size);

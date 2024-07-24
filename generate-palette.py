# A tool to generate the color palette used by graphic functions

for i in range(64):
    r = hex(int(((i>>0)&3)/3*255))[2:].rjust(2,'0').upper()
    g = hex(int(((i>>2)&3)/3*255))[2:].rjust(2,'0').upper()
    b = hex(int(((i>>4)&3)/3*255))[2:].rjust(2,'0').upper()
    print('[0x%s] = 0xFF%s%s%s,'%(hex(i+16)[2:].upper(),r,g,b))
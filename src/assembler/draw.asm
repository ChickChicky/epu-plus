; PRESS IJKL
mov :8 re, 0x4F
mov :8 rc, 'P
int 0xFF00
mov :8 ra, 1
mov :8 rc, 'R
int 0xFF00
mov :8 ra, 2
mov :8 rc, 'E
int 0xFF00
mov :8 ra, 3
mov :8 rc, 'S
int 0xFF00
mov :8 ra, 4
int 0xFF00
mov :8 rb, 1
mov :8 ra, 0
mov :8 rc, 'I
int 0xFF00
mov :8 ra, 1
mov :8 rc, 'J
int 0xFF00
mov :8 ra, 2
mov :8 rc, 'K
int 0xFF00
mov :8 ra, 3
mov :8 rc, 'L
int 0xFF00
int 0xFF0F

loop:

    ; Handle input
    int 0xFF11
    mov uc, ra
    and ra, 1024
    cmp ra, 0
    mov ra, uc
    jeq skip1
        add ub, 1
    skip1:
    and ra, 256
    cmp ra, 0
    mov ra, uc
    jeq skip2
        sub ub, 1
    skip2:
    and ra, 2048
    cmp ra, 0
    mov ra, uc
    jeq skip3
        add ua, 1
    skip3:
    and ra, 512
    cmp ra, 0
    mov ra, uc
    jeq skip4
        sub ua, 1
    skip4:

    ; Draw Dot
    mov ra, ua
    mov rb, ub
    mov rc, 10
    int 0xFF01
    int 0xFF0F
    
    ; Pause
    mov ra, 0
    pause:
    add ra, 1
    cmp ra, 10000
    jge pause

jmp loop
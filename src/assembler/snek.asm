mov :8 *4, 4  ; N segments

mov :16 *5, 0 ; Apple X
mov :16 *7, 0 ; Apple Y

; Segment #1 (head) init
mov :16 *9,  127 ; X
mov :16 *11, 83  ; Y
mov :8  *13, 0   ; Direction
; Segment #2
mov :16 *14, 127 ; X
mov :16 *16, 84  ; Y
mov :8  *18, 0   ; Direction
; Segment #3
mov :16 *19, 127 ; X
mov :16 *21, 85  ; Y
mov :8  *23, 0   ; Direction
; Segment #4
mov :16 *24, 127 ; X
mov :16 *26, 86  ; Y
mov :8  *28, 0   ; Direction

cal move_apple ; Init apple position
int 0xFF0F     ;

jmp loop

process_snek:
    mov ra, 0  ; Init registers for interrupts
    mov rb, 0  ; 

    mov uf, ue

    mov ua, 0
    process_snek_iter:
        mov ub, ua ; Get the address of the segment
        mul ub, 5  ; -> ub
        add ub, 9  ;

        add ub, 4      ; Extract direction
        mov :8 uc, *ub ; -> uc
        mov :8 *ub, ue ; Save previous direction
        mov ue, uc     ; 
        sub ub, 4      ;

        mov rc, 0       ; Clear previous position
        mov :16 ra, *ub ;
        add ub, 2       ;
        mov :16 rb, *ub ;
        sub ub, 2       ;
        int 0xFF01      ;
        
        mov ug, ra
        mov uh, rb
        
        ; Move according to direction
        cmp uc, 0
        jne move_up
            mov uc, ub
            add uc, 2
            mov :16 ud, *uc
            sub ud, 1
            mov :16 *uc, ud
            jmp move_left
        move_up:
        cmp uc, 1
        jne move_right
            mov :16 ud, *ub
            add ud, 1
            mov :16 *ub, ud
            jmp move_left
        move_right:
        cmp uc, 2
        jne move_down
            mov uc, ub
            add uc, 2
            mov :16 ud, *uc
            add ud, 1
            mov :16 *uc, ud
            jmp move_left
        move_down:
        cmp uc, 3
        jne move_left
            mov :16 ud, *ub
            sub ud, 1
            mov :16 *ub, ud
        move_left:

        mov rc, 10      ; Draw the current position
        mov :16 ra, *ub ;
        add ub, 2       ;
        mov :16 rb, *ub ;
        int 0xFF01      ;

        add ua, 1     ; Advance loop
        cmp :8 ua, *4 ;
    jne process_snek_iter

    mov uc, ug
    mov :16 ug, *5
    cmp :16 ug, *9
    jne eat_apple
    mov :16 ug, *7
    cmp :16 ug, *11
    jne eat_apple
        add ua, 1
        mov :8 *4, ua
        add ub, 3
        mov :16 *ub, uc
        add ub, 2
        mov :16 *ub, uh
        add ub, 2
        mov :8 *ub, ue
        cal move_apple
    eat_apple:

    mov ue, uf

    int 0xFF0F
ret

process_input:
    int 0xFF11
    mov uc, ra
    and ra, 1024
    cmp ra, 0
    mov ra, uc
    jeq skip1
        cmp ue, 0
        jeq skip1
        mov ue, 2
        ret
    skip1:
    and ra, 256
    cmp ra, 0
    mov ra, uc
    jeq skip2
        cmp ue, 2
        jeq skip2
        mov ue, 0
        ret
    skip2:
    and ra, 2048
    cmp ra, 0
    mov ra, uc
    jeq skip3
        cmp ue, 3
        jeq skip3
        mov ue, 1
        ret
    skip3:
    and ra, 512
    cmp ra, 0
    mov ra, uc
    jeq skip4
        cmp ue, 1
        jeq skip4
        mov ue, 3
        ret
    skip4:
ret

move_apple:
    int 0x0100
    mov rb, ra
    int 0x0100
    mod ra, 256
    mod rb, 168
    mov :16 *5, ra
    mov :16 *7, rb
    mov rc, 9
    int 0xFF01
ret

loop:

    cal process_input
    cal process_snek
    
    mov ra, 0
    pause:
    add ra, 1
    cmp ra, 30000
    jge pause

jmp loop
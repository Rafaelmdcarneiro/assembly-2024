.global synth
.text
.equ REV_LEN, 24000
.equ FM_INSTR_SIZE, 48
.equ TICKLEN, 7000
.equ TICKLEN_B, 28000
synth:
pusha
movl %eax,%edi
pushl %edi
pushl %edi
movl $110*44100,%ecx
xorl %eax,%eax
rep
stosl
popl %edi
movl $fm_op_freqs, %eax
movl %eax,instr_addr
pushl %edi
movl $song, %ebp
call seq
popl %edi
movl $fm_op_freqs2, %eax
movl %eax,instr_addr
pushl %edi
movl $song2, %ebp
call seq
popl %edi
synth.skip2:
movl $fm_op_freqs3, %eax
movl %eax,instr_addr
pushl %edi
movl $song3, %ebp
call seq
popl %edi
synth.skip3:
movl $fm_op_freqs4, %eax
movl %eax,instr_addr
pushl %edi
movl $song4, %ebp
call seq
popl %edi
synth.drum:
movl $fm_op_freqs_drum, %eax
movl %eax,instr_addr
pushl %edi
movl $drum_song, %ebp
call seq
popl %edi
synth.do_reverb:
popl %esi
movl %esi,%edi
addl $4*110*44100,%edi
movl $110*44100,%ecx
call reverb
popa
ret
.data
instr_addr:
.byte 0,0,0,0
.text
seq:
xorl %edx,%edx
seq.seq_loop:
testl %edx,%edx
jnz seq.no_advance_order
xorl %eax,%eax
movb (%ebp),%al
incl %ebp
cmpb $-1,%al
je seq.out
imull $16,%eax,%esi
movl $16,%edx
seq.no_advance_order:
xorl %eax,%eax
movb sequences(%esi),%al
movl %eax,current_note
movb %al,%cl
shrb $4,%cl
andb $0x0f,%al
pushl %eax
fildl (%esp)
popl %eax
fidivl osc_semitones
f2xm1
fld1
faddp
movl $1,%ebx
shl %cl,%ebx
pushl %ebx
fimull (%esp)
popl %ebx
fmuls base_freq
movl $TICKLEN, %ecx
pusha
movl instr_addr,%ebp
testl $0xffffffff,(%ebp)
jz seq.is_drum
call fm
jmp seq.not_drum
seq.is_drum:
call drum
seq.not_drum:
popa
fstp %st
addl $TICKLEN_B, %edi
incl %esi
decl %edx
jmp seq.seq_loop
seq.out:
ret
.data
.equ OFF, 0
.equ Cs0, 1
.equ D0, 2
.equ Ds0, 3
.equ E0, 4
.equ F0, 5
.equ Fs0, 6
.equ G0, 7
.equ Gs0, 8
.equ A0, 9
.equ As0, 10
.equ B0, 11
.equ C1, 16
.equ Cs1, 17
.equ D1, 18
.equ Ds1, 19
.equ E1, 20
.equ F1, 21
.equ Fs1, 22
.equ G1, 23
.equ Gs1, 24
.equ A1, 25
.equ As1, 26
.equ B1, 27
.equ C2, 32
.equ Cs2, 33
.equ D2, 34
.equ Ds2, 35
.equ E2, 36
.equ F2, 37
.equ Fs2, 38
.equ G2, 39
.equ Gs2, 40
.equ A2, 41
.equ As2, 42
.equ B2, 43
.equ C3, 48
.equ Cs3, 49
.equ D3, 50
.equ Ds3, 51
.equ E3, 52
.equ F3, 53
.equ Fs3, 54
.equ G3, 55
.equ Gs3, 56
.equ A3, 57
.equ As3, 58
.equ B3, 59
.equ C4, 64
.equ Cs4, 65
.equ D4, 66
.equ Ds4, 67
.equ E4, 68
.equ F4, 69
.equ Fs4, 70
.equ G4, 71
.equ Gs4, 72
.equ A4, 73
.equ As4, 74
.equ B4, 75
.equ C5, 80
.equ Cs5, 81
.equ D5, 82
.equ Ds5, 83
.equ E5, 84
.equ F5, 85
.equ Fs5, 86
.equ G5, 87
.equ Gs5, 88
.equ A5, 89
.equ As5, 90
.equ B5, 91
.equ END_SONG, 255
.equ BD, 1
.equ SD, 2
osc_semitones:
.byte 12,0,0,0
song:
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 1
.byte 1
.byte 1
.byte 14
.byte 1
.byte 12
.byte 13
.byte 1
.byte 6
.byte 6
.byte 6
.byte 15
.byte 6
.byte 6
.byte 6
.byte 15
.byte 1
.byte 1
.byte 1
.byte 14
.byte 1
.byte 12
.byte 13
.byte 1
.byte 6
.byte 6
.byte 15
.byte 6
.byte 15
.byte 6
.byte 6
.byte 13
.byte 1
.byte 5
.byte 255
song2:
.byte 0
.byte 0
.byte 0
.byte 0
.byte 2
.byte 0
.byte 0
.byte 0
.byte 2
.byte 0
.byte 2
.byte 0
.byte 2
.byte 0
.byte 2
.byte 2
.byte 0
.byte 0
.byte 0
.byte 0
.byte 17
.byte 0
.byte 17
.byte 0
.byte 2
.byte 0
.byte 2
.byte 2
.byte 2
.byte 0
.byte 2
.byte 0
.byte 2
.byte 17
.byte 0
.byte 17
.byte 2
.byte 2
.byte 0
.byte 2
.byte 255
song3:
.byte 7
.byte 7
.byte 9
.byte 10
.byte 7
.byte 7
.byte 10
.byte 9
.byte 7
.byte 7
.byte 9
.byte 7
.byte 9
.byte 10
.byte 10
.byte 7
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 7
.byte 7
.byte 10
.byte 9
.byte 7
.byte 10
.byte 8
.byte 7
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 8
.byte 10
.byte 7
.byte 7
.byte 255
song4:
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 11
.byte 19
.byte 19
.byte 19
.byte 19
.byte 19
.byte 19
.byte 19
.byte 11
.byte 11
.byte 11
.byte 255
drum_song:
.byte 0
.byte 0
.byte 0
.byte 0
.byte 4
.byte 4
.byte 4
.byte 4
.byte 4
.byte 4
.byte 4
.byte 4
.byte 4
.byte 3
.byte 4
.byte 4
.byte 0
.byte 0
.byte 0
.byte 0
.byte 4
.byte 4
.byte 3
.byte 4
.byte 4
.byte 4
.byte 4
.byte 3
.byte 4
.byte 4
.byte 3
.byte 4
.byte 4
.byte 3
.byte 4
.byte 4
.byte 4
.byte 4
.byte 4
.byte 3
.byte 255
sequences:
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 32
.byte 32
.byte 0
.byte 26
.byte 0
.byte 32
.byte 32
.byte 0
.byte 0
.byte 0
.byte 0
.byte 58
.byte 64
.byte 0
.byte 70
.byte 65
.byte 0
.byte 0
.byte 0
.byte 0
.byte 80
.byte 0
.byte 0
.byte 81
.byte 0
.byte 0
.byte 80
.byte 0
.byte 0
.byte 75
.byte 0
.byte 0
.byte 1
.byte 0
.byte 0
.byte 0
.byte 2
.byte 0
.byte 1
.byte 0
.byte 0
.byte 1
.byte 0
.byte 1
.byte 2
.byte 0
.byte 1
.byte 0
.byte 1
.byte 0
.byte 0
.byte 0
.byte 2
.byte 0
.byte 1
.byte 0
.byte 1
.byte 0
.byte 0
.byte 0
.byte 2
.byte 0
.byte 1
.byte 0
.byte 32
.byte 32
.byte 0
.byte 26
.byte 0
.byte 32
.byte 32
.byte 0
.byte 32
.byte 16
.byte 16
.byte 2
.byte 0
.byte 0
.byte 0
.byte 0
.byte 37
.byte 37
.byte 0
.byte 35
.byte 0
.byte 37
.byte 37
.byte 0
.byte 0
.byte 53
.byte 0
.byte 64
.byte 0
.byte 67
.byte 69
.byte 70
.byte 16
.byte 16
.byte 16
.byte 0
.byte 16
.byte 0
.byte 0
.byte 0
.byte 0
.byte 16
.byte 16
.byte 0
.byte 16
.byte 0
.byte 0
.byte 0
.byte 21
.byte 21
.byte 21
.byte 0
.byte 21
.byte 0
.byte 0
.byte 0
.byte 0
.byte 21
.byte 21
.byte 0
.byte 21
.byte 0
.byte 0
.byte 0
.byte 16
.byte 16
.byte 16
.byte 0
.byte 16
.byte 0
.byte 0
.byte 0
.byte 0
.byte 16
.byte 16
.byte 0
.byte 16
.byte 17
.byte 0
.byte 17
.byte 10
.byte 10
.byte 10
.byte 0
.byte 10
.byte 0
.byte 0
.byte 0
.byte 0
.byte 10
.byte 10
.byte 0
.byte 10
.byte 17
.byte 0
.byte 17
.byte 0
.byte 80
.byte 0
.byte 0
.byte 80
.byte 0
.byte 0
.byte 80
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 48
.byte 42
.byte 0
.byte 42
.byte 0
.byte 26
.byte 26
.byte 0
.byte 0
.byte 0
.byte 0
.byte 58
.byte 64
.byte 0
.byte 68
.byte 69
.byte 26
.byte 26
.byte 0
.byte 26
.byte 0
.byte 26
.byte 42
.byte 0
.byte 26
.byte 0
.byte 0
.byte 58
.byte 64
.byte 0
.byte 69
.byte 64
.byte 32
.byte 32
.byte 0
.byte 26
.byte 0
.byte 32
.byte 32
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 65
.byte 37
.byte 37
.byte 0
.byte 35
.byte 0
.byte 37
.byte 37
.byte 0
.byte 0
.byte 0
.byte 53
.byte 53
.byte 0
.byte 48
.byte 35
.byte 35
.byte 35
.byte 35
.byte 0
.byte 35
.byte 0
.byte 34
.byte 34
.byte 0
.byte 0
.byte 0
.byte 58
.byte 42
.byte 0
.byte 26
.byte 42
.byte 0
.byte 0
.byte 85
.byte 0
.byte 0
.byte 0
.byte 85
.byte 85
.byte 0
.byte 0
.byte 0
.byte 80
.byte 0
.byte 85
.byte 69
.byte 69
.byte 0
.byte 10
.byte 10
.byte 0
.byte 10
.byte 0
.byte 10
.byte 0
.byte 0
.byte 0
.byte 26
.byte 10
.byte 0
.byte 8
.byte 0
.byte 18
.byte 10
.byte 0
.byte 80
.byte 0
.byte 0
.byte 80
.byte 0
.byte 0
.byte 80
.byte 0
.byte 0
.byte 80
.byte 0
.byte 80
.byte 0
.byte 0
.byte 74
.text
fm:
fm.output_loop:
pushl %ecx
movl $fm_ctrs, %ebx
xorl %eax,%eax
movl $3,%ecx
fm.fm_osc_loop:
pushl %ecx
fld %st
fmuls portamento_a
flds 12(%ebx,%eax)
fmuls portamento_b
faddp
movl current_note,%ecx
cmpl $0,%ecx
je fm.no_porta
fsts 12(%ebx,%eax)
fld1
fsubs fm_volume
fmuls fm_attack
fadds fm_volume
fstps fm_volume
jmp fm.yes_porta
fm.no_porta:
flds fm_volume
fmuls fm_decay
fadds small
fstps fm_volume
fm.yes_porta:
fldz
xorl %esi,%esi
movl $3,%ecx
fm.fm_mod_sum_loop:
pushl %ecx
pushl %eax
flds 24(%esi,%ebx)
imull $3,%eax,%eax
addl %esi,%eax
fmuls 12(%ebp,%eax)
faddp %st(1)
popl %eax
addl $4,%esi
popl %ecx
loop fm.fm_mod_sum_loop
flds (%ebx,%eax)
flds (%ebp,%eax)
fmul %st(3)
faddp
faddp
fldpi
fadd %st
fxch %st(1)
fprem1
fsts (%ebx,%eax)
fsin
fm.no_wrap:
fstps fm_outs(%eax)
addl $4,%eax
fstp %st
fstp %st
popl %ecx
decl %ecx
jnz fm.fm_osc_loop
flds last_osc
fmuls 48(%ebp)
fmuls fm_volume
fadds (%edi)
fstps (%edi)
addl $4,%edi
popl %ecx
decl %ecx
jnz fm.output_loop
ret
drum:
xorl %ebx,%ebx
cmpb $1,%al
je drum.bd
cmpb $2,%al
je drum.sd
addl $TICKLEN_B, %edi
ret
drum.bd:
movl $2200,%ecx
movl $0x00fffff,%esi
flds drum_vol
jmp drum.bd_loop
drum.sd:
movl $1200,%ecx
movl $0x08ffff,%esi
flds drum_vol_2
drum.bd_loop:
addl %ecx,%ebx
andl %esi,%ebx
pushl %ebx
fildl (%esp)
popl %ebx
fdivs drum_magic
fsin
pushl %ecx
fimull (%esp)
popl %ecx
fmul %st(1)
flds (%edi)
faddp
fstps (%edi)
addl $4,%edi
loop drum.bd_loop
fstp %st
ret
.data
drum_magic:
.byte 0,249,34,71
drum_vol:
.byte 0,183,81,57
drum_vol_2:
.byte 0,183,209,57
small:
.byte 0,197,167,54
.text
reverb:
reverb.reverb_loop:
pushl %ecx
flds (%esi)
fmuls rev_filter_a
flds rev_filter_state
fmuls rev_filter_b
faddp
fsts rev_filter_state
movl rev_buf_pos,%ebx
fadds rev_buf(%ebx)
fmuls rev_x
reverb.b:
fstps rev_buf(%ebx)
subl $4,%ebx
jns reverb.no_wrap
addl $REV_LEN, %ebx
reverb.no_wrap:
movl %ebx,rev_buf_pos
movl $2,%ecx
reverb.reverb_ch_loop:
pushl %ecx
fldz
movl $24,%ecx
reverb.reverb_accum_loop:
imull $128,%ecx,%eax
addl %eax,%ebx
subl $4777*4,%ebx
jns reverb.no_wrap2
addl $REV_LEN, %ebx
reverb.no_wrap2:
fadds rev_buf(%ebx)
fchs
loop reverb.reverb_accum_loop
fmuls rev_amp
fadds (%esi)
fsts (%edi)
addl $4,%edi
fstp %st
popl %ecx
loop reverb.reverb_ch_loop
addl $4,%esi
popl %ecx
decl %ecx
jnz reverb.reverb_loop
ret
.data
fm_decay:
.byte 0,190,127,63
fm_attack:
.byte 0,215,163,60
portamento_a:
.byte 0,73,29,58
portamento_b:
.byte 0,216,127,63
rev_amp:
.byte 0,153,25,63
base_freq:
.byte 0,251,46,59
rev_x:
.byte 0,204,204,62
fm_instruments:
fm_op_freqs:
.byte 0,0,128,63
.byte 0,0,128,63
.byte 0,0,0,64
fm_mod_amps:
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,204,204,61
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,204,76,62
.byte 0,204,76,62
.byte 0,204,204,61
volume:
.byte 0,20,46,62
fm_op_freqs2:
.byte 0,0,0,64
.byte 0,0,160,64
.byte 0,0,0,64
fm_mod_amps2:
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,153,25,62
.byte 0,204,76,61
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,153,25,62
.byte 0,0,0,0
volume2:
.byte 0,194,245,61
fm_op_freqs3:
.byte 0,0,128,64
.byte 0,0,64,64
.byte 0,0,128,63
fm_mod_amps3:
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,215,35,60
.byte 0,204,76,61
.byte 0,0,0,0
volume3:
.byte 0,215,35,62
fm_op_freqs4:
.byte 0,71,129,63
.byte 0,225,250,62
.byte 0,0,0,63
fm_mod_amps4:
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,194,245,61
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,194,245,61
.byte 0,153,25,62
.byte 0,153,153,62
volume4:
.byte 0,81,56,62
fm_op_freqs_drum:
.byte 0,0,0,0
.byte 0,0,0,0
.byte 0,0,0,0
rev_filter_a:
.byte 0,204,204,61
rev_filter_b:
.byte 0,102,102,63
.bss
current_note:
.byte 0,0,0,0
fm_volume:
.space 1*4
fm_ctrs:
.space 3*4
osc_freqs:
.space 3*4
fm_outs:
.space 2*4
last_osc:
.space 1*4
rev_filter_state:
.space 1*4
rev_buf_pos:
.space 1*4
.space 1*4
rev_buf:
.space 30000*4


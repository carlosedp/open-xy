/* 11a03f20: engine-group modulation CALL.A 8d5182d4. The original C
 * arguments are project, track, slot and amount. R4 still holds the full
 * selector >> 8. Pass it as a fifth argument; all callee-saved registers
 * retain their stock values. Other destination groups keep their own calls.
 */
.section .text
.align 2
.extern sdk_lfo_modulate
.global sdk_lfo_audio
.type sdk_lfo_audio, STT_FUNC
sdk_lfo_audio:
    LINK 0;
    SP += -20;
    R3 = [FP + 20];
    [SP + 12] = R3;
    [SP + 16] = R4;
    CALL sdk_lfo_modulate;
    SP += 20;
    UNLINK;
    RTS;
.size sdk_lfo_audio, .-sdk_lfo_audio

.syntax unified
.arch armv7-a
.fpu vfpv3-d16
.thumb
.text

@ Runtime-free Idriç numerical leaves for Android armeabi-v7a.
@ C ABI: softfp at the boundary, hardware Float32 internally.

@ Selected Idriç exports:
@ NativeARMPositive.mix_frame_samples -> mix_frame_samples

@ Validated Idriç leaf IR for mix_frame_samples:
@   function mix_frame_samples
@   arguments: [v0:Float32@0, v1:Float32@1]
@     v2:Float32@2 = add v0:Float32@0 v1:Float32@1
@   return v2:Float32@2
@   frame-bytes: 16

        .p2align 2
        .global mix_frame_samples
        .type mix_frame_samples, %function
        .thumb_func
mix_frame_samples:
        sub.w   sp, sp, #16
        str     r0, [sp, #0]
        str     r1, [sp, #4]
        vldr    s0, [sp, #0]
        vldr    s1, [sp, #4]
        vadd.f32 s0, s0, s1
        vstr    s0, [sp, #8]
        ldr     r0, [sp, #8]
        add.w   sp, sp, #16
        bx      lr
        .size mix_frame_samples, .-mix_frame_samples

.section .note.GNU-stack,"",%progbits

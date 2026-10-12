/* harvested: matched m2c drafts; pending style pass */
#include "common.h"

typedef struct FlagDesc {
    u32 mask;
    u32 value;
    char *string;
} FlagDesc;

/// Inspect named register flags for the disabled diagnostic report.
/// \param value Register value to inspect.
/// \param s Diagnostic label; unused in this build.
/// \param desc Flag descriptions terminated by an entry with a zero mask.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800422A0 (0x800422A0).
void faultInspectRegisterFlags(u32 value, char *s, FlagDesc *desc) {
    s32 first = 1;

    while (desc->mask != 0) {
        if ((value & desc->mask) == desc->value) {
            if (first) {
                first = 0;
            }
        }
        desc++;
    }
}

s32 mathIsNaNFloat(f32);           /* extern */
s32 mathIsInfiniteFloat(f32);      /* extern */
s32 mathIsSubnormalFloat(f32);     /* extern */

/// Inspect a single-precision value for the disabled fault report.
/// \param labelAddress RAM address of the diagnostic label; unused in this build.
/// \param value Floating-point value to inspect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800422E8 (0x800422E8).
void faultInspectFloat(s32 labelAddress, f32 value) {
    if ((mathIsNaNFloat(value) == 0) && (mathIsInfiniteFloat(value) == 0)) {
        mathIsSubnormalFloat(value);
    }
}

s32 mathIsNaNDouble(f64);           /* extern */
s32 mathIsInfiniteDouble(f64);      /* extern */
s32 mathIsSubnormalDouble(f64);     /* extern */

/// Inspect a double-precision value for the disabled fault report.
/// \param labelAddress RAM address of the diagnostic label; unused in this build.
/// \param value Floating-point value to inspect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80042330 (0x80042330).
void faultInspectDouble(s32 labelAddress, f64 value) {
    if ((mathIsNaNDouble(value) == 0) && (mathIsInfiniteDouble(value) == 0)) {
        mathIsSubnormalDouble(value);
    }
}

void debugTextDrawFramebuffers(char *, s32, s32);
/// Flag descriptions for the saved exception cause register.
/// \note Original identifier: \c D_8009D550 (0x8009D550).
extern FlagDesc gFaultCauseFlagDescriptions[];
/// Flag descriptions for the saved processor status register.
/// \note Original identifier: \c D_8009D6A0 (0x8009D6A0).
extern FlagDesc gFaultStatusFlagDescriptions[];
/// Flag descriptions for the saved floating-point control/status register.
/// \note Original identifier: \c D_8009D814 (0x8009D814).
extern FlagDesc gFaultFpControlStatusFlagDescriptions[];
/// Diagnostic label for the saved exception cause register.
/// \note Original identifier: \c D_800A78E4 (0x800A78E4).
extern char gFaultCauseLabel;
/// Diagnostic label for the saved processor status register.
/// \note Original identifier: \c D_800A78EC (0x800A78EC).
extern char gFaultStatusLabel;
/// Diagnostic label for the saved floating-point control/status register.
/// \note Original identifier: \c D_800A7AB4 (0x800A7AB4).
extern char gFaultFpControlStatusLabel;
/// Diagnostic label for floating-point register 0 interpreted as double precision.
/// \note Original identifier: \c D_800A7ABC (0x800A7ABC).
extern char gFaultDoubleRegister0Label;
/// Diagnostic label for floating-point register 2 interpreted as double precision.
/// \note Original identifier: \c D_800A7AC4 (0x800A7AC4).
extern char gFaultDoubleRegister2Label;
/// Diagnostic label for floating-point register 4 interpreted as double precision.
/// \note Original identifier: \c D_800A7AC8 (0x800A7AC8).
extern char gFaultDoubleRegister4Label;
/// Diagnostic label for floating-point register 6 interpreted as double precision.
/// \note Original identifier: \c D_800A7AD0 (0x800A7AD0).
extern char gFaultDoubleRegister6Label;
/// Diagnostic label for floating-point register 8 interpreted as double precision.
/// \note Original identifier: \c D_800A7AD4 (0x800A7AD4).
extern char gFaultDoubleRegister8Label;
/// Diagnostic label for floating-point register 10 interpreted as double precision.
/// \note Original identifier: \c D_800A7ADC (0x800A7ADC).
extern char gFaultDoubleRegister10Label;
/// Diagnostic label for floating-point register 12 interpreted as double precision.
/// \note Original identifier: \c D_800A7AE0 (0x800A7AE0).
extern char gFaultDoubleRegister12Label;
/// Diagnostic label for floating-point register 14 interpreted as double precision.
/// \note Original identifier: \c D_800A7AE8 (0x800A7AE8).
extern char gFaultDoubleRegister14Label;
/// Diagnostic label for floating-point register 16 interpreted as double precision.
/// \note Original identifier: \c D_800A7AEC (0x800A7AEC).
extern char gFaultDoubleRegister16Label;
/// Diagnostic label for floating-point register 18 interpreted as double precision.
/// \note Original identifier: \c D_800A7AF4 (0x800A7AF4).
extern char gFaultDoubleRegister18Label;
/// Diagnostic label for floating-point register 20 interpreted as double precision.
/// \note Original identifier: \c D_800A7AF8 (0x800A7AF8).
extern char gFaultDoubleRegister20Label;
/// Diagnostic label for floating-point register 22 interpreted as double precision.
/// \note Original identifier: \c D_800A7B00 (0x800A7B00).
extern char gFaultDoubleRegister22Label;
/// Diagnostic label for floating-point register 24 interpreted as double precision.
/// \note Original identifier: \c D_800A7B04 (0x800A7B04).
extern char gFaultDoubleRegister24Label;
/// Diagnostic label for floating-point register 26 interpreted as double precision.
/// \note Original identifier: \c D_800A7B0C (0x800A7B0C).
extern char gFaultDoubleRegister26Label;
/// Diagnostic label for floating-point register 28 interpreted as double precision.
/// \note Original identifier: \c D_800A7B10 (0x800A7B10).
extern char gFaultDoubleRegister28Label;
/// Diagnostic label for floating-point register 30 interpreted as double precision.
/// \note Original identifier: \c D_800A7B18 (0x800A7B18).
extern char gFaultDoubleRegister30Label;
/// Diagnostic label for floating-point register 0 interpreted as single precision.
/// \note Original identifier: \c D_800A7B20 (0x800A7B20).
extern char gFaultFloatRegister0Label;
/// Diagnostic label for floating-point register 2 interpreted as single precision.
/// \note Original identifier: \c D_800A7B28 (0x800A7B28).
extern char gFaultFloatRegister2Label;
/// Diagnostic label for floating-point register 4 interpreted as single precision.
/// \note Original identifier: \c D_800A7B2C (0x800A7B2C).
extern char gFaultFloatRegister4Label;
/// Diagnostic label for floating-point register 6 interpreted as single precision.
/// \note Original identifier: \c D_800A7B30 (0x800A7B30).
extern char gFaultFloatRegister6Label;
/// Diagnostic label for floating-point register 8 interpreted as single precision.
/// \note Original identifier: \c D_800A7B34 (0x800A7B34).
extern char gFaultFloatRegister8Label;
/// Diagnostic label for floating-point register 10 interpreted as single precision.
/// \note Original identifier: \c D_800A7B3C (0x800A7B3C).
extern char gFaultFloatRegister10Label;
/// Diagnostic label for floating-point register 12 interpreted as single precision.
/// \note Original identifier: \c D_800A7B40 (0x800A7B40).
extern char gFaultFloatRegister12Label;
/// Diagnostic label for floating-point register 14 interpreted as single precision.
/// \note Original identifier: \c D_800A7B44 (0x800A7B44).
extern char gFaultFloatRegister14Label;
/// Diagnostic label for floating-point register 16 interpreted as single precision.
/// \note Original identifier: \c D_800A7B48 (0x800A7B48).
extern char gFaultFloatRegister16Label;
/// Diagnostic label for floating-point register 18 interpreted as single precision.
/// \note Original identifier: \c D_800A7B50 (0x800A7B50).
extern char gFaultFloatRegister18Label;
/// Diagnostic label for floating-point register 20 interpreted as single precision.
/// \note Original identifier: \c D_800A7B54 (0x800A7B54).
extern char gFaultFloatRegister20Label;
/// Diagnostic label for floating-point register 22 interpreted as single precision.
/// \note Original identifier: \c D_800A7B58 (0x800A7B58).
extern char gFaultFloatRegister22Label;
/// Diagnostic label for floating-point register 24 interpreted as single precision.
/// \note Original identifier: \c D_800A7B5C (0x800A7B5C).
extern char gFaultFloatRegister24Label;
/// Diagnostic label for floating-point register 26 interpreted as single precision.
/// \note Original identifier: \c D_800A7B64 (0x800A7B64).
extern char gFaultFloatRegister26Label;
/// Diagnostic label for floating-point register 28 interpreted as single precision.
/// \note Original identifier: \c D_800A7B68 (0x800A7B68).
extern char gFaultFloatRegister28Label;
/// Diagnostic label for floating-point register 30 interpreted as single precision.
/// \note Original identifier: \c D_800A7B6C (0x800A7B6C).
extern char gFaultFloatRegister30Label;
/// Heading displayed alongside the faulted thread summary.
/// \note Original identifier: \c D_800A7B74 (0x800A7B74).
extern char gFaultThreadSummaryLabel;
/// Decimal format for the faulted thread identifier.
/// \note Original identifier: \c D_800A7B78 (0x800A7B78).
extern char gFaultThreadIdFormat;
/// Hexadecimal format for the saved program counter.
/// \note Original identifier: \c D_800A7B7C (0x800A7B7C).
extern char gFaultProgramCounterFormat;
/// Hexadecimal format for the saved return address.
/// \note Original identifier: \c D_800A7B84 (0x800A7B84).
extern char gFaultReturnAddressFormat;

/// Display identifying information for a faulted thread.
/// \param thread Thread whose saved execution state is to be reported.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8004237C (0x8004237C).
void faultDrawThreadSummary(OSThread *thread) {
    char buffer[16];
    __OSThreadContext *context;
    context = &thread->context;
    faultInspectRegisterFlags(context->cause, &gFaultCauseLabel, gFaultCauseFlagDescriptions);
    faultInspectRegisterFlags(context->sr, &gFaultStatusLabel, gFaultStatusFlagDescriptions);
    faultInspectRegisterFlags(context->fpcsr, &gFaultFpControlStatusLabel, gFaultFpControlStatusFlagDescriptions);
    faultInspectDouble((s32)&gFaultDoubleRegister0Label, context->fp0.d);
    faultInspectDouble((s32)&gFaultDoubleRegister2Label, context->fp2.d);
    faultInspectDouble((s32)&gFaultDoubleRegister4Label, context->fp4.d);
    faultInspectDouble((s32)&gFaultDoubleRegister6Label, context->fp6.d);
    faultInspectDouble((s32)&gFaultDoubleRegister8Label, context->fp8.d);
    faultInspectDouble((s32)&gFaultDoubleRegister10Label, context->fp10.d);
    faultInspectDouble((s32)&gFaultDoubleRegister12Label, context->fp12.d);
    faultInspectDouble((s32)&gFaultDoubleRegister14Label, context->fp14.d);
    faultInspectDouble((s32)&gFaultDoubleRegister16Label, context->fp16.d);
    faultInspectDouble((s32)&gFaultDoubleRegister18Label, context->fp18.d);
    faultInspectDouble((s32)&gFaultDoubleRegister20Label, context->fp20.d);
    faultInspectDouble((s32)&gFaultDoubleRegister22Label, context->fp22.d);
    faultInspectDouble((s32)&gFaultDoubleRegister24Label, context->fp24.d);
    faultInspectDouble((s32)&gFaultDoubleRegister26Label, context->fp26.d);
    faultInspectDouble((s32)&gFaultDoubleRegister28Label, context->fp28.d);
    faultInspectDouble((s32)&gFaultDoubleRegister30Label, context->fp30.d);
    faultInspectFloat((s32)&gFaultFloatRegister0Label, context->fp0.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister2Label, context->fp2.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister4Label, context->fp4.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister6Label, context->fp6.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister8Label, context->fp8.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister10Label, context->fp10.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister12Label, context->fp12.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister14Label, context->fp14.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister16Label, context->fp16.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister18Label, context->fp18.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister20Label, context->fp20.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister22Label, context->fp22.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister24Label, context->fp24.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister26Label, context->fp26.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister28Label, context->fp28.f.f_even);
    faultInspectFloat((s32)&gFaultFloatRegister30Label, context->fp30.f.f_even);
    debugTextDrawFramebuffers(&gFaultThreadSummaryLabel, 0x28, 9);
    sprintf(buffer, &gFaultThreadIdFormat, thread->id);
    debugTextDrawFramebuffers(buffer, 0x48, 9);
    sprintf(buffer, &gFaultProgramCounterFormat, context->pc);
    debugTextDrawFramebuffers(buffer, 0x68, 9);
    sprintf(buffer, &gFaultReturnAddressFormat, (u32)context->ra);
    debugTextDrawFramebuffers(buffer, 0xB8, 9);
}

extern OSThread *__osGetCurrFaultedThread(void);
/// Pending diagnostic text consumed and cleared by the fault handler.
/// \note Original identifier: \c D_8009D9AC (0x8009D9AC).
extern char *gFaultPendingDiagnosticText;
/// Integer displayed with pending fault diagnostic text.
/// \note Original identifier: \c D_8009D9B0 (0x8009D9B0).
extern s32 gFaultPendingDiagnosticValue;
/// Decimal format for the pending fault diagnostic value.
/// \note Original identifier: \c D_800A7BBC (0x800A7BBC).
extern char gFaultDiagnosticValueFormat;
/// Queue receiving fault notifications; its address also marks the fault-thread stack end.
/// \note Original identifier: \c D_800F8ED0 (0x800F8ED0).
extern OSMesgQueue gFaultEventQueue;

/// Handle fault notifications and display the available diagnostic summary.
/// \param unused Unused thread-entry argument.
/// \returns Does not return.
/// \note Original decompilation identifier: \c func_800426B4 (0x800426B4).
void faultHandlerThread(void *unused) {
    OSMesg message;
    char buffer[12];
    OSThread *faulted;

    osSetEventMesg(OS_EVENT_FAULT, &gFaultEventQueue, (OSMesg)1);
    for (;;) {
        osRecvMesg(&gFaultEventQueue, &message, OS_MESG_BLOCK);
        /* A compiled-out debug check on the received message: its body is
         * stripped from this build, but its loop-invariant constant still
         * reserved the saved s4. The constant is not recoverable from the ROM;
         * any value needing a register reproduces the allocation. */
        if ((s32)message == 0x10) {
        }
        if (gFaultPendingDiagnosticText != NULL) {
            debugTextDrawFramebuffers(gFaultPendingDiagnosticText, 0x28, 9);
            sprintf(buffer, &gFaultDiagnosticValueFormat, gFaultPendingDiagnosticValue);
            debugTextDrawFramebuffers(buffer, 0xC8, 9);
            gFaultPendingDiagnosticText = NULL;
        } else {
            faulted = __osGetCurrFaultedThread();
            if (faulted != NULL) {
                faultDrawThreadSummary(faulted);
            }
        }
        osViBlack(FALSE);
    }
}

/// Thread record for the fault-reporting handler.
/// \note Original identifier: \c D_800F8B20 (0x800F8B20).
extern OSThread gFaultHandlerThread;
/// Single-message backing storage for the fault notification queue.
/// \note Original identifier: \c D_800F8EE8 (0x800F8EE8).
extern OSMesg gFaultEventMessageStorage;

/// Start the fault-reporting thread.
/// \param threadId Identifier assigned to the fault-reporting thread.
/// \param priority Scheduling priority assigned to the fault-reporting thread.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800427C8 (0x800427C8).
void faultStartHandler(s32 threadId, s32 priority) {
    osCreateMesgQueue(&gFaultEventQueue, &gFaultEventMessageStorage, 1);
    osCreateThread(&gFaultHandlerThread, threadId, faultHandlerThread, NULL, &gFaultEventQueue, priority);
    osStartThread(&gFaultHandlerThread);
}

// Type definitions for unsigned integers and size types
typedef unsigned char uint8_t; ///< 8-bit unsigned integer
typedef unsigned int uint32_t; ///< 32-bit unsigned integer
typedef uint32_t size_t;       ///< Size type (32-bit)

#include "kernel.h" ///< Kernel-related definitions and functions
#include "common.h" ///< Common utilities and macros

// External symbols for memory and stack boundaries defined in the linker script
// kernel.ld
extern char __bss[];       ///< Start of the BSS section
extern char __bss_end[];   ///< End of the BSS section
extern char __stack_top[]; ///< Top of the stack

/**
 * @brief Performs a Supervisor Binary Interface (SBI) call.
 *
 * @param arg0 Argument 0 to the SBI call.
 * @param arg1 Argument 1 to the SBI call.
 * @param arg2 Argument 2 to the SBI call.
 * @param arg3 Argument 3 to the SBI call.
 * @param arg4 Argument 4 to the SBI call.
 * @param arg5 Argument 5 to the SBI call.
 * @param fid Function ID for the SBI call.
 * @param eid Extension ID for the SBI call.
 * @return struct sbiret Structure containing the result of the SBI call:
 *         - `error`: Error code from the SBI call.
 *         - `value`: Value returned by the SBI call.
 */
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
  // Load arguments into registers according to RISC-V calling convention
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  // Perform the SBI call using the ecall instruction
  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         "r"(a6), "r"(a7)
                       : "memory");

  return (struct sbiret){.error = a0, .value = a1};
}

/**
 * Outputs a character to the console.
 *
 * @param ch - The character to output.
 */
void putchar(char ch) {
  // SBI call to the console putchar function (extension ID = 1)
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

/**
 * Initial entry point for the kernel.
 *
 * This function sets the stack pointer and jumps to the kernel main function.
 * It is placed in the `.text.boot` section and does not use compiler-generated
 * prologues or epilogues.
 */
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__("mv sp, %[stack_top]\n" // Set the stack pointer
                       "j kernel_main\n"       // Jump to kernel_main
                       :
                       : [stack_top] "r"(__stack_top)); // Input: stack top
}

/**
 * Handles unexpected traps or exceptions.
 *
 * @param f The trap frame containing the CPU state at the time of the trap.
 */
void handle_trap(struct trap_frame *f) {
  // Read trap-related control and status registers
  uint32_t scause = READ_CSR(scause); ///< Trap cause
  uint32_t stval = READ_CSR(stval);   ///< Faulting address or value
  uint32_t user_pc =
      READ_CSR(sepc); ///< Program counter at the time of the trap

  // Print panic message and trap diagnostics
  PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
        user_pc);
}

/**
 * The kernel trap entry point.
 *
 * Saves the CPU state, calls the trap handler, and restores the state before
 * returning.
 */
__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
  __asm__ __volatile__(
      "csrw sscratch, sp\n"    // Save the current stack pointer
      "addi sp, sp, -4 * 31\n" // Allocate stack space for 31 registers
      // Save registers on the stack
      "sw ra,  4 * 0(sp)\n"
      "sw gp,  4 * 1(sp)\n"
      "sw tp,  4 * 2(sp)\n"
      "sw t0,  4 * 3(sp)\n"
      "sw t1,  4 * 4(sp)\n"
      "sw t2,  4 * 5(sp)\n"
      "sw t3,  4 * 6(sp)\n"
      "sw t4,  4 * 7(sp)\n"
      "sw t5,  4 * 8(sp)\n"
      "sw t6,  4 * 9(sp)\n"
      "sw a0,  4 * 10(sp)\n"
      "sw a1,  4 * 11(sp)\n"
      "sw a2,  4 * 12(sp)\n"
      "sw a3,  4 * 13(sp)\n"
      "sw a4,  4 * 14(sp)\n"
      "sw a5,  4 * 15(sp)\n"
      "sw a6,  4 * 16(sp)\n"
      "sw a7,  4 * 17(sp)\n"
      "sw s0,  4 * 18(sp)\n"
      "sw s1,  4 * 19(sp)\n"
      "sw s2,  4 * 20(sp)\n"
      "sw s3,  4 * 21(sp)\n"
      "sw s4,  4 * 22(sp)\n"
      "sw s5,  4 * 23(sp)\n"
      "sw s6,  4 * 24(sp)\n"
      "sw s7,  4 * 25(sp)\n"
      "sw s8,  4 * 26(sp)\n"
      "sw s9,  4 * 27(sp)\n"
      "sw s10, 4 * 28(sp)\n"
      "sw s11, 4 * 29(sp)\n"

      // Save the original stack pointer
      "csrr a0, sscratch\n"
      "sw a0, 4 * 30(sp)\n"

      // Pass the stack pointer to the trap handler
      "mv a0, sp\n"
      "call handle_trap\n"

      // Restore registers from the stack
      "lw ra,  4 * 0(sp)\n"
      "lw gp,  4 * 1(sp)\n"
      "lw tp,  4 * 2(sp)\n"
      "lw t0,  4 * 3(sp)\n"
      "lw t1,  4 * 4(sp)\n"
      "lw t2,  4 * 5(sp)\n"
      "lw t3,  4 * 6(sp)\n"
      "lw t4,  4 * 7(sp)\n"
      "lw t5,  4 * 8(sp)\n"
      "lw t6,  4 * 9(sp)\n"
      "lw a0,  4 * 10(sp)\n"
      "lw a1,  4 * 11(sp)\n"
      "lw a2,  4 * 12(sp)\n"
      "lw a3,  4 * 13(sp)\n"
      "lw a4,  4 * 14(sp)\n"
      "lw a5,  4 * 15(sp)\n"
      "lw a6,  4 * 16(sp)\n"
      "lw a7,  4 * 17(sp)\n"
      "lw s0,  4 * 18(sp)\n"
      "lw s1,  4 * 19(sp)\n"
      "lw s2,  4 * 20(sp)\n"
      "lw s3,  4 * 21(sp)\n"
      "lw s4,  4 * 22(sp)\n"
      "lw s5,  4 * 23(sp)\n"
      "lw s6,  4 * 24(sp)\n"
      "lw s7,  4 * 25(sp)\n"
      "lw s8,  4 * 26(sp)\n"
      "lw s9,  4 * 27(sp)\n"
      "lw s10, 4 * 28(sp)\n"
      "lw s11, 4 * 29(sp)\n"
      "lw sp,  4 * 30(sp)\n"

      // Return from the trap
      "sret\n");
}

/**
 * @brief The kernel's main function.
 *
 * Initializes memory, sets up the trap entry point, and halts execution.
 */
void kernel_main(void) {
  // Zero out the BSS section
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

  // Set the trap entry point
  WRITE_CSR(stvec, (uint32_t)kernel_entry);

  // Halt the CPU (for demonstration)
  __asm__ __volatile__("unimp");
}

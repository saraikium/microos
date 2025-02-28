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
extern char __free_ram[];
extern char __free_ram_end[];
extern char __kernel_base[];

struct process procs[PROCS_MAX];

// Alocate request number of pages memeory
paddr_t alloc_pages(uint32_t n) {
  static paddr_t next_paddr = (paddr_t)__free_ram;
  paddr_t paddr = next_paddr;
  next_paddr += n * PAGE_SIZE;

  if (next_paddr > (paddr_t)__free_ram_end)
    PANIC("Out of Memory");

  memset((void *)paddr, 0, n * PAGE_SIZE);
  return paddr;
}

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
  uint32_t scause = READ_CSR(scause); //< Trap cause
  uint32_t stval = READ_CSR(stval);   //< Faulting address or value
  uint32_t user_pc = READ_CSR(sepc); //< Program counter at the time of the trap

  // Print panic message and trap diagnostics
  PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
        user_pc);
}

__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
  __asm__ __volatile__(
      // Save callee-saved registers onto the current process's stack.
      "addi sp, sp, -13 * 4\n" // Allocate stack space for 13 4-byte registers
      "sw ra,  0  * 4(sp)\n"   // Save callee-saved registers only
      "sw s0,  1  * 4(sp)\n"
      "sw s1,  2  * 4(sp)\n"
      "sw s2,  3  * 4(sp)\n"
      "sw s3,  4  * 4(sp)\n"
      "sw s4,  5  * 4(sp)\n"
      "sw s5,  6  * 4(sp)\n"
      "sw s6,  7  * 4(sp)\n"
      "sw s7,  8  * 4(sp)\n"
      "sw s8,  9  * 4(sp)\n"
      "sw s9,  10 * 4(sp)\n"
      "sw s10, 11 * 4(sp)\n"
      "sw s11, 12 * 4(sp)\n"

      // Switch the stack pointer.
      "sw sp, (a0)\n" // *prev_sp = sp;
      "lw sp, (a1)\n" // Switch stack pointer (sp) here

      // Restore callee-saved registers from the next process's stack.
      "lw ra,  0  * 4(sp)\n" // Restore callee-saved registers only
      "lw s0,  1  * 4(sp)\n"
      "lw s1,  2  * 4(sp)\n"
      "lw s2,  3  * 4(sp)\n"
      "lw s3,  4  * 4(sp)\n"
      "lw s4,  5  * 4(sp)\n"
      "lw s5,  6  * 4(sp)\n"
      "lw s6,  7  * 4(sp)\n"
      "lw s7,  8  * 4(sp)\n"
      "lw s8,  9  * 4(sp)\n"
      "lw s9,  10 * 4(sp)\n"
      "lw s10, 11 * 4(sp)\n"
      "lw s11, 12 * 4(sp)\n"
      "addi sp, sp, 13 * 4\n" // pop 13 4-byte registers from the stack
      "ret\n");
}

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
  if (!is_aligned(vaddr, PAGE_SIZE))
    PANIC("unaligned vaddr %x", vaddr);

  if (!is_aligned(paddr, PAGE_SIZE))
    PANIC("unaligned paddr %x", paddr);

  uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
  if ((table1[vpn1] & PAGE_V) == 0) {
    // Create the non-existent 2nd level page table.
    uint32_t pt_paddr = alloc_pages(1);
    table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
  }

  // Set the 2nd level page table entry to map the physical page.
  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
  uint32_t *table0 = (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE);
  table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

struct process *create_process(uint32_t pc) {
  struct process *proc = NULL;

  // Check if we have an availble slot to run the process
  int i;
  for (i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_UNUSED) {
      proc = &procs[i];
      break;
    }
  }

  if (!proc) {
    PANIC("No free process slots");
  }

  // Stack callee-saved registers. These register values will be restored in
  // the first context switch in switch_context.
  uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
  for (int i = 0; i < 12; i++) {
    *--sp = 0; // Zero out the s11 to s0 (Callee-saved registers)
  }
  *--sp = (uint32_t)pc;

  uint32_t *page_table = (uint32_t *)alloc_pages(1);
  for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end;
       paddr += PAGE_SIZE) {
    map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
  }

  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t)sp;
  proc->page_table = page_table;

  return proc;
};

/**
 * The kernel trap entry point.
 *
 * Saves the CPU state, calls the trap handler, and restores the state before
 * returning.
 */
__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
  __asm__ __volatile__(

      // Retrieve the kernel stack of the running process from sscratch.
      "csrrw sp, sscratch, sp\n"
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

      // Retrieve and save the sp at the time of exception.
      "csrr a0, sscratch\n"
      "sw a0,  4 * 30(sp)\n"

      // Reset the kernel stack.
      "addi a0, sp, 4 * 31\n"
      "csrw sscratch, a0\n"

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

struct process *current_proc;
struct process *idle_proc;

void yield(void) {
  struct process *next = idle_proc;

  for (int i = 0; i < PROCS_MAX; i++) {
    struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
    if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
      next = proc;
      break;
    }
  }

  if (next == current_proc)
    return;

  // Context switch
  struct process *prev = current_proc;
  current_proc = next;

  __asm__ __volatile__(
      "sfence.vma\n"
      "csrw satp, %[satp]\n"
      "sfence.vma\n"
      "csrw sscratch, %[sscratch]\n"
      :
      : [satp] "r"(SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)),
        [sscratch] "r"((uint32_t)&next->stack[sizeof(next->stack)]));

  switch_context(&prev->sp, &next->sp);
}

void delay(void) {
  for (int i = 0; i < 30000000; i++) {
    __asm__ __volatile__("nop");
  }
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
  printf("Starting process A\n");
  while (1) {
    putchar('A');
    yield();
  }
}

void proc_b_entry(void) {
  printf("Starting process B\n");
  while (1) {
    putchar('B');
    yield();
  }
}

/**
 * @brief The kernel's main function.
 *
 * Initializes memory, sets up the trap entry point, and halts execution.
 */
void kernel_main(void) {
  // Zero out the BSS section
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

  printf("\n\n");

  WRITE_CSR(stvec, (uint32_t)kernel_entry);

  idle_proc = create_process((uint32_t)NULL);
  idle_proc->pid = -1; // idle
  current_proc = idle_proc;

  proc_a = create_process((uint32_t)proc_a_entry);
  proc_b = create_process((uint32_t)proc_b_entry);
  proc_a_entry();
  yield();

  PANIC("Switch to idle process.\n");
}

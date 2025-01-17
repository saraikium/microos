int main(int argc, char **argv) {
  int value = 0;
  __asm__ __volatile__("csrr %0, sepc" : "=r"(value));
  return 0;
};

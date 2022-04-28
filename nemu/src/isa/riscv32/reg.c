#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  int i;
  printf("General reg:  -------------------------------------------------------------------------------\n");
  for ( i = 0; i < 32; i++){
	  printf("%s\t:0x%08x  |  ", reg_name(i, 0), gpr(i));
    if((i+1)%4 == 0){
      printf("\n");
    }
  }
  printf("Special reg: -------------------------------------------------------------------------------\n");
  printf("pc\t:0x%08x\n", cpu.pc);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  if(!strcmp(s, "pc")){
    // printf("match: %s\n", s);
    *success = true;
    return cpu.pc;
  }
  for(int i = 0; i < 32; i++) {
    if(!strcmp(s, reg_name(i, 0))){
      // printf("match reg: %s\n", reg_name(i, 0));
      *success = true;
      return gpr(i);
    }
  }
  *success = false;
  return 0;
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int choose(int n){
  return n*(rand()*1.0/RAND_MAX);
}

void gen_num(){
  switch(choose(2)){
    case 0: {
      // if (choose(2)) strcat(buf, "-");
      char ch = '1' + choose (9);
      strcat(buf, &ch); 
      for (int i = 0; i < choose (3); i++) {
        ch = '0' + choose (10);
        strcat(buf, &ch); 
      }
    }break;
    case 1: {
      strcat(buf, choose (2)?"0x0":"0X0");
      for (int i = 0; i < choose(8); i++) {
        char ch;
        switch (choose(3)){
          case 0: ch = '0' + choose(10); break; 
          case 1: ch = 'a' + choose(6); break;
          case 2: ch = 'A' + choose(6); break;
          default: break;
        }
        strcat(buf, &ch); 
      }
    }
    default: break; 
  }
}

void gen(char ch){
  strcat(buf, &ch);
}

// calculate the length of an array
#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))

static char * op[] = {
  "+", "-", "*", "/", "==", "!=", "||", "&&",
};

void gen_rand_op(){
  strcat(buf, op[choose(ARRLEN(op))]);
}


static void gen_rand_expr() {
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr(); gen(')'); break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();
    printf("gen_rand_expr: %s\n", buf);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    if(EOF==fscanf(fp, "%d", &result)) printf("fscanf failed\n");  
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}

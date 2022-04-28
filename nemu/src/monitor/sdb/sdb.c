#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/vaddr.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
typedef struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd;

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args){
  int step;
  if (args == NULL) step = 1;
  else sscanf(args, "%d", &step);
  cpu_exec(step);
  return 0;
}

static int cmd_info_r(char *args){
  char *arg = strtok(NULL, " ");
  bool success = false;
  if (arg == NULL) isa_reg_display();
  else printf("%s:\t:0x%08x", arg, isa_reg_str2val(arg, &success));

  return 0; 
}

static int cmd_info_w(char *args){
  // char *arg = strtok(NULL, " ");
  wp_display();
  return 0; 
}

static cmd cmd_info_table [] = {
  { "r", "Print register info.", cmd_info_r },
  { "w", "Print watchpoint info.", cmd_info_w },
  /* TODO: Add more commands */
};
#define NR_CMD_INFO ARRLEN(cmd_info_table)


static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD_INFO; i ++) {
      printf("info %s -- %s\n", cmd_info_table[i].name, cmd_info_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD_INFO; i ++) {
      if (strcmp(arg, cmd_info_table[i].name) == 0) {
		if (cmd_info_table[i].handler(arg) < 0) {
		  printf("info %s failed\n", cmd_info_table[i].name);
		   break;
		}
        return 0;
      }
    }
    if (i == NR_CMD_INFO) { printf("Unknown info subcommand '%s'\n", arg); }
  }
  return 0;
}

static int cmd_x(char *args){
  paddr_t addr;
  int len = 1;

  char* arg1 = strtok(NULL, " ");
  char* arg2 = strtok(NULL, " ");

  if(arg1 == NULL || arg2 == NULL) return 0;    

  sscanf(arg1, "%d", &len);
  sscanf(arg2, "%x", &addr);

  for(int i = 0 ; i < len ; i++){
    word_t data = vaddr_read(addr + i * 4,4);
    printf("0x%08x  " , addr + i * 4 );
    for(int j =0 ; j < 4 ; j++){
        printf("0x%02x " , data & 0xff);
        data = data >> 8 ;
    }
    printf("\n");
  }
  return 0;
}

static int cmd_p(char *args){
  if (args == NULL) return 0; 
  bool success = true;

  word_t ret = expr(args, &success);

  if (!success) return 0;
  else printf("ans: %u\n", ret);

  return 0;
}

static int cmd_w(char *args){
  if (args == NULL) return 0; 
  // word_t ret = expr(args, &success);
  WP* wp = wp_new(args);
  if(wp!=NULL) Log("new wp: %d\t", wp->NO);
  return 0; 
}

static int cmd_d(char *args){
  char* arg = strtok(NULL, " ");
  if(arg == NULL) return 0;
  int wp_no = 0;
  sscanf(arg, "%d", &wp_no);
  wp_delete(wp_no);
  return 0; 
}

static int cmd_help(char *args);

static cmd cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step program until it reaches a different source line.\n\
	        \r\tUsage: step [N]\n\
	        \r\tArgument N means step N times (or till program stops for another reason).", cmd_si },
  { "info", "List of integer registers and their contents, for selected stack frame.\n\
          \r\tinfo [r/w] ", cmd_info },
  { "x", "Examine memory\n \
          \r\tUsage: x [N] [EXPR]", cmd_x },
  { "p", "Print value of expression EXP.", cmd_p },
  { "w", "Set a watchpoint for an expression.\n\
          \r\tUsage: w [EXPR] when EXPR changes, stop the program.", cmd_w },
  { "d", "Delete all or some breakpoints.", cmd_d },


  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s \t- %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

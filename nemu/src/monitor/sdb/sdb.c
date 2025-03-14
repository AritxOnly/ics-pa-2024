/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "debug.h"
#include "memory/vaddr.h"
#include <stdbool.h>
#include <string.h>
#include <utils.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void delete_wp(int n);
void insert_wp(const char* expr);
bool sdb_wp_display();

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
  nemu_state.state = NEMU_QUIT; // 按下Q时，将NEMU状态设置为NEMU_QUIT
  return -1;
}

/** 
 * PA1 impl commands
 * TODO: si [N] command
 * TODO: info SUBCMD command
 * TODO: x N EXPR command
 * TODO: p EXPR command
 * TODO: w EXPR command
 * TODO: d N command
 */
static int cmd_si(char *args) {
  char *arg = strtok(args, " ");
  int step;

  if (arg == NULL) {
    step = 1; // 默认值为1
  }
  else {
    sscanf(arg, "%d", &step);
  }
  cpu_exec(step); // CPU执行step步指令
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(args, " ");
  if (strcmp(arg, "r") == 0) {
    isa_reg_display();
  } else if (strcmp(arg, "w") == 0) {
    sdb_wp_display();
  } else {
    printf("Invalid argument\n");
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(args, " ");
  char *exp = strtok(NULL, " ");
  if (exp == NULL || arg == NULL) {
    printf("Invalid Arguments\n");
    return 0;
  }

  int n;
  vaddr_t addr;
  bool succ = true;
  sscanf(arg, "%d", &n);
  addr = expr(exp, &succ);
  Assert(succ, "Calculate expression fault!");

  for (int i = 0; i < n; i++) {
    word_t data = vaddr_read(addr, 4);
    printf("Addr:0x%08x\t\t0x%08x\t\t%d\n", addr, data, data);
    addr += 4;
  }

  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Invalid arguments\n");
    return 0;
  }
  bool success = true;
  uint32_t value = expr(args, &success);
  if (success)
    printf("result: %u (0x%08x)\n", value, value);
  else
    printf("Invalid expression\n");
  return 0;
}

static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Invalid arguments\n");
    return 0;
  }
  /**
   * TODO: 表达式求值
   */
  insert_wp(args);
  
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Invalid arguments\n");
  } else {
    int n = atoi(arg);
    delete_wp(n);
  }
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si", "Execute N steps. If N isn’t provided, default to 1.", cmd_si},
  {"info", "Display information based on the specified subcommand.\n\
            Available Subcommands:\n\
            \tr  - Print the status of registers.\n\
            \tw  - Print information about watchpoints.", cmd_info},
  {"x", "Use: x N EXPR\n\
            Calculate the EXPR arg as the initial memory pointer\n\
            Print out the following N values", cmd_x},
  {"p", "Use: p EXPR\n\
            Print out the value of the EXPR", cmd_p},
  {"w", "Use: w EXPR\n\
            Set watchpoint for EXPR", cmd_w},
  {"d", "Use: d N\n\
            Delete the watchpoint with ID N", cmd_d}
  
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
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
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
     * which may need further rsing
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

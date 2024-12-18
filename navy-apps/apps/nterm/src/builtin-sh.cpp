#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>
#include <stdlib.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static int cmd_echo(char *msg) {
  sh_printf(msg);
  return 0;
}

static int cmd_exit(char *args) {
  char *first = strtok(args, " ");
  int arg = 0;
  sscanf(args, "%d", &arg);
  exit(arg);
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  {"echo", "Echo back the input", cmd_echo},
  {"exit", "Exit nterm", cmd_exit},
};

#define NR_CMD sizeof(cmd_table) / sizeof(cmd_table[0])

// #error cmd's "\n" should be handled
static void sh_handle_cmd(const char *cmd) {
  char buf[256];
  strncpy(buf, cmd, strlen(cmd) - 1); // 去掉末尾的\n
  buf[strlen(cmd) - 1] = '\0';
  char *first = strtok((char *)buf, " ");
  char *args = (char *)buf + strlen(buf) + 1;
  int i;
  for (i = 0; i < NR_CMD; i ++) {
    if (strcmp(buf, cmd_table[i].name) == 0) {
      if (cmd_table[i].handler(args) < 0) { return; }
      break;
    }
  }
  if (i == NR_CMD) {
    char *token = strtok(args, " ");
    char *argv[64] = { first, token };
    int ptr = 2;
    while (token != NULL) {
      printf("%s\n", token);
      token = strtok(NULL, " ");
      argv[ptr++] = token;
    }

    execvp(buf, argv);
  }
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();
  setenv("PATH", "/bin", 0);

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}

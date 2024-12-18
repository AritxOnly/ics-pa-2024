#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>

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

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  {"echo", "", cmd_echo},
};

#define NR_CMD 1

static void sh_handle_cmd(const char *cmd) {
  char *first = strtok((char *)cmd, " ");
  char *args = (char *)cmd + strlen(cmd) + 1;
  int i;
  for (i = 0; i < NR_CMD; i ++) {
    if (strcmp(cmd, cmd_table[i].name) == 0) {
      if (cmd_table[i].handler(args) < 0) { return; }
      break;
    }
  }
  if (i == NR_CMD) {
    FILE *fp = fopen(cmd, "r");
    if (!fp) {
      printf("Unknown command '%s'\n", cmd);
    } else {
      execve(cmd, NULL, NULL);
      fclose(fp);
    }
  }
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

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

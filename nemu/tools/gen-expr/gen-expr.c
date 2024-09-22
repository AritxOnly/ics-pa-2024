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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <math.h>

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

int ptr = 0;

static void gen_rand_expr();

static uint32_t choose(uint32_t n) {
  if (n == 0) return rand();
  int r = rand() % n;
  return r;
}

static void gen_rand_op() {
  if (ptr + 1 >= 65536) {
    ptr = 0;
    memset(buf, 0, sizeof(buf));
    gen_rand_expr();
  }
  switch (choose(4)) {
    case 0: buf[ptr++] = '+';  break;
    case 1: buf[ptr++] = '-';  break;
    case 2: buf[ptr++] = '*';  break;
    case 3: buf[ptr++] = '/';  break;
    default:  assert(0);  break;
  }
}

static void gen(char c) {
  if (ptr + 1 >= 65536) {
    ptr = 0;
    memset(buf, 0, sizeof(buf));
    gen_rand_expr();
  }
  buf[ptr++] = c;
}

static void gen_num() {
  if (ptr + 1 >= 65536) {
    ptr = 0;
    memset(buf, 0, sizeof(buf));
    gen_rand_expr();
  }
  int max_len = 65536 - ptr;
  max_len = (max_len > 10) ? 0 : max_len;
  uint32_t num = choose(pow(10, max_len));
  char str[12]; // should be enough
  sprintf(str, "%d", num);  // 格式化
  buf[ptr] = '\0';
  strcat(buf, str);
  ptr += strlen(str);
}

static void gen_rand_expr() {
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr();  gen(')'); break;
    default:  gen_rand_expr();  gen_rand_op();  gen_rand_expr();
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
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}

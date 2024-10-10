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

int recursion_depth = 0;

static void gen_rand_expr();
static void gen(char c);

// static uint32_t uint_pow(uint32_t x, uint32_t y) {
//   if (y == 0) return 0;
//   int i, res = 1;
//   for (i = 0; i < y; i++) {
//     res *= x;
//   }
//   return res;
// }

static uint32_t choose(uint32_t n) {
  int r = rand() % n;
  return r;
}

static void gen_rand_op() {
  char operands[] = {'+', '-', '*', '/'};
  char str[] = {operands[choose(4)], '\0'};
  assert(strlen(buf) + 1 < 30000);
  strcat(buf, str);
}

static void gen(char c) {
  char str[] = {c, '\0'};
  assert(strlen(buf) + 1 < 30000);
  strcat(buf, str);
}

static void gen_num() {
  char num_str[32];
  sprintf(num_str, "%d", choose((unsigned)(-1)));
  assert(strlen(buf) + strlen(num_str) < 30000);
  strcat(buf, num_str);
}

static void gen_space() {
  int cycle = choose(3);
  for (int i = 0; i < cycle; i++) {
    if (choose(100) > 90) {
      assert(strlen(buf) + 1 < 30000);
      strcat(buf, " ");
    }
  }
}

static void gen_rand_expr() {
  gen_space();
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr();  gen(')'); break;
    default:  gen_rand_expr();  gen_rand_op();  gen_rand_expr();
  }
  gen_space();
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

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

#include "common.h"
#include "debug.h"
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdio.h>

enum {
  TK_NOTYPE = 256, TK_EQ = 255,

  /* TODO: Add more token types */
  TK_INT_DEC = 0,
  TK_INT_HEX = 1,
  TK_BRACKET_L = 2,
  TK_BRACKET_R = 3,


};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // space
  {"[0-9]+", TK_INT_DEC}, // 10 base integers

  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiply
  {"\\/", '/'},         // divide

  {"\\(", TK_BRACKET_L},  // left bracket
  {"\\)", TK_BRACKET_R},  // right bracket

  {"==", TK_EQ},        // equal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_INT_DEC:  // 十进制时将token的内容也存入str
            tokens[nr_token].type = rules[i].token_type;
            if (substr_len <= 32) { // 缓冲区未溢出
              strncpy(tokens[nr_token++].str, substr_start, substr_len); 
            } else {
              assert(0);
            }
            break;
          default:
            tokens[nr_token++].type = rules[i].token_type;
            break;
        }

        break;
      }
    }

    Log("nr_token: %d", nr_token);

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


static bool check_parentheses(int p, int q) {
  Assert(p <= q, "Bad expression");
  if (tokens[p].type == TK_BRACKET_L && tokens[q].type == TK_BRACKET_R) {
    int depth = 0;
    for (int i = p; i <= q; i++) {
      if (tokens[i].type == TK_BRACKET_L) {
        depth++;
      } else if (tokens[i].type == TK_BRACKET_R) {
        depth--;
      }
      if (depth < 0) {
        Log("Bad expression");
        return false; // bad expression
      }
    }
    return true;
  }
  return false;
}

static int find_main_operand(int p, int q) {
  int op = p;
  int priority = 3;
  int depth = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_BRACKET_L) {
      depth++;
    } else if (tokens[i].type == TK_BRACKET_R) {
      depth--;
    }
    if (depth < 0) {
      Log("Bad expression");
      return false; // bad expression
    }
    bool is_operand = tokens[i].type == '+' || tokens[i].type == '-' ||
                      tokens[i].type == '*' || tokens[i].type == '/';
    if (is_operand && depth == 0) {
      int cur_prior = (tokens[i].type == '+' || tokens[i].type == '-') ?
                      1 : 2;
      if (cur_prior <= priority) {
        op = i;
        priority = cur_prior;
      }
    }
  }
  return op;
}

static int eval(int p, int q) {
  Assert(p <= q, "Bad expression");
  if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    int num;
    sscanf(tokens[p].str, "%d", &num);
    return num;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op = find_main_operand(p, q);
    int val1 = eval(p, op - 1); // 主运算符左边的值
    int val2 = eval(op + 1, q); // 主运算符右边的值

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  int result = eval(0, nr_token);

  return result;
}


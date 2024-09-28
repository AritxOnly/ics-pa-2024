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

#include "sdb.h"

#define NR_WP 32

// Watchpoint链表
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  bool is_occupied;
  char str[32]; // 存储表达式

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].is_occupied = false;
  }

  head = NULL;  // 使用中监视点链表的头节点
  free_ = wp_pool;  // 未使用监视点链表的头节点
}

/* TODO: Implement the functionality of watchpoint */
/* 分配一个新的监视点 */
WP* new_wp() {
  if (free_ == NULL) {
    printf("No free watchpoints available!\n");
    return NULL;
  }

  // 从free_链表中取出一个监视点
  WP* wp = free_;
  free_ = free_->next;

  // 插入到使用中监视点链表的头部 
  wp->next = head;
  head = wp;
  wp->is_occupied = true;

  printf("Watchpoint %d allocated\n", wp->NO);
  return wp;
}

/* 释放一个指定的监视点 */
void free_wp(WP* wp) {
  if (wp == NULL || !wp->is_occupied) {
    printf("Invalid watchpoint to free.\n");
    return;
  }

  // 从使用中链表中删除
  if (head == wp) {
    head = wp->next;
  } else {
    WP* cur = head;
    while (cur != NULL && cur->next != wp) {
      cur = cur->next;
    }
    if (cur != NULL) {
      cur->next = wp->next;
    }
  }

  // 将wp返回到free_链表的头部
  wp->next = free_;
  free_ = wp;
  wp->is_occupied = false;
  memset(wp->str, 0, sizeof(wp->str));

  printf("Watchpoint %d freed.\n", wp->NO);
}

void delete_wp(int n) { // 删除编号为n的watchpoint
  if (n < 0 || n >= NR_WP) {
    printf("Invalid deletion\n");
  }
  WP* cur = head;
  while (cur != NULL) {
    if (cur->NO == n) {
      free_wp(cur);
      return;
    }
    cur = cur->next;
  }
  printf("Invalid deletion\n");
}

void insert_wp(const char* expr) {
  WP* wp = new_wp();
  if (wp == NULL) {
    return;
  } else {
    strncpy(wp->str, expr, 32);
  }
}

void sdb_wp_display() {
  printf("Activated registers status\n");
  WP* cur = head;
  bool success = true;
  while (cur) {
    printf("  %d:\t%s == %u\n", cur->NO, cur->str, expr(cur->str, &success));
    cur = cur->next;
  }
  if (!success) assert(0);
}
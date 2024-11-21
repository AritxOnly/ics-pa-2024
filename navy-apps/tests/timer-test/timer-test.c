#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <NDL.h>

int main() {
    NDL_Init(0);
    uint32_t start = NDL_GetTicks();

    while (1) {
        uint32_t current = NDL_GetTicks();  // 获取当前时间

        long elapsed = current - start;

        if (elapsed >= 5000) {  // 判断是否超过0.5秒
            printf("passed 0.5 seconds\n");
            fflush(stdout);  // 刷新输出缓冲区
            start = NDL_GetTicks();
        }
    }

    return 0;
}
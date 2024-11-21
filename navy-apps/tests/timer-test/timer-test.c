#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

int main() {
    struct timeval start, current;
    gettimeofday(&start, NULL);  // 获取起始时间

    while (1) {
        gettimeofday(&current, NULL);  // 获取当前时间

        // 计算时间差，单位为微秒
        long elapsed = (current.tv_sec - start.tv_sec) * 1000000 + (current.tv_usec - start.tv_usec);

        if (elapsed >= 500000) {  // 判断是否超过0.5秒（500,000微秒）
            printf("已经过去了0.5秒。\n");
            fflush(stdout);  // 刷新输出缓冲区
            gettimeofday(&start, NULL);  // 重置起始时间
        }
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void EXIT(int sig);
void alarmfunc(int num);

int main() {
  for (int ii=1;ii<=64;++ii) {
    signal(ii,SIG_IGN);
  }
  signal(SIGINT,EXIT);
  signal(SIGTERM,EXIT);

  while (1) {
    printf("执行了一次任务\n");
    sleep(1);
  }
}
void EXIT(int sig) {
  printf("接收到了%d信号，程序将退出。\n",sig);
  //这里编写善后代码
  exit(0);
}

void alarmfunc(int num) {
  printf("接收到了时钟信号%d。\n",num);
  alarm(3);
}

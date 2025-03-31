#ifndef DAEMON_RUN_H
#define DAEMON_RUN_H

/** 
 * 程序已守护进程方式运行
 * zhangyl 2018.08.20
 */
#ifndef WIN32
void daemon_run();

// 退出时的清理函数
void prog_exit(int signo);
#endif

#endif // daemon_run_h
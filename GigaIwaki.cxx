#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <ctime>
#include <sstream>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <syslog.h>
#include <poll.h>
#include <unistd.h>
#include <atomic>

#include "TCPclient.h"

using namespace std;

// ★ 签名改成 atomic end_flag + ready
void GigaIwaki(const char *host, std::atomic<bool> &end_flag, std::atomic<int> &ready){
  string h_tmp = host;

  // 你原来这里 sleep(b_id) 是错峰启动，会导致最早几秒必漏数据
  // 如果你一定要错峰，可以打开下面两行，但那 ready==12 会慢很多。
  // int b_id = atoi(&h_tmp[h_tmp.length()-1]);
  // sleep(b_id);

  TCPclient iwaki(host, 24);

  // Read run counter
  ifstream ifs_runnum("runnum.dat");
  if(!ifs_runnum){
    cerr << "Cannot read the runnum file." << endl;
    return;
  }

  string str_runnum;
  ifs_runnum >> str_runnum;
  int runnum = atoi(str_runnum.c_str());
  ifs_runnum.close();

  // Read data dir
  ifstream ifs_datadir("./datadir.dat");
  if(!ifs_datadir){
    cerr << "Cannot read the datadir file." << endl;
    return;
  }

  string str_datadir;
  ifs_datadir >> str_datadir;
  ifs_datadir.close();

  if(iwaki.isAlive()){
    bool file_opened = false;
    ofstream OutData;
    unsigned FileNum = 0;

    // 先打开文件（binary）
    {
      char filename[300];
      sprintf(filename, "%s/uTPC_%04d_%s_%05d.raw",
              str_datadir.c_str(), runnum, host, FileNum);
      OutData.open(filename, ios::out | ios::binary);
      if(!OutData){
        cerr << "[" << host << "] Cannot open output file: " << filename << endl;
        iwaki.Close();
        return;
      }
      file_opened = true;
    }

    // ★ 到这里：TCPclient 已创建 + isAlive 成功 + 输出文件已打开
    //    认为“接收端 ready”，通知主线程
    int r = ready.fetch_add(1, std::memory_order_relaxed) + 1;
    // 可选：打印
    // printf("[%s] ready (%d/12)\n", host, r);

    char msg[2000];

    while(!end_flag.load(std::memory_order_relaxed)){
      int num = iwaki.RecieveMsg(msg, 2000);
      if(num > 0){
        OutData.write(msg, num);

        // 64GB rollover：你原来用 tellp 判断OK，但要确保是 binary 且 tellp 可用
        if(OutData.tellp() > static_cast<std::streampos>(0xfffffffffLL)){
          OutData.close();
          FileNum++;

          char filename[300];
          sprintf(filename, "%s/uTPC_%04d_%s_%05d.raw",
                  str_datadir.c_str(), runnum, host, FileNum);
          OutData.open(filename, ios::out | ios::binary);
          if(!OutData){
            cerr << "[" << host << "] Cannot open next file: " << filename << endl;
            break;
          }
        }
      } else {
        // num <= 0：可以稍微让一下 CPU，避免空转（视 TCPclient 行为）
        // usleep(100);
      }
    }

    if(file_opened) OutData.close();
    iwaki.Close();
  } else {
    cerr << "[" << host << "] TCPclient is not alive." << endl;
  }
}

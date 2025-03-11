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

#include "TCPclient.h"

using namespace std;


void GigaIwaki(const char *host, bool &end_flag){
  string h_tmp = host;
  int b_id = atoi(&h_tmp[h_tmp.length()-1]);
  sleep(b_id);

  char ql_id = b_id & 0xff;
  if(h_tmp.find("anode") != std::string::npos)
    ql_id += 0x40;
  else
    ql_id += 0x80;


  TCPclient iwaki(host, 24);

  // Read run counter
  ifstream ifs_runnum("runnum.dat");
  if(!ifs_runnum){
    cerr << "Cannot read the runnum file." << endl;
    exit(1);
  }

  string str_runnum;
  ifs_runnum >> str_runnum;
  int runnum = atoi(str_runnum.c_str());
  ifs_runnum.close();  


  // Read data dir
  ifstream ifs_datadir("./datadir.dat");
  if(!ifs_datadir){
    cerr << "Cannot read the datadir file." << endl;
    exit(1);
  }

  string str_datadir;
  ifs_datadir >> str_datadir;
  ifs_datadir.close();  
  

  if(iwaki.isAlive()){
    bool file_opened=0;
    ofstream OutData;
    unsigned FileNum=0;

    while(!end_flag){
      if(!file_opened){
	char filename[300];
	sprintf(filename, "%s/uTPC_%04d_%s_%05d.raw",
		str_datadir.c_str(), runnum, host, FileNum);
	OutData.open(filename);
	if(OutData)
	  file_opened = 1;
      }
	  
      char msg[2000];
      int num = iwaki.RecieveMsg(msg, 2000);
      if(num>0){
	OutData.write(msg, num);

	if(OutData.tellp() > 0xfffffffff){ // 64GB maximum
	  OutData.close();
	  FileNum++;

	  char filename[300];
	  sprintf(filename, "uTPC_%s_%05d.raw", host, FileNum);
	  OutData.open(filename);
	}
      }
    }

    if(file_opened)
      OutData.close();

    iwaki.Close();
  }
}

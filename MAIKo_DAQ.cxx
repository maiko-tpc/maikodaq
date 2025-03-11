#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <ctime>
#include <mutex>
#include <thread>
#include <queue>
#include <unistd.h>
#include <syslog.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include "vme-gbe-comm.h"

using namespace std;
void GigaIwaki(const char*, bool&);

bool end_flag;

void EndSeq(int){
  end_flag = 1;
}

const std::string VMEGBE2_IP = "10.10.10.36";
const unsigned short RPV_LEVEL = 0xf00a;
const unsigned short RPV_PULSE = 0xf008;

int WriteRPV(std::string ip_address,
	      unsigned short vme_address,
	      unsigned short data){

  int ret = 0;
  try{
    // TCP port 24
    VmeGbeComm comm(ip_address, 24);
    //Address mode A16
    comm.setAddressMode(VmeGbeComm::A16);

    unsigned short buf = htons(data);

    // 2 byte, ID == 1 (arbitrary?)
    ret = comm.write(vme_address, 2, 1,
		     VmeGbeComm::D16, (char*)&buf);
    
  }
  catch(std::string &e_str){
    std::cerr << "An exception caught in WriteRPV()" << std::endl;
    std::cerr << "IP " << ip_address << std::endl;
    std::cerr << std::hex << "VME address " << vme_address << std::endl;
    std::cerr << std::hex << "data " << data << std::endl;
    std::cerr << e_str;
  }
 catch(std::exception &e_ex){
    std::cerr << "An exception caught in WriteRPV()" << std::endl;
    std::cerr << "IP " << ip_address << std::endl;
    std::cerr << std::hex << "VME address " << vme_address << std::endl;
    std::cerr << std::hex << "data " << data << std::endl;
    std::cerr << e_ex.what();
  }
 catch(...){
  std::cerr << "An unexpected exception caught in WriteRPV()" << std::endl;
    std::cerr << "IP " << ip_address << std::endl;
    std::cerr << std::hex << "VME address " << vme_address << std::endl;
    std::cerr << std::hex << "data " << data << std::endl;
 }

  return ret;
  
}


int main(int argc, char *argv[]){
  end_flag = 0;
  signal(SIGINT, EndSeq);

  int res;

  if(argc!=2){
    cerr << "" << endl;
    cerr << " USAGE> ./MAIKo_DAQ [run comment]" << endl;
    cerr << " Example> ./MAIKo_DAQ \"alpha source, upic 400V\" " << endl;    
    cerr << "" << endl;
    exit(1); 
  }

  
  // Read run counter
  ifstream ifs_runnum("runnum.dat");
  if(!ifs_runnum){
    cerr << "Cannot read the runnum file." << endl;
    exit(1);
  }

  string str_runnum;
  ifs_runnum >> str_runnum;
  int runnum = atoi(str_runnum.c_str());
  cout << "Run number: " << runnum+1 << endl;

  ifs_runnum.close();

  // Increment the Run number file
  ofstream ofs_runnum("runnum.dat", ios_base::trunc);
  ofs_runnum << runnum+1 << endl;
  ofs_runnum.close();
  
  // Get start time
  time_t timer;                                                                   struct tm *local;                                                               int year, month, day, hour, min, sec;                                           timer = time(NULL);                                                             local = localtime(&timer);                                                      year = local->tm_year + 1900;                                                   month = local->tm_mon + 1;                                                      day = local->tm_mday;
  hour = local->tm_hour;                                                       
  min = local->tm_min;                                                         
  sec = local->tm_sec;            
  
  printf("DAQ start time: %d/%02d/%02d %02d:%02d:%02d\n",
	 year, month, day, hour, min, sec);
  
  // Write Run comment
  string runcom = argv[1];
  ofstream ofs_runcom("runcom.dat", ios::app);  
  char comment[1024];
  sprintf(comment, "%04d START:%d/%02d/%02d %02d:%02d:%02d  %s",
	  runnum+1,
	  year, month, day, hour, min, sec,
	  runcom.c_str());
  ofs_runcom << comment << endl;
  ofs_runcom.close();
  
  // trigger disable !!!
  //  sprintf(cmd, "./trg_disable.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_LEVEL, 0x20);
  printf("Trigger disable: %d\n", res);


  
  // visual Scaler reset !!!
  //  sprintf(cmd, "./sca_reset.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_PULSE, 0x04);
  printf("Scaler reset: %d\n", res);

  // visual Scaler start !!!
  //  sprintf(cmd, "./sca_start.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_PULSE, 0x01);
  printf("Scaler start: %d\n", res);
  
  
//  // DAQ disable
//  sprintf(cmd, "./daq_disable.sh");
//  res = system(cmd);
//  printf("DAQ disable: %d\n", res);

  // counter reset !!!
  //  sprintf(cmd, "./cntrst.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_PULSE, 0x40);
  printf("cntrst: %d\n", res);

  // DAQ enable !!!
  //  sprintf(cmd, "./daq_enable.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_LEVEL, 0xa0);
  printf("DAQ enable: %d\n", res);

  // sleep
  sleep(2);  

  // Trigger enable !!!
  //  sprintf(cmd, "./trg_enable.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_LEVEL, 0x80);
  printf("Trigger enable: %d\n", res);
  

  printf("Press Cntl-C to stop the DAQ.\n");

  
  thread Anode0(GigaIwaki, "anode0", ref(end_flag));
  thread Anode1(GigaIwaki, "anode1", ref(end_flag));
  thread Anode2(GigaIwaki, "anode2", ref(end_flag));
  thread Anode3(GigaIwaki, "anode3", ref(end_flag));  
  thread Anode4(GigaIwaki, "anode4", ref(end_flag));
  thread Anode5(GigaIwaki, "anode5", ref(end_flag));  

  thread Cathode0(GigaIwaki, "cathode0", ref(end_flag));
  thread Cathode1(GigaIwaki, "cathode1", ref(end_flag)); 
  thread Cathode2(GigaIwaki, "cathode2", ref(end_flag));
  thread Cathode3(GigaIwaki, "cathode3", ref(end_flag)); 
  thread Cathode4(GigaIwaki, "cathode4", ref(end_flag));
  thread Cathode5(GigaIwaki, "cathode5", ref(end_flag));  

  while(1){
    sleep(1);
    if(end_flag)
      break;
  }

  Anode0.join();
  Anode1.join();
  Anode2.join();
  Anode3.join(); 
  Anode4.join();
  Anode5.join();
  
  Cathode0.join();
  Cathode1.join();  
  Cathode2.join();
  Cathode3.join();  
  Cathode4.join();
  Cathode5.join();  

  sleep(3);

  // DAQ disable !!!
  //  sprintf(cmd, "./daq_disable.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_LEVEL, 0x00);
  printf("DAQ disable: %d\n", res);

  // trigger disable !!!
  //  sprintf(cmd, "./trg_disable.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_LEVEL, 0x20);
  printf("Trigger disable: %d\n", res);


  // Scaler stop !!!
  //  sprintf(cmd, "./sca_stop.sh");
  //  res = system(cmd);
  res = WriteRPV(VMEGBE2_IP, RPV_PULSE, 0x02);
  printf("Scaler stop: %d\n", res);

  // Get stop time
  timer = time(NULL);                                                            
  local = localtime(&timer);                                                     
  year = local->tm_year + 1900;                                                  
  month = local->tm_mon + 1;                                                     
  day = local->tm_mday;
  hour = local->tm_hour;                                                       
  min = local->tm_min;                                                         
  sec = local->tm_sec;            
  
  printf("DAQ stop time: %d/%02d/%02d %02d:%02d:%02d\n",
	 year, month, day, hour, min, sec);
  
  // write run comment
  ofstream ofs_runcom2("runcom.dat", ios::app);  
  sprintf(comment, "STOP:%d/%02d/%02d %02d:%02d:%02d\n",
	  year, month, day, hour, min, sec);
  ofs_runcom2 << comment << endl;
  ofs_runcom2.close();

  signal(SIGINT, SIG_DFL);
}

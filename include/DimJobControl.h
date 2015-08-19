#ifndef _DimJobControl_h

#define _DimJobControl_h
#include <iostream>

#include <string.h>
#include<stdio.h>
#include "dis.hxx"
#include "dic.hxx"

//#include "DIFReadoutConstant.h"
using namespace std;
#include <sstream>
#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <sys/types.h>
#include <signal.h>
#include "json/json.h"

#include <string>
#include <stdint.h>


class DimProcessData
{

 public:
  DimProcessData(std::string json_string);
  
  enum {notcreated=0,running=1,killed=2};
  Json::Value _processInfo;
  pid_t _childPid;
  uint32_t _status;

};

class DimJobControl: public DimServer
{
public:
  DimJobControl();

  ~DimJobControl();
  virtual void commandHandler();

  void allocateCommands();
  
  void startProcess(DimProcessData* p);
  void killProcess(pid_t pid,uint32_t sig);

  std::string status();
  std::string log(pid_t pid);
  void clear();

private:
  std::string _hostname;
  std::map<pid_t,DimProcessData*> _processMap;
  DimCommand* _startCommand;
  DimCommand* _clearCommand;
  DimCommand* _killCommand;
  DimCommand* _statusCommand;
  DimCommand* _logCommand;
  DimService* _jobService;
  DimService* _logService;
};
#endif


#ifndef _DimJobInterface_h

#define _DimJobInterface_h
#include <iostream>

#include <string.h>
#include<stdio.h>
#include "dis.hxx"
#include "dic.hxx"
using namespace std;
#include <sstream>
#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include "json/json.h"

#include <string>



class DimJobInterface : public DimClient
{
public:
  DimJobInterface();

  ~DimJobInterface();
  virtual void infoHandler();
  
  void loadJSON(std::string fname);
  void List();
  void startJobs(std::string host);
  void clearHostJobs(std::string host);
  void clearAllJobs();
  void status();
  void killJob(std::string host,uint32_t pid,uint32_t sig);

private:
  Json::Value _root;
  
  std::vector<Json::Value> _processList;
  std::vector<Json::Value> _jobValue;
  char _jobbuffer[8192];
  std::vector<DimInfo*> _jobInfo;
  std::vector<std::string> _DJCNames;
};
#endif


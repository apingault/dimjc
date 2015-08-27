#ifndef _DimJobInterface_h

#define _DimJobInterface_h
#include <iostream>

#include <stdint.h>
#include <string.h>
#include<stdio.h>
#include "dis.hxx"
#include "dic.hxx"
using namespace std;
#include <sstream>
#include <map>
#include <vector>
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
  void startJob(std::string host, std::string name);
  void clearHostJobs(std::string host);
  void clearAllJobs();
  void status();
  void killJob(std::string host,uint32_t pid,uint32_t sig=9);
  void restartJob(std::string host,std::string name,uint32_t pid, uint32_t sig=9 );
  std::string processStatusList();
  std::string processJobList();
  const Json::Value &getProcessStatusValue() const;
  const Json::Value &getRoot() const;

private:

  Json::Value _root;
  std::vector<Json::Value> _processList;
  std::vector<Json::Value> _jobValue;
  Json::Value _processArray;
  char _jobbuffer[8192];
  std::vector<DimInfo*> _jobInfo;
  std::vector<std::string> _DJCNames;
};
#endif



#include "DimJobControl.h"
#include <stdint.h>
#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>  
#include <sys/param.h>  
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

#include <string.h>
#include <cstdio>
#include <cstring>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <vector>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <dirent.h>
#include <sys/types.h>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>      // std::ifstream
// #define DEBUG_PRINT_ENABLED 1  // uncomment to enable DEBUG statements
#define INFO_PRINT_ENABLED 1
#if DEBUG_PRINT_ENABLED
#define INFO_PRINT_ENABLED 1
#define DEBUG_PRINT printf
#else
#define DEBUG_PRINT(format, args...) ((void)0)
#endif
#if INFO_PRINT_ENABLED
#define INFO_PRINT printf
#else
#define INFO_PRINT(format, args...) ((void)0)
#endif

#define _jobControl_NMAX 4096
#define _jobControl_MMAX 16
#define _jobControl_MAXENV 100
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}
DimProcessData::DimProcessData(std::string json_string)
{
  Json::Reader reader;
  bool parsedFromString;
  Json::StyledWriter styledWriter;
  std::cout<<"Parsing" <<json_string<<std::endl;
  //  std::string jj=ReplaceAll(json_string, std::string("'"), std::string("\""));
  //bool parsingSuccessful = reader.parse(jj,_processInfo);
  bool parsingSuccessful = reader.parse(json_string,_processInfo);
  if (parsingSuccessful)
    {
      std::cout << styledWriter.write(_processInfo) << std::endl;
    }
  _childPid=0;
  _status=DimProcessData::notcreated;
}
std::string proc_find(uint32_t lpid) 
{
    std::stringstream s;
    s<< "/proc/"<<lpid<<"/status";

    std::ifstream infile(s.str().c_str());
    if (!infile.good())
      return std::string("DEAD");
    std::string line;
    while (std::getline(infile, line))
      {
	if (line.substr(0,6).compare("State:")==0)
	  return line.substr(6);
	//std::cout<<"PID"<<lpid<<" "<<line.substr(6)<<std::endl;
      }
    infile.close();

}

void DimJobControl::startProcess(DimProcessData* p)
{
  if (p->_status!=DimProcessData::notcreated) return;

  std::string sprog=p->_processInfo["Program"].asString();
  std::vector<std::string> sarg;
  for (uint32_t ia=0;ia<p->_processInfo["Arguments"].size();ia++)
    {
      sarg.push_back(p->_processInfo["Arguments"][ia].asString());
    }
 //std::map<std::string,std::string> menv;
  std::vector<std::string> venv;
  for (uint32_t ia=0;ia<p->_processInfo["Environnement"].size();ia++)
    {
      venv.push_back(p->_processInfo["Environnement"][ia].asString());
    }

 signal(SIGCHLD, SIG_IGN);
 // forking
 pid_t pid = fork();

 if (pid!=0) //Parent
   {
     p->_childPid=pid;
     p->_status=DimProcessData::running;
     return;
   }
 // client
  
  // we are in the child 
  //

   char executivePath[_jobControl_NMAX];
   char argv[_jobControl_MMAX][_jobControl_NMAX];  // build and initialize argv[][]
   char *pArgv[_jobControl_MMAX];
   char envp[_jobControl_MAXENV][_jobControl_NMAX];
   char* pEnvp[_jobControl_MAXENV];


   // Executive Path
   sprintf(executivePath,sprog.c_str());
 // fills arguments list

  for (int i = 0; i<_jobControl_MMAX ; i++) {                    
    for (int j = 0; j<_jobControl_NMAX ; j++) {
      argv[i][j] = (char)NULL;
    }
  }

  int i=1;
  for (std::vector<std::string>::iterator iter = sarg.begin(); iter!=sarg.end(); iter++) {
    sprintf( argv[i], "%s", (*iter).c_str());
    pArgv[i] = & argv[i][0];
    i++;
  }
  pArgv[0] = executivePath;
  pArgv[i] = NULL;


  
  // stop the watchdog
  //  watchdogEnabled_ = false;
  //  watchdogTimer_->stop();


  // brute force close 
  // xdaq only opens first 5.
  //
   close(0);
   for (int i = 3; i < 32; i++) {
     close(i);
   }
  
 // Fills environment list

  i=0;
  for (std::vector<std::string>::iterator iter = venv.begin(); iter!=venv.end(); iter++) {
    sprintf( envp[i], "%s", (*iter).c_str());
    // std::stringstream oss;
    // oss << "environment list : " << i << " : " << (*iter) << std::endl;
    // LOG4CPLUS_DEBUG(this->getApplicationLogger(), oss.str()); 
    pEnvp[i] = & envp[i][0];
    i++;
  }
  pEnvp[i] = NULL;

  // set new user id to root
  int ret=0;
  ret = setuid(0);
  if ( ret != 0 ) {
	//Let's try a second time
  	ret = setuid(0);
  	if ( ret != 0 ) {
	  INFO_PRINT("child: FATAL couldn't setuid() to %i.\n",0);
	}
  }
  
  
  // check for config file existence
  


  // open procID+log for stdout and stderr
  
  char logPath[100];
  
  pid_t mypid = getpid();                                          // get my pid to append to filename 	
  sprintf(logPath,"/tmp/dimjcPID%i.log",mypid);             // construct filename to /tmp/....
  try {
	  int tmpout = open( logPath , O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH );    // open file
	  dup2( tmpout, 1 );                                         // stdout to file
	  dup2( tmpout, 2 );                                         // stderr to file
	  close( tmpout );                                       // close unused descriptor
  }
  catch (std::exception &e) {
    INFO_PRINT("child: FATAL couldn't write log file to %s.\n", logPath);
    exit(-1);
  }
  catch (...)
    {
      INFO_PRINT("child: FATAL couldn't write log file to %s.\n", logPath);
      exit(-1);
    }


  ret = execve( executivePath, pArgv, pEnvp); 	      

  INFO_PRINT("jobControl: FATAL OOps, we came back with ret = %i , dying",ret);
  exit(-1);
}
DimJobControl::DimJobControl() 
{
  

  cout<<"Building DimJobControl"<<endl;
  std::stringstream s0;
  char hname[80];
  gethostname(hname,80);
  _hostname.assign(hname);
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/JOBSTATUS";
  _jobService = new DimService(s0.str().c_str(),"CREATED");
  //_jobService->updateService();
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/LOGSTATUS";
  _logService = new DimService(s0.str().c_str(),"CREATED");
  //_logService->updateService();
  allocateCommands();
  s0.str(std::string());
  s0<<"DimJobControl-"<<hname;

  DimServer::start(s0.str().c_str()); 
  //  cout<<"Starting DimDaqCtrl"<<endl;

}
DimJobControl::~DimJobControl()
{
  clear();
  delete _startCommand;
  delete _killCommand;
  delete _statusCommand;
  delete _logCommand;
  delete _jobService;
  delete _logService;

}
void DimJobControl::killProcess(pid_t pid,uint32_t  sig)
{
  // for (std::map<pid_t,DimProcessData*>::iterator it=_processMap.begin();it!=_processMap.end();it++)
  //   {
  //     if (it->second->_status==DimProcessData::running && pid==it->first)
  // 	{
  // 	  ::kill(it->first,sig);
  // 	  it->second->_status=DimProcessData::killed;
  // 	}

  //   }
  
  


  std::map<pid_t, DimProcessData*>::iterator itr = _processMap.begin();
  while (itr != _processMap.end()) 
    {
      if (itr->second->_status==DimProcessData::running && pid==itr->first) 
	{
	std::map<pid_t, DimProcessData*>::iterator toErase = itr;
	::kill(itr->first,sig);
	itr->second->_status=DimProcessData::killed;
	delete itr->second;
	++itr;
	_processMap.erase(toErase);
	} 
      else 
	{
	++itr;
	}
    }



}

void::DimJobControl::clear()
{
  for (std::map<pid_t,DimProcessData*>::iterator it=_processMap.begin();it!=_processMap.end();it++)
    {
      if (it->second->_status==DimProcessData::running)
	::kill(it->first,SIGKILL);
      delete it->second;
    }
  _processMap.clear();
}

std::string DimJobControl::status()
{
  //  std::stringstream s0;
  // s0.str(std::string());
  Json::FastWriter fastWriter;
  Json::Value fromScratch;
  Json::Value array;
  
  for (std::map<pid_t,DimProcessData*>::iterator it=_processMap.begin();it!=_processMap.end();it++)
    {
      Json::Value pinf;
      pinf["HOST"]=_hostname;
      pinf["PID"]=it->first;
      pinf["NAME"]=it->second->_processInfo["Name"];
      pinf["STATUS"]=proc_find(it->first);
      array.append(pinf);
    }
  fromScratch["JOBS"]=array;
 return fastWriter.write(fromScratch);
}
std::string DimJobControl::log(pid_t pid)
{
  std::stringstream s0;
  s0.str(std::string());
  s0<<"Not yet done "<<std::endl;
  return s0.str();
}
void DimJobControl::allocateCommands()
{
  std::stringstream s0;
  char hname[80];
  gethostname(hname,80);
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/CLEAR";
  _clearCommand=new DimCommand(s0.str().c_str(),"I:1",this);
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/START";
  _startCommand=new DimCommand(s0.str().c_str(),"C",this);
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/KILL";
  _killCommand=new DimCommand(s0.str().c_str(),"I:2",this);
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/STATUS";
  _statusCommand=new DimCommand(s0.str().c_str(),"I:1",this);
  s0.str(std::string());
  s0<<"/DJC/"<<hname<<"/LOG";
  _logCommand=new DimCommand(s0.str().c_str(),"I:1",this);
  
}


void DimJobControl::commandHandler()
{
  DimCommand *currCmd = getCommand();
  printf(" J'ai recu %s COMMAND  \n",currCmd->getName());
  if (currCmd==_clearCommand)
    {
      this->clear();
      _jobService->updateService((char*) this->status().c_str());
      return ;

    }

  if (currCmd==_startCommand)
    {

      std::cout<<currCmd->getString()<<std::endl;
      std::cout<<std::string(currCmd->getString())<<std::endl;
      DimProcessData* dp=new DimProcessData(std::string(currCmd->getString()));
      this->startProcess(dp);
      std::pair<pid_t, DimProcessData*> pp(dp->_childPid,dp);
      _processMap.insert(pp);
      usleep(2000);
      _jobService->updateService((char*) this->status().c_str());
      
      return;
    }

  if (currCmd==_killCommand)
    {

      int* data=(int*) currCmd->getData();
      this->killProcess(data[0],data[1]);
      usleep(2000);
      _jobService->updateService((char*) this->status().c_str());
      
      return;
    }

  if (currCmd==_statusCommand)
    {
      _jobService->updateService((char*) this->status().c_str());
      
       return;
    }
  if (currCmd==_logCommand)
    {
      _logService->updateService((char*) this->log(currCmd->getInt()).c_str());
      return ;

    }

  
  cout<<"Unknown command"<<currCmd->getName()<<" \n";
    
  return ;
}

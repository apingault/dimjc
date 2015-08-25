
#include "DimJobInterface.h"
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
#include "json/json.h"

#include <string.h>

#include <fstream>      // std::ifstream


DimJobInterface::DimJobInterface()
{
 
}
DimJobInterface::~DimJobInterface()
{
 
 
}

void DimJobInterface::loadJSON(std::string fname)
{
  // Parse the file

  Json::Reader reader;
  std::ifstream ifs (fname.c_str(), std::ifstream::in);

 // Let's parse it  
 
  bool parsedSuccess = reader.parse(ifs, 
                                   _root, 
                                   false);
  
 if(not parsedSuccess)
    {
      // Report failures and their locations 
      // in the document.
      std::cout<<"Failed to parse JSON"<<std::endl 
	  <<reader.getFormatedErrorMessages()
	       <<std::endl;
      return ;
    }

 std::vector<std::string> vjobs=_root["hosts"].getMemberNames();

 _processList.clear();
   Json::StyledWriter styledWriter;

 for (std::vector<std::string>::iterator it=vjobs.begin();it!=vjobs.end();it++)
   {
     Json::Value h=_root["hosts"][(*it)];
     std::cout<<"============"<<(*it)<<"==========="<<std::endl;
     for (uint32_t ia=0;ia<h.size();ia++)
       {
	 std::cout<<">>>"<<ia<<" >>>> Process"<<std::endl;
	 std::cout << styledWriter.write(h[ia]) << std::endl;
	 h[ia]["host"]=(*it);

	 _processList.push_back(h[ia]);
       }
   }


  // Look for DB server
  DimBrowser* dbr=new DimBrowser(); 
  char *service, *format; 
  int type;
  // Get DB service
  cout<<"On rentre dans scandns "<<endl;
  _DJCNames.clear();
  _jobInfo.clear();
  _jobValue.clear();
  dbr->getServices("/DJC/*/JOBSTATUS" ); 
 while(type = dbr->getNextService(service, format)) 
    { 
      cout << service << " -  " << format << endl; 
      std::string ss;
      ss.assign(service);
      size_t n=ss.find("/JOBSTATUS");
      cout<<ss.substr(0,n)<<endl;
      cout<<ss.substr(0,n).substr(5,n-5)<<endl;
      if(std::find(vjobs.begin(), vjobs.end(),ss.substr(0,n).substr(5,n-5))==vjobs.end()) continue;
      _DJCNames.push_back(ss.substr(0,n).substr(5,n-5));
      DimInfo* jinf=new DimInfo(service,_jobbuffer,this);
      _jobInfo.push_back(jinf);
      Json::Value jv;
      _jobValue.push_back(jv);


    } 

      delete dbr;
}

void DimJobInterface::infoHandler()
{
   DimInfo *curr = getInfo(); // get current DimInfo address 
   if (curr->getSize()==1) return;
   for (uint32_t i=0;i<_jobInfo.size();i++)
     if (curr==_jobInfo[i])
       {
	 _jobValue[i].clear();
	 Json::Reader reader;
	 std::string jsonMessage;
	 jsonMessage.assign(curr->getString());
	 bool parsingSuccessful = reader.parse(jsonMessage,_jobValue[i]);

	 // Fill the process Array
	 // std::cout<<"Fill process array "<<_jobValue[i]["JOBS"].size()<<std::endl;
	 // Json::StyledWriter styledWriter;
	 // std::cout << styledWriter.write(_jobValue[i]);
	 for (int ip=0;ip<_jobValue[i]["JOBS"].size();ip++)
	   {
	     Json::Value pin=_jobValue[i]["JOBS"][ip];
	     bool found=false;
	      for (std::vector<Json::Value>::iterator it=_processList.begin();it!=_processList.end();it++)
		{
		  //printf("host %s  compare to %s  gives %d \n",host.c_str(),(*it)["host"].asString().c_str(),host.compare((*it)["host"].asString()));
		  if ( pin["HOST"].asString().compare((*it)["host"].asString())!=0) continue;
		  if ( pin["NAME"].asString().compare((*it)["Name"].asString())!=0) continue;
		  found=true;break;
		}
	      if (!found)
		pin["DAQ"]="N";
	      else
		pin["DAQ"]="Y";
	      _processArray.append(pin);

	     
	   }
	   

       return;
     }

}
std::string DimJobInterface::processJobList()
{
   Json::FastWriter fastWriter;
   return fastWriter.write(_root);
}

std::string DimJobInterface::processStatusList()
{
   Json::FastWriter fastWriter;
   return fastWriter.write(_processArray);
}

const Json::Value &DimJobInterface::getProcessStatusValue() const
{
	return _processArray;
}

const Json::Value &DimJobInterface::getRoot() const
{
	return _root;
}

void DimJobInterface::List()
{
  // this->status();
  // usleep(500000);
  /*
  Json::StyledWriter styledWriter;
  for (uint32_t i=0;i<_jobValue.size();i++)
    {
    std::cout << styledWriter.write(_jobValue[i]);
    }
  Json::FastWriter fastWriter;
  */
  
  std::cout<<std::setw(6)<<"\e[1m"<<"PID";
  std::cout<<std::setw(15)<<"NAME";
  std::cout<<std::setw(25)<<"HOST";
  std::cout<<std::setw(20)<<"STATUS";
  std::cout<<std::setw(10)<<"In DAQ"<<"\e[0m"<<std::endl<<std::endl;
  for (int ip=0;ip<_processArray.size();ip++)
    {
      std::cout<<std::setw(6)<<_processArray[ip]["PID"].asString();
      std::cout<<std::setw(15)<<_processArray[ip]["NAME"].asString();
      std::cout<<std::setw(25)<<_processArray[ip]["HOST"].asString();
      std::cout<<std::setw(20)<<_processArray[ip]["STATUS"].asString();

      std::cout<<std::setw(10)<<_processArray[ip]["DAQ"].asString()<<std::endl;
    }
}
void DimJobInterface::restartJob(std::string host,std::string name,uint32_t pid, uint32_t sig )
{
  std::stringstream s0;
  
  s0.str(std::string());
  this->killJob(host,pid,sig);
  usleep(500000);

  // Restart the process
  Json::FastWriter fastWriter;
  for (std::vector<Json::Value>::iterator it=_processList.begin();it!=_processList.end();it++)
    {

      if ( host.compare((*it)["host"].asString())!=0) continue;
      if ( name.compare((*it)["Name"].asString())!=0) continue;
      s0.str(std::string());
      s0<<"/DJC/"<<(*it)["host"].asString()<<"/START";
      std::cout<<s0.str()<<std::endl;
      std::cout<<fastWriter.write((*it)).c_str()<<std::endl;
      DimClient::sendCommand(s0.str().c_str(),fastWriter.write((*it)).c_str());
      break;

    }

}
void DimJobInterface::startJobs(std::string host)
{
  std::stringstream s0;
  
  s0.str(std::string());

  Json::FastWriter fastWriter;
  for (std::vector<Json::Value>::iterator it=_processList.begin();it!=_processList.end();it++)
    {
      printf("host %s  compare to %s  gives %d \n",host.c_str(),(*it)["host"].asString().c_str(),host.compare((*it)["host"].asString()));
      if (host.compare("ALL")==0 || host.compare((*it)["host"].asString())==0)
	{
	  s0.str(std::string());
	  s0<<"/DJC/"<<(*it)["host"].asString()<<"/START";
	  std::cout<<s0.str()<<std::endl;
	  std::cout<<fastWriter.write((*it)).c_str()<<std::endl;
	  DimClient::sendCommand(s0.str().c_str(),fastWriter.write((*it)).c_str());
	}

    }

}
void DimJobInterface::clearHostJobs(std::string host)
{
  std::stringstream s0;
  
  s0.str(std::string());
  s0<<"/DJC/"<<host<<"/CLEAR";
  DimClient::sendCommand(s0.str().c_str(),(int) 1);
}

void DimJobInterface::clearAllJobs()
{
  for (std::vector<std::string>::iterator it=_DJCNames.begin();it!=_DJCNames.end();it++)
   {
     std::stringstream s0;

      s0<<"/DJC/"<<(*it)<<"/CLEAR";
      DimClient::sendCommand(s0.str().c_str(),(int) 1);

   }
}

void DimJobInterface::status()
{
  std::stringstream s0;
  
  s0.str(std::string());
  // std::cout<<"Clear process array "<<std::endl;
  _processArray.clear();

  for (std::vector<std::string>::iterator its=_DJCNames.begin();its!=_DJCNames.end();its++)
   {
     //std::cout<<"Looking for "<<(*its)<<std::endl;
     bool found=false;
     for (std::vector<Json::Value>::iterator it=_processList.begin();it!=_processList.end();it++)
       {
	 if ((*its).compare((*it)["host"].asString())==0)
	   {
	     found=true;break;
	   }
       }
     if (!found) continue;
     s0.str(std::string());

     s0<<"/DJC/"<<(*its)<<"/STATUS";
     DimClient::sendCommand(s0.str().c_str(),(int) 1);
   }
}

void DimJobInterface::killJob(std::string host,uint32_t pid,uint32_t sig)
{
  std::stringstream s0;
  
  s0.str(std::string());

  s0<<"/DJC/"<<host<<"/KILL";
  int data[2];
  data[0]=pid;
  data[1]=sig;
  DimClient::sendCommand(s0.str().c_str(),data,2*sizeof(int32_t));

   

}




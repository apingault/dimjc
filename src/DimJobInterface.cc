
#include "DimJobInterface.h"
#include "fileTailer.hh"

// -- std headers
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>  
#include <sys/param.h>
#include <string.h>

DimJobInterface::DimJobInterface() :
	DimTimer(),
	m_timerPeriod(0)
{
	pthread_mutex_init(&m_mutex, NULL);
}

//-------------------------------------------------------------------------------------------------

DimJobInterface::~DimJobInterface()
{
	pthread_mutex_destroy(&m_mutex);
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::loadJSON(const std::string &fileName)
{
	this->clear();

	// Parse the file
	Json::Reader reader;
	std::ifstream ifs (fileName.c_str(), std::ifstream::in);

	// Let's parse it
	bool parsedSuccess = reader.parse(ifs, m_root, false);

	if(!parsedSuccess)
	{
		// Report failures and their locations
		// in the document.
		std::cout << "Failed to parse JSON"
				  << std::endl
				  << reader.getFormatedErrorMessages()
				  << std::endl;

		return ;
	}

	Json::Value &hostsValue = m_root["HOSTS"];
	Json::Value &vars = m_root["VARS"];
	std::vector<std::string> hosts = hostsValue.getMemberNames();
	Json::StyledWriter styledWriter;

	this->performVariablesReplacement( hostsValue , vars );

	// load process entries
	for (std::vector<std::string>::iterator iter = hosts.begin(), endIter = hosts.end() ;
			endIter != iter ; ++iter)
	{
		Json::Value h = hostsValue[(*iter)];
		std::cout<<"============"<<(*iter)<<"==========="<<std::endl;



		for (uint32_t ia=0 ; ia<h.size() ; ia++)
		{
			std::cout << ">>> " << ia << " >>>> Process" << std::endl;
			std::cout << styledWriter.write(h[ia]) << std::endl;
			h[ia]["HOST"]=(*iter);

			m_processList.push_back(h[ia]);
		}
	}


	// Look for DB server
	DimBrowser* dbr = new DimBrowser();
	char *service, *format;
	int type;

	// Get DB service
	std::cout<< "Scanning dim DNS" << std::endl;
	dbr->getServices("/DJC/*/JOBSTATUS" );

	while(type = dbr->getNextService(service, format))
	{
		std::cout << service << " -  " << format << std::endl;
		std::string ss = service;

		size_t n = ss.find("/JOBSTATUS");
		std::string host = ss.substr(0,n).substr(5, n-5);

		if(std::find(hosts.begin(), hosts.end(), host) == hosts.end())
			continue;

		m_djcNames.push_back(host);
		std::cout << "Host : " << host << std::endl;

		DimInfo *pJobInfo = new DimInfo(service, m_jobbuffer, this);
		m_jobInfo.push_back(pJobInfo);

		Json::Value jobValue;
		m_jobValue.push_back(jobValue);
	}

	delete dbr;
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::infoHandler()
{
	DimInfo *pInfo = getInfo(); // get pInfo DimInfo address

	if (pInfo->getSize() == 1)
		return;

	for (uint32_t i=0 ; i<m_jobInfo.size() ; i++)
	{
		if (pInfo == m_jobInfo[i])
		{
			m_jobValue[i].clear();

			std::string ss = m_jobInfo[i]->getName();
			size_t n = ss.find("/JOBSTATUS");
			std::string hostName = ss.substr(0,n).substr(5, n-5);

			Json::Reader reader;
			std::string jsonMessage = pInfo->getString();

			bool parsingSuccessful = reader.parse(jsonMessage, m_jobValue[i]);

			// Fill the process array
			for (int ip=0 ; ip<m_jobValue[i]["JOBS"].size() ; ip++)
			{
				Json::Value pin = m_jobValue[i]["JOBS"][ip];
				bool found = false;

				for (std::vector<Json::Value>::iterator iter = m_processList.begin(), endIter = m_processList.end() ;
						endIter != iter ; ++iter)
				{
					if ( pin["HOST"].asString().compare((*iter)["HOST"].asString()) != 0)
						continue;

					if ( pin["NAME"].asString().compare((*iter)["NAME"].asString()) != 0)
						continue;

					found=true;
					break;
				}

				if (!found)
					pin["DAQ"] = "N";
				else
					pin["DAQ"] = "Y";

				m_processArray.append(pin);
			}

			this->statusReceived(hostName);

			return;
		}
	}
}

//-------------------------------------------------------------------------------------------------

std::string DimJobInterface::processJobList() const
{
   Json::FastWriter fastWriter;
   return fastWriter.write(m_root);
}

//-------------------------------------------------------------------------------------------------

std::string DimJobInterface::processStatusList() const
{
   Json::FastWriter fastWriter;
   return fastWriter.write(m_processArray);
}

//-------------------------------------------------------------------------------------------------

Json::Value DimJobInterface::processStatus(const std::string &hostName) const
{
	Json::Value processStatus;

	for (uint32_t i=0 ; i<m_jobInfo.size() ; i++)
	{
		std::string ss = m_jobInfo[i]->getName();
		size_t n = ss.find("/JOBSTATUS");
		std::string hname = ss.substr(0,n).substr(5, n-5);

		if(hname == hostName)
		{
			processStatus = m_jobValue[i];
			break;
		}
	}

	return processStatus;
}

//-------------------------------------------------------------------------------------------------

const Json::Value &DimJobInterface::getProcessStatusValue() const
{
	return m_processArray;
}

//-------------------------------------------------------------------------------------------------

const Json::Value &DimJobInterface::getRoot() const
{
	return m_root;
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::list()
{
	std::cout << std::setw(6)  << "\033[1m" << "PID";
	std::cout << std::setw(15) << "NAME";
	std::cout << std::setw(25) << "HOST";
	std::cout << std::setw(20) << "STATUS";
	std::cout << std::setw(10) << "In DAQ" << "\033[0m" << std::endl << std::endl;

	for (int ip=0 ; ip<m_processArray.size() ; ip++)
	{
		std::cout << std::setw(6)  << m_processArray[ip]["PID"].asUInt();
		std::cout << std::setw(15) << m_processArray[ip]["NAME"].asString();
		std::cout << std::setw(25) << m_processArray[ip]["HOST"].asString();
		std::cout << std::setw(20) << m_processArray[ip]["STATUS"].asString();
		std::cout << std::setw(10) << m_processArray[ip]["DAQ"].asString() << std::endl;
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::restartJob(const std::string &hostName, const std::string &jobName,
		uint32_t pid, uint32_t sig)
{
	this->killJob(hostName, pid, sig);
	usleep(500000);

	std::stringstream s0;
	Json::FastWriter fastWriter;

	// Restart the process
	for (std::vector<Json::Value>::iterator iter = m_processList.begin(), endIter = m_processList.end() ;
			endIter != iter ; ++iter)
	{
		if(hostName.compare((*iter)["HOST"].asString()) != 0)
			continue;

		if(jobName.compare((*iter)["NAME"].asString()) != 0)
			continue;

		s0.str("");
		s0 << "/DJC/" << (*iter)["HOST"].asString() << "/START";

		std::cout << s0.str() << std::endl;
		std::cout << fastWriter.write((*iter)).c_str() << std::endl;

		DimClient::sendCommand(s0.str().c_str(), fastWriter.write((*iter)).c_str());

		break;
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::startJob(const std::string &hostName, const std::string &jobName)
{
	std::stringstream s0;
	Json::FastWriter fastWriter;

	// Start the process
	for (std::vector<Json::Value>::iterator iter = m_processList.begin(), endIter = m_processList.end() ;
			endIter != iter ; ++iter)
	{
		if(hostName.compare((*iter)["HOST"].asString()) != 0)
			continue;

		if(jobName.compare((*iter)["NAME"].asString()) != 0)
			continue;

		s0.str("");
		s0 << "/DJC/" << (*iter)["HOST"].asString() << "/START";

		std::cout << s0.str() << std::endl;
		std::cout << fastWriter.write((*iter)).c_str() << std::endl;

		DimClient::sendCommand(s0.str().c_str(), fastWriter.write((*iter)).c_str());

		break;
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::startJobs(const std::string &hostName)
{
  std::stringstream s0;
  Json::FastWriter fastWriter;

	for (std::vector<Json::Value>::iterator iter = m_processList.begin(), endIter = m_processList.end() ;
			endIter != iter ; ++iter)
	{
		if (hostName.compare("ALL")!=0 && hostName.compare((*iter)["HOST"].asString()) != 0)
			continue;

		s0.str("");
		s0 << "/DJC/" << (*iter)["HOST"].asString() << "/START";

		std::cout << s0.str() << std::endl;
		std::cout << fastWriter.write((*iter)).c_str() << std::endl;

		DimClient::sendCommand(s0.str().c_str(), fastWriter.write((*iter)).c_str());
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::clearHostJobs(const std::string &hostName)
{
	std::stringstream s0;
	s0 << "/DJC/" << hostName <<"/CLEAR";

	DimClient::sendCommand(s0.str().c_str(), (int) 1);
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::clearAllJobs()
{
	for (std::vector<std::string>::iterator iter = m_djcNames.begin(), endIter = m_djcNames.end() ;
			endIter != iter ; ++iter)
	{
		std::stringstream s0;
		s0 << "/DJC/" << (*iter) << "/CLEAR";

		DimClient::sendCommand(s0.str().c_str(), (int) 1);
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::status()
{
	pthread_mutex_lock(&m_mutex);

	std::stringstream s0;
	m_processArray.clear();

	for (std::vector<std::string>::iterator djcIter = m_djcNames.begin(), djcEndIter = m_djcNames.end() ;
			djcEndIter != djcIter ; ++djcIter)
	{
		bool found = false;

		for (std::vector<Json::Value>::iterator iter = m_processList.begin(), endIter = m_processList.end() ;
				endIter != iter ; ++iter)
		{
			if ((*djcIter).compare((*iter)["HOST"].asString()) == 0)
			{
				found = true;
				break;
			}
		}

		if(!found)
			continue;

		s0.str("");
		s0 << "/DJC/" << (*djcIter) << "/STATUS";

		DimClient::sendCommand(s0.str().c_str(), (int) 1);
	}

	pthread_mutex_unlock(&m_mutex);
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::killJob(const std::string &hostName, uint32_t pid, uint32_t sig)
{
	std::stringstream s0;
	s0 << "/DJC/" << hostName << "/KILL";

	int32_t data[2];
	data[0] = pid;
	data[1] = sig;

	DimClient::sendCommand(s0.str().c_str(), data, 2*sizeof(int32_t));
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::startTimer(int nSeconds)
{
	m_timerPeriod = nSeconds;
	DimTimer::start(nSeconds);
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::stopTimer()
{
	DimTimer::stop();
}

//-------------------------------------------------------------------------------------------------

std::string DimJobInterface::queryLogFile(const std::string &hostName, pid_t pid, const unsigned int nLines)
{
	std::stringstream ss;
	ss << "/DJC/" << hostName << "/LOGRPC";

	DimRpcInfo info((char*) ss.str().c_str(), (char*)"");

	int data[2];
	data[0] = pid;
	data[1] = nLines;

	std::cout << "Sending : pid " << data[0] << " , nLines : " << data[1] << std::endl;

	info.setData((void*)data, 2*sizeof(int));

	// wait for server answer
	char *contents( info.getString() );

	if( contents )
		return std::string( contents );
	else
		return std::string();
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::clear()
{
	m_root.clear();
	m_processList.clear();
	m_jobValue.clear();
	m_processArray.clear();

	for(unsigned int i=0 ; i<m_jobInfo.size() ; i++)
		delete m_jobInfo.at(i);

	m_jobInfo.clear();
	m_djcNames.clear();
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::performVariablesReplacement( Json::Value &hostsValue, const Json::Value &vars )
{
	std::vector<std::string> hosts = hostsValue.getMemberNames();

	// load process entries
	for (std::vector<std::string>::iterator iter = hosts.begin(), endIter = hosts.end() ;
			endIter != iter ; ++iter)
	{
		Json::Value &h = hostsValue[(*iter)];

		std::map<std::string,std::string> parameters;
		this->buildHostVariableMap( *iter , vars , parameters );

		if( parameters.empty() )
			continue;

		for (uint32_t ia=0 ; ia<h.size() ; ia++)
		{
			Json::Value &name = h[ia]["NAME"];
			Json::Value &program = h[ia]["PROGRAM"];
			Json::Value &args = h[ia]["ARGS"];
			Json::Value &envs = h[ia]["ENV"];

			std::string nameStr = name.asString();
			this->replace(nameStr, parameters);
			h[ia]["NAME"] = Json::Value(nameStr);

			std::string programStr = program.asString();
			this->replace(programStr, parameters);
			h[ia]["PROGRAM"] = Json::Value(programStr);

			for(unsigned int a=0 ; a<args.size() ; a++)
			{
				std::string argStr = args[a].asString();
				this->replace(argStr, parameters);
				h[ia]["ARGS"][a] = Json::Value(argStr);
			}

			for(unsigned int e=0 ; e<envs.size() ; e++)
			{
				std::string envStr = envs[e].asString();
				this->replace(envStr, parameters);
				h[ia]["ENV"][e] = Json::Value(envStr);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::buildHostVariableMap( const std::string &hostName, const Json::Value &vars , std::map<std::string,std::string> &parameters)
{
	if( ! vars["GLOBAL"].empty() )
	{
		std::vector<std::string> globals = vars["GLOBAL"].getMemberNames();

		for( std::vector<std::string>::iterator iter = globals.begin(), endIter = globals.end() ;
				endIter != iter ; ++iter)
		{
			std::string var = vars["GLOBAL"][(*iter)].asString();
			this->replace( var , parameters );
			parameters[ *iter ] = var;
		}
	}

	if( ! vars[hostName].empty() )
	{
		std::vector<std::string> varNames = vars[hostName].getMemberNames();

		for( std::vector<std::string>::iterator iter = varNames.begin(), endIter = varNames.end() ;
				endIter != iter ; ++iter)
		{
			std::string var = vars[hostName][(*iter)].asString();
			this->replace( var , parameters );
			parameters[ *iter ] = var;
		}
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::replace( std::string &targetString, const std::map<std::string,std::string> &variables)
{
	for(std::map<std::string, std::string>::const_iterator iter = variables.begin(), endIter = variables.end() ;
			endIter != iter ; ++iter)
	{
		std::string variable = "${" + iter->first + "}";
		size_t pos = targetString.find(variable);

		if(pos != std::string::npos)
			targetString.replace(pos, variable.size(), iter->second);
	}
}

//-------------------------------------------------------------------------------------------------

void DimJobInterface::timerHandler()
{
	this->status();

	// restart the timer
	this->startTimer(m_timerPeriod);
}


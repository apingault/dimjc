
#include "DimJobControl.h"
#include "fileTailer.hh"

// -- std headers
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif


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
#define _jobControl_MMAX 100
#define _jobControl_MAXENV 100

#define TAILER_MAX_BUFFER_SIZE 524288 // = 1024*512

//-------------------------------------------------------------------------------------------------

class LogRpc : public DimRpc
{
public:
	LogRpc(char *name, char *format_in, char *format_out);
	void rpcHandler();
	std::string log(pid_t pid, uint32_t nLines);
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

LogRpc::LogRpc(char *name, char *format_in, char *format_out) :
	DimRpc(name, format_in, format_out)
{
	/* nop */
}

void LogRpc::rpcHandler()
{
	int *data = (int *) this->getData();

	if(!data)
	{
		setData((char*)"");
		return;
	}

	const pid_t pid(data[0]);
	const unsigned int nLines(data[1]);

	std::string contents(this->log(pid, nLines));

	setData( (char*) contents.c_str() );
}

//-------------------------------------------------------------------------------------------------

std::string LogRpc::log(pid_t pid, uint32_t nLines)
{
	std::stringstream fileName;
	fileName << "/tmp/dimjcPID" << pid << ".log";

	if(nLines > 0)
	{
		FileTailer tailer(TAILER_MAX_BUFFER_SIZE);
		char tailBuffer[TAILER_MAX_BUFFER_SIZE];
		tailer.tail(fileName.str(), nLines, tailBuffer);

		return std::string(tailBuffer);
	}
	else
	{
		std::ifstream in(fileName.str().c_str(), std::ios::in);

		if ( in )
		{
			std::string contents;

			in.seekg(0, std::ios::end);
			contents.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();

			return contents;
		}
		else
			return "File '" + fileName.str() + "' not found !";
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

std::string &trimString(std::string &str)
{
	if (str.empty())
		return str;

	while (str.at(0) == ' ')
		str.erase(0, 1);

	if (str.empty())
		return str;

	while (str.at(str.size() - 1) == ' ')
		str.erase(str.size() - 2, 1);

	return str;
}

//-------------------------------------------------------------------------------------------------

std::string processStatus(uint32_t lpid)
{

#ifdef __APPLE__
	/*
	 * /proc system not implemented on MacOSX
	 * Use sysctl to get informations on the processus
	 */
	struct kinfo_proc kp;
	size_t length = sizeof(kp);

	if (length == 0)
		return std::string("DimJobControl.cc: syctl::kinfo_proc is empty");

	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)(lpid) };

	if (sysctl(mib, 4, &kp, &length, NULL, 0) < 0)
		return std::string("DimJobControl.cc: Unknown Value for sysctl");

	int processStatus = kp.kp_proc.p_stat;
	if (processStatus <= 0) {
		// No PID Status found
		return std::string("X (dead)");
	}

	// Return the /proc/PID/status state for backward compatibilty & lisibility
	if ( (processStatus & SIDL) == SIDL )
		return std::string("D (disk sleep)");
	if ( (processStatus & SRUN) == SRUN )
		return std::string("R (running)");
	if ( (processStatus & SSLEEP) == SSLEEP )
		return std::string("S (sleeping)");
	if ( (processStatus & SSTOP) == SSTOP )
		return std::string("T (stopped)");
	if ( (processStatus & SZOMB) == SZOMB )
		return std::string("Z (zombie)");

	return std::string("X (dead)");

#else
	std::stringstream s;
	s << "/proc/" << lpid << "/status";

	std::ifstream infile(s.str().c_str());

	if (!infile.good())
		return std::string("X (dead)");

	std::string line;

	while (std::getline(infile, line))
	{
		if (line.substr(0, 6).compare("State:") == 0)
		{
			std::string state = line.substr(6);
			return trimString(state);
		}
	}

	infile.close();

	return std::string("X (dead)");
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DimProcessData::DimProcessData(const std::string &jsonString)
{
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(jsonString, m_processInfo);

	if (parsingSuccessful)
	{
		Json::StyledWriter styledWriter;
		std::cout << styledWriter.write(m_processInfo) << std::endl;
	}

	m_childPid = 0;
	m_status = NOT_CREATED;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DimJobControl::DimJobControl()
{
	std::cout << "Building DimJobControl" << std::endl;

	char hname[80];
	gethostname(hname, 80);
	m_hostname = hname;

	std::stringstream s0;

	s0 << "/DJCDQM/" << m_hostname << "/JOBSTATUS";
	m_pJobService = new DimService((char *) s0.str().c_str(), (char*) "CREATED");

	s0.str("");
	s0 << "/DJCDQM/" << m_hostname << "/LOGSTATUS";
	m_pLogService = new DimService((char*) s0.str().c_str(), (char*) "CREATED");

	this->allocateCommands();

	s0.str("");
	s0 << "/DJCDQM/" << m_hostname << "/LOGRPC";
	m_pLogRpc = new LogRpc((char*) s0.str().c_str(), "I:2", "C");

	s0.str("");
	s0 << "DimJobControl-" << m_hostname;

	DimServer::start((char*) s0.str().c_str());
}

//-------------------------------------------------------------------------------------------------

DimJobControl::~DimJobControl()
{
	clear();

	delete m_pLogRpc;
	delete m_pClearCommand;
	delete m_pStartCommand;
	delete m_pKillCommand;
	delete m_pStatusCommand;
	delete m_pLogCommand;
	delete m_pJobService;
	delete m_pLogService;
}

//-------------------------------------------------------------------------------------------------

void DimJobControl::killProcess(pid_t pid, uint32_t sig)
{
	PidToProcessMap::iterator iter = m_processMap.begin();

	while (iter != m_processMap.end())
	{
		if (iter->second->m_status == DimProcessData::RUNNING && pid == iter->first)
		{
			PidToProcessMap::iterator toErase = iter;

			::kill(iter->first, sig);

			iter->second->m_status = DimProcessData::KILLED;
			delete iter->second;

			++iter;
			m_processMap.erase(toErase);

			break;
		}
		else
			++iter;
	}
}

//-------------------------------------------------------------------------------------------------

void::DimJobControl::clear()
{
	for (PidToProcessMap::iterator iter = m_processMap.begin(), endIter = m_processMap.end() ;
	     endIter != iter ; ++iter)
	{
		if (iter->second->m_status == DimProcessData::RUNNING)
			::kill(iter->first, SIGKILL);

		delete iter->second;
	}
	m_processMap.clear();
}

//-------------------------------------------------------------------------------------------------

std::string DimJobControl::status()
{
	Json::FastWriter fastWriter;
	Json::Value fromScratch;
	Json::Value array;

	for (PidToProcessMap::iterator iter = m_processMap.begin(), endIter = m_processMap.end() ;
	     endIter != iter ; ++iter)
	{
		Json::Value pinf;

		pinf["HOST"] = m_hostname;
		pinf["PID"] = iter->first;
		pinf["NAME"] = iter->second->m_processInfo["NAME"];
		pinf["STATUS"] = processStatus(iter->first);

		array.append(pinf);
	}

	fromScratch["JOBS"] = array;

	return fastWriter.write(fromScratch);
}

//-------------------------------------------------------------------------------------------------

std::string DimJobControl::log(pid_t pid)
{
//	// TODO to implement this function
	std::stringstream s0;
	s0.str(std::string());
	s0 << "Not yet done " << std::endl;
	return s0.str();

//	std::stringstream fileName;
//	fileName << "/tmp/dimjcPID" << pid << ".log";
//
//	std::ifstream in(filename.str().c_str(), std::ios::in);
//
//	if( in )
//	{
//		std::string contents;
//
//		in.seekg(0, std::ios::end);
//		contents.resize(in.tellg());
//		in.seekg(0, std::ios::beg);
//		in.read(&contents[0], contents.size());
//		in.close();
//
//		return contents;
//	}
//	else
//		return "File '" + fileName.str() + "' not found !";
}

//-------------------------------------------------------------------------------------------------

void DimJobControl::allocateCommands()
{
	std::stringstream s0;

	s0 << "/DJCDQM/" << m_hostname << "/CLEAR";
	m_pClearCommand = new DimCommand(s0.str().c_str(), "I:1", this);

	s0.str("");
	s0 << "/DJCDQM/" << m_hostname << "/START";
	m_pStartCommand = new DimCommand(s0.str().c_str(), "C", this);

	s0.str("");
	s0 << "/DJCDQM/" << m_hostname << "/KILL";
	m_pKillCommand = new DimCommand(s0.str().c_str(), "I:2", this);

	s0.str("");
	s0 << "/DJCDQM/" << m_hostname << "/STATUS";
	m_pStatusCommand = new DimCommand(s0.str().c_str(), "I:1", this);

	s0.str("");
	s0 << "/DJCDQM/" << m_hostname << "/LOG";
	m_pLogCommand = new DimCommand(s0.str().c_str(), "I:1", this);
}

//-------------------------------------------------------------------------------------------------

void DimJobControl::startProcess(DimProcessData* pProcessData)
{
	if (pProcessData->m_status != DimProcessData::NOT_CREATED)
		return;

	std::string programName = pProcessData->m_processInfo["PROGRAM"].asString();
	std::vector<std::string> arguments;
	std::vector<std::string> environmentVars;

	for (uint32_t ia = 0 ; ia < pProcessData->m_processInfo["ARGS"].size() ; ia++)
		arguments.push_back(pProcessData->m_processInfo["ARGS"][ia].asString());

	for (uint32_t ia = 0; ia < pProcessData->m_processInfo["ENV"].size(); ia++)
		environmentVars.push_back(pProcessData->m_processInfo["ENV"][ia].asString());

	signal(SIGCHLD, SIG_IGN);

	// forking
	pid_t pid = fork();

	// parent case
	if (pid != 0)
	{
		pProcessData->m_childPid = pid;
		pProcessData->m_status = DimProcessData::RUNNING;
		return;
	}

	// child case
	char executivePath[_jobControl_NMAX];
	char argv[_jobControl_MMAX][_jobControl_NMAX];  // build and initialize argv[][]
	char *pArgv[_jobControl_MMAX];
	char envp[_jobControl_MAXENV][_jobControl_NMAX];
	char* pEnvp[_jobControl_MAXENV];


	// Executive Path
	sprintf(executivePath, programName.c_str());

	// fills arguments list
	for (int i = 0; i < _jobControl_MMAX ; i++) {
		for (int j = 0; j < _jobControl_NMAX ; j++) {
			argv[i][j] = (char)NULL;
		}
	}

	int i = 1;

	for (std::vector<std::string>::const_iterator iter = arguments.begin(), endIter = arguments.end() ;
	     iter != endIter ; ++iter)
	{
		sprintf( argv[i], "%s", (*iter).c_str());
		pArgv[i] = & argv[i][0];
		i++;
	}

	pArgv[0] = executivePath;
	pArgv[i] = NULL;

	// brute force close
	// xdaq only opens first 5.
	close(0);
	for (int i = 3; i < 32; i++)
		close(i);

	i = 0;

	// Fills environment list
	for (std::vector<std::string>::const_iterator iter = environmentVars.begin(), endIter = environmentVars.end() ;
	     endIter != iter ; ++iter)
	{
		sprintf( envp[i], "%s", (*iter).c_str());
		pEnvp[i] = & envp[i][0];
		i++;
	}

	pEnvp[i] = NULL;

	// set new user id to root
	int ret = 0;
	ret = setuid(0);

	if ( ret != 0 )
	{
		//Let's try a second time
		ret = setuid(0);

		if ( ret != 0 )
			INFO_PRINT("child: FATAL couldn't setuid() to %i.\n", 0);
	}


	// open procID+log for stdout and stderr
	char logPath[100];

	pid_t mypid = getpid(); // get my pid to append to filename
	sprintf(logPath, "/tmp/dimjcPID%i.log", mypid);           // construct filename to /tmp/....

	try
	{
		int tmpout = open( logPath , O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH ); // open file
		dup2( tmpout, 1 );                                         // stdout to file
		dup2( tmpout, 2 );                                         // stderr to file
		close( tmpout );                                       // close unused descriptor
	}
	catch (std::exception &e)
	{
		INFO_PRINT("child: FATAL couldn't write log file to %s.\n", logPath);
		exit(-1);
	}
	catch (...)
	{
		INFO_PRINT("child: FATAL couldn't write log file to %s.\n", logPath);
		exit(-1);
	}

	ret = execve( executivePath, pArgv, pEnvp);

	INFO_PRINT("jobControl: FATAL OOps, we came back with ret = %i , errno = %i , dying", ret, errno);
	exit(-1);
}

//-------------------------------------------------------------------------------------------------

void DimJobControl::commandHandler()
{
	DimCommand *pCommand = getCommand();
	std::cout << "J'ai recu " << pCommand->getName() << " COMMAND" << std::endl;

	if (pCommand == m_pClearCommand)
	{
		this->clear();
		m_pJobService->updateService((char*) this->status().c_str());
		return;
	}

	if (pCommand == m_pStartCommand)
	{
		std::cout << pCommand->getString() << std::endl;
		std::string jsonString = pCommand->getString();

		// create, start and register the process
		DimProcessData *pProcessData = new DimProcessData(jsonString);
		this->startProcess(pProcessData);
		m_processMap.insert(PidToProcessMap::value_type(pProcessData->m_childPid, pProcessData));

		usleep(2000);

		m_pJobService->updateService((char*) this->status().c_str());

		return;
	}

	if (pCommand == m_pKillCommand)
	{
		int* data = (int*) pCommand->getData();

		if (!data)
			return;

		pid_t pid = data[0];
		uint32_t sig = data[1];

		this->killProcess(pid, sig);

		usleep(2000);

		m_pJobService->updateService((char*) this->status().c_str());

		return;
	}

	if (pCommand == m_pStatusCommand)
	{
		m_pJobService->updateService((char*) this->status().c_str());

		return;
	}

	if (pCommand == m_pLogCommand)
	{
		m_pLogService->updateService((char*) this->log(pCommand->getInt()).c_str());

		return ;
	}

	std::cout << "Unknown command " << pCommand->getName() << std::endl;
}

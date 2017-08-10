#ifndef _DimDQMJobControl_h
#define _DimDQMJobControl_h

// -- std headers
#include <stdint.h>
#include <sys/types.h>
#include <map>
#include <string>

// -- dim headers
#include <dis.hxx>
#include <dic.hxx>

// -- json headers
#include "json/json.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

/**
 *  @brief  DimProcessData class
 */
class DimProcessData
{
public:
	/**
	 *  @brief  Status enum
	 */
	enum Status
	{
		NOT_CREATED = 0,
		RUNNING = 1,
		KILLED = 2
	};

	/**
	 *  @brief  Constructor. Construct a process structure from a json string
	 *          that contains all needed infos on a process
	 *
	 *  @param  jsonString the jsonString to initialize the process
	 */
	DimProcessData(const std::string &jsonString);

	Json::Value    m_processInfo;     ///< The json process value
	pid_t          m_childPid;        ///< The process pid
	Status         m_status;          ///< The process status
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

/**
 *  @brief  DimDQMJobControl class
 */
class DimDQMJobControl: public DimServer
{
public:
	/**
	 *  @brief  Constructor.
	 *          Allocate commands and services
	 */
	DimDQMJobControl();

	/**
	 *  @brief  Destructor
	 */
	virtual ~DimDQMJobControl();

	/**
	 *  @brief  Kill all processes
	 */
	virtual void clear();

	/**
	 *  @brief  Start a process and register it in the jobs control
	 *
	 *  @param  pDimProcessData the process to start
	 */
	void startProcess(DimProcessData* pDimProcessData);

	/**
	 *  @brief  Kill a registered process
	 *
	 *  @param  pid the process pid
	 *  @param  sig the signal to send to the process
	 */
	void killProcess(pid_t pid, uint32_t sig);

protected:
	/**
	 *  @brief  Dim command handler.
	 *          Handles commands from clients
	 */
	virtual void commandHandler();

	/**
	 *  @brief  Allocate commands
	 */
	virtual void allocateCommands();

private:
	/**
	 *  @brief  Get all the process status
	 *
	 *  @return  The process status
	 */
	std::string status();

	/**
	 *  @brief  Get the log of specific process
	 *
	 *  @param  pid the process pid
	 *
	 *  @return  the process log as string
	 */
	std::string log(pid_t pid);

protected:

	typedef std::map<pid_t,DimProcessData*> PidToProcessMap;

	std::string        m_hostname;         ///< The host name on which the job control is running
	PidToProcessMap    m_processMap;       ///< The handled process map

	DimCommand*        m_pStartCommand;    ///< The start process command
	DimCommand*        m_pClearCommand;    ///< The clear command for all processes
	DimCommand*        m_pKillCommand;     ///< The kill command for a specific process
	DimCommand*        m_pStatusCommand;   ///< The status command for all processes
	DimCommand*        m_pLogCommand;      ///< The log command for a specific process
	DimService*        m_pJobService;      ///< The jobs list service for all processes
	DimService*        m_pLogService;      ///< The log service for a specific process
	DimRpc*            m_pLogRpc;          ///< The rpc to query log for a specific pid
};


#endif


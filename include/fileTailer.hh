/* @author Mirabito Laurent
 * @copyright CNRS , IPNL
 */

#ifndef _FILETAILER_H
#define _FILETAILER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>


/**
 *  @brief  FileTailer class
 */
class FileTailer
{
public:
	/**
	 *  @brief  Constructor
	 *
	 *  @param  maxbuff the max buffer size to tail
	 */
	FileTailer(uint32_t maxbuff);

	/**
	 *  @brief  Tail the file with a maximum number of lines
	 */
	void tail(const std::string &fName, uint32_t nLines, char *pBuffer);

	/**
	 *  @brief  Tail the file with a maximum number of lines
	 */
	void tail(FILE *f, uint32_t nLines, char *pBuffer);

private:
	int findTail(char *lines[][2], int nlines, char buff[], int maxbuff);
	int fileTail(FILE* s,char *lines[][2], int nlines, char buff[], int maxbuff);
	void shift(char *lines[][2], int nlines);
	bool testForRoom(char *lines[][2], int index, char *buffp);

private:
	uint32_t _MAXBUFF;
};  
#endif 

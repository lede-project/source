#include "stdafx.h"

//helpful URLs:
// http://www.networksorcery.com/enp/protocol/dns.htm
// http://www.freesoft.org/CIE/RFC/1035/43.htm

//application name and version
#define APP_NAME "DNS2SOCKS V1.8"
//first output line in console
#define APP_STRING "\n" APP_NAME " (free software, use parameter /? to display help)\n"
//log file header line
#define LOG_HEADER APP_NAME " log opened"
//date/time format string for ISO 8601 format, but leave away the T delimiter as it's bad to read
#define DATE_TIME_FORMAT "%u-%02u-%02u %02u:%02u:%02u   "
//127.0.0.1 for default SOCKS5 server
#define DEFAULT_SOCKS_SERVER "127.0.0.1"
//9050 for default SOCKS5 port
#define DEFAULT_SOCKS_PORT "9050"
//213.73.91.35 for default DNS server supporting TCP (dnscache.berlin.ccc.de)
#define DEFAULT_DNS_SERVER "213.73.91.35"
//127.0.0.1 for local IP address for listening
#define DEFAULT_LISTEN_IP "127.0.0.1"
//53 for default DNS port
#define DEFAULT_DNS_PORT "53"

//defines for OutputToLog (bits)
#define OUTPUT_LINE_BREAK (1)
#define OUTPUT_DATE_TIME (2)
#define OUTPUT_CONSOLE (4)
#define OUTPUT_ALL (OUTPUT_LINE_BREAK|OUTPUT_DATE_TIME|OUTPUT_CONSOLE)

union UClient
{
	struct sockaddr_storage sAddr;	//address of requesting client for UDP
	SOCKET hSock;					//socket handle for TCP
};

//entry for DNS request and answer (cache entry)
struct SEntry
{
	struct SEntry* psNext;			//next list entry or NULL
	uint16_t* u16aAnswer;			//pointer to answer or NULL
	time_t iTime;					//time when the answer was deliviered last time
	union UClient client;			//information on how to send response back to client
	unsigned char uAddrLen;			//length of used part of sAddr for UDP, sizeof(SOCKET) for TCP
	uint16_t u16aRequest[1];		//extended dynamically at malloc for "struct SEntry" (use "uint16_t" to ensure according alignment, first element contains length in big endian format)
};

static struct SEntry* g_psFirst=NULL;			//list of DNS requests and answers (cache)
static unsigned int g_uCacheCount=0;			//amout of entries in list g_psFirst
static int g_bCacheEnabled=1;					//!=0 when cache is enabled
static struct sockaddr_storage g_sDnsSrvAddr;	//DNS server supporting TCP
static struct sockaddr_storage g_sSocksAddr;	//SOCKS5 server
static CRITICAL_SECTION g_sCritSect;			//to protect the list g_psFirst and g_uCacheCount
static SOCKET g_hSockUdp;						//UDP socket
static unsigned char* g_uaUsrPwd=NULL;			//authentication package for SOCKS
static int g_iUsrPwdLen;						//length of g_caUsrPwd

//OS specific functionality
#ifdef _WIN32

//for Windows we can create an own console window, so here we use some OS specific functions;
//also for the file access we use the WIN32 API - we could use the C API as we do for the other OSes
//but as we need to take special care about the line break (\r\n) anyway and WIN32 should be a little
//bit faster, we use WIN32

static HANDLE g_hConsole=NULL;					//handle for console output
static HANDLE g_hLogFile=INVALID_HANDLE_VALUE;	//handle for log file

//passes a string and its length to a function
#define STRING_AND_LEN(szString) szString, sizeof(szString)-1

static char* GetSysError(int iErrNo)
{
	char* szBuffer;
	size_t uLen;

	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, iErrNo, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&szBuffer, 0, NULL))
	{
		szBuffer=(char*)LocalAlloc(LMEM_FIXED, 14);
		memcpy(szBuffer, "Unknown error", 14);
	}
	else
	{
		//remove line breaks at the end
		uLen=strlen(szBuffer);
		while(uLen>0 && (szBuffer[uLen-1]=='\n' || szBuffer[uLen-1]=='\r'))
			szBuffer[--uLen]='\0';
	}
	return szBuffer;
}

static void FreeSysError(char* szString)
{
	LocalFree(szString);
}

static void OutputToLog(unsigned int uOutputSettingBits, const char* szFormatString, ...)
{
	va_list pArgs;
	int iLenPrefix;
	int iLen;
	DWORD uDummy;
	char szBuf[1024];

	//nothing to do?
	if((!g_hConsole || !(uOutputSettingBits&OUTPUT_CONSOLE)) && g_hLogFile==INVALID_HANDLE_VALUE)
		return;
	//add line break?
	if(uOutputSettingBits&OUTPUT_LINE_BREAK)
	{
		szBuf[0]='\r';
		szBuf[1]='\n';
		iLenPrefix=2;
	}
	else
		iLenPrefix=0;
	if(uOutputSettingBits&OUTPUT_DATE_TIME)
	{
		SYSTEMTIME sTime;

		//add date/time string
		GetLocalTime(&sTime);
		iLen=_snprintf_s(szBuf+iLenPrefix, ARRAYSIZE(szBuf)-iLenPrefix, _TRUNCATE, DATE_TIME_FORMAT, sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
		if(iLen<0)	//error?
			return;
		iLenPrefix+=iLen;
	}
	//add log string
	va_start(pArgs, szFormatString);
	iLen=_vsnprintf_s(szBuf+iLenPrefix, ARRAYSIZE(szBuf)-iLenPrefix, _TRUNCATE, szFormatString, pArgs);
	if(iLen<0)	//error?
		return;
	iLen+=iLenPrefix;
	//output
	if(g_hConsole && (uOutputSettingBits&OUTPUT_CONSOLE))
	{
		wchar_t wcBuf[1024];

		//convert to wide char to avoid trouble with special characters
		iLen=MultiByteToWideChar(CP_ACP, 0, szBuf, iLen, wcBuf, ARRAYSIZE(wcBuf));
		if(iLen)
			WriteConsoleW(g_hConsole, wcBuf, iLen, &uDummy, NULL);
	}
	if(g_hLogFile)
		WriteFile(g_hLogFile, szBuf, iLen, &uDummy, NULL);
}

static void OpenLogFile(const char* szFilePath, int bAppend)
{
	g_hLogFile=CreateFile(szFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, bAppend?OPEN_ALWAYS:CREATE_ALWAYS, 0, NULL);
	if(g_hLogFile==INVALID_HANDLE_VALUE)
	{
		char* szErrMsg=GetSysError(GetLastError());

		OutputToLog(OUTPUT_ALL, "Failed to open log file \"%s\": %s", szFilePath, szErrMsg);
		FreeSysError(szErrMsg);
	}
	else
	{
		//append and no new file was created?
		if(bAppend && GetLastError()==ERROR_ALREADY_EXISTS)
		{
			DWORD uDummy;

			//go to end of file for appending
			SetFilePointer(g_hLogFile, 0, NULL, FILE_END);
			//add line breaks for appending
			WriteFile(g_hLogFile, STRING_AND_LEN("\r\n\r\n"), &uDummy, NULL);
		}
		//output header string in log
		OutputToLog(OUTPUT_DATE_TIME, LOG_HEADER);
	}
}

static void OpenConsole()
{
	if(g_hConsole)	//already openend?
		return;
	AllocConsole();
	g_hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
	if(g_hConsole)
	{
		DWORD uDummy;

		WriteConsole(g_hConsole, STRING_AND_LEN(APP_STRING), &uDummy, NULL);
	}
}

static void OutputFatal(const char* szFormatString, ...)
{
	HANDLE hReadConsole;
	va_list pArgs;
	int iLen;
	DWORD uDummy;
	INPUT_RECORD sRecord;
	char szBuf[4096];

	OpenConsole();
	if(!g_hConsole)	//console available?
		return;
	va_start(pArgs, szFormatString);
	iLen=_vsnprintf_s(szBuf, ARRAYSIZE(szBuf), _TRUNCATE, szFormatString, pArgs);
	if(iLen<0)	//error?
		return;
	//output
	WriteConsole(g_hConsole, szBuf, iLen, &uDummy, NULL);
	WriteConsole(g_hConsole, STRING_AND_LEN("\nPress any key to close the application..."), &uDummy, NULL);
	hReadConsole=GetStdHandle(STD_INPUT_HANDLE);
	//wait for key
	while(ReadConsoleInput(hReadConsole, &sRecord, 1, &uDummy) && (sRecord.EventType!=KEY_EVENT || !sRecord.Event.KeyEvent.bKeyDown))
		;
}

//thread handling for Windows
#define THREAD_FUNCTION(threadFunction, pParam) static DWORD __stdcall threadFunction(LPVOID pParam)

int ThreadCreate(LPTHREAD_START_ROUTINE pThreadFunction, void* pParam)
{
	DWORD uId;
	HANDLE hThread=CreateThread(NULL, 0, pThreadFunction, pParam, 0, &uId);

	if(!hThread)
	{
		char* szErrMsg=GetSysError(GetLastError());

		OutputToLog(OUTPUT_ALL, "Creating new thread has failed: %s", szErrMsg);
		FreeSysError(szErrMsg);
		return 0;	//error
	}
	CloseHandle(hThread);
	return 1;	//o.k.
}

#else //#ifdef _WIN32

//on other OS create no own console window
static int g_bConsole=0;		//console output enabled?
static FILE* g_pLogFile=NULL;	//log file pointer

#define GetSysError(iErrNo) strerror(iErrNo)
#define FreeSysError(szString)	//nothing to do

static void OutputToLog(unsigned int uOutputSettingBits, const char* szFormatString, ...)
{
	va_list pArgs;
	int iLenPrefix;
	int iLen;
	char szBuf[4096];

	//nothing to do?
	if((!g_bConsole || !(uOutputSettingBits&OUTPUT_CONSOLE)) && !g_pLogFile)
		return;
	//add line break?
	if(uOutputSettingBits&OUTPUT_LINE_BREAK)
	{
		szBuf[0]='\n';
		iLenPrefix=1;
	}
	else
		iLenPrefix=0;
	if(uOutputSettingBits&OUTPUT_DATE_TIME)
	{
		//add date/time string
		time_t iTime;
		struct tm* psTime;

		if(time(&iTime)==(time_t)-1)
			return;	//error
		psTime=localtime(&iTime);
		if(!psTime)	//error?
			return;
		iLen=snprintf(szBuf+iLenPrefix, sizeof(szBuf)-iLenPrefix, DATE_TIME_FORMAT, psTime->tm_year+1900, psTime->tm_mon+1, psTime->tm_mday, psTime->tm_hour, psTime->tm_min, psTime->tm_sec);
		if(iLen<0)	//error?
			return;
		iLenPrefix+=iLen;
	}
	//add log string
	va_start(pArgs, szFormatString);
	iLen=vsnprintf(szBuf+iLenPrefix, sizeof(szBuf)-iLenPrefix, szFormatString, pArgs);
	va_end(pArgs);
	if(iLen<0)	//error?
		return;
	iLen+=iLenPrefix;
	if(iLen>=sizeof(szBuf))	//truncated? -> make sure to add \0
	{
		szBuf[sizeof(szBuf)-1]='\0';
		iLen=sizeof(szBuf)-1;
	}
	//output
	if(g_bConsole && (uOutputSettingBits&OUTPUT_CONSOLE))
		fputs(szBuf, stdout);
	if(g_pLogFile)
		fwrite(szBuf, 1, iLen, g_pLogFile);
}

static void OpenLogFile(const char* szFilePath, int bAppend)
{
	g_pLogFile=fopen(szFilePath, bAppend?"ab":"wb");
	if(!g_pLogFile)
		OutputToLog(OUTPUT_ALL, "Failed to open log file \"%s\": %s", szFilePath, strerror(errno));
	else
	{
		//append and file is not empty?
		if(bAppend && ftell(g_pLogFile))
		{
			//add line breaks for appending
			fputs("\n\n", g_pLogFile);
		}
		setbuf(g_pLogFile, NULL);	//disable buffering (no newline needed for actual output)
		//output header string in log
		OutputToLog(OUTPUT_DATE_TIME, LOG_HEADER);
	}
}

static void OpenConsole()
{
	if(g_bConsole)	//already openend?
		return;
	g_bConsole=1;
	setbuf(stdout, NULL);	//disable buffering (no newline needed for actual output)
	printf(APP_STRING);
}

//for OutputFatal we can just use a macro to prefix the parameters
#define OutputFatal	OpenConsole(); printf

//thread handling via pthread
#define THREAD_FUNCTION(threadFunction, pParam) static void* threadFunction(void* pParam)

int ThreadCreate(void* (*pThreadFunction)(void*), void* pParam)
{
	pthread_t hThread;
	int iErrNo=pthread_create(&hThread, NULL, pThreadFunction, pParam);

	if(iErrNo)
	{
		OutputToLog(OUTPUT_ALL, "Creating new thread has failed: error code %d", iErrNo);
		return 0;	//error
	}
	pthread_detach(hThread);
	return 1;	//o.k.
}

#endif //#ifdef _WIN32


//returns the internal length of a socket address (IPv4 / IPv6)
static unsigned int GetAddrLen(const struct sockaddr_storage* psAddr)
{
	if(psAddr->ss_family==AF_INET)
		return sizeof(struct sockaddr_in);
	return sizeof(struct sockaddr_in6);
}

//sends UDP answer
static void SendAnswer(struct SEntry* psEntry)
{
	//ignore error here
	//UDP?
	if(psEntry->uAddrLen!=sizeof(SOCKET))
		//+2 because DNS on UDP doesn't include the length
		sendto(g_hSockUdp, (char*)(psEntry->u16aAnswer+1), ntohs(*psEntry->u16aAnswer), 0, (struct sockaddr*)&psEntry->client.sAddr, psEntry->uAddrLen);
	else
	{
		//TCP
		send(*(SOCKET*)&psEntry->client.hSock, (char*)psEntry->u16aAnswer, ntohs(*psEntry->u16aAnswer)+2, 0);
		closesocket(psEntry->client.hSock);
	}
}

//searches for an entry in the cache list and removes it - also closes a socket and outputs an error message
static void RemoveEntry(struct SEntry* psEntry, SOCKET hSock, int bUseCriticalSection)
{
	struct SEntry** ppsPrev;
	struct SEntry* psEntry2;

	//close the socket created by the caller
	if(hSock!=SOCKET_ERROR)
		closesocket(hSock);
	if(bUseCriticalSection)
		EnterCriticalSection(&g_sCritSect);
	ppsPrev=&g_psFirst;
	for(psEntry2=g_psFirst; psEntry2; psEntry2=psEntry2->psNext)
	{
		//found entry?
		if(psEntry==psEntry2)
		{
			//remove entry from list
			*ppsPrev=psEntry->psNext;
			--g_uCacheCount;
			break;
		}
		ppsPrev=&psEntry2->psNext;
	}
	//free the entry
	free(psEntry->u16aAnswer);	//should be NULL anyway
	//close socket in case of TCP
	if(psEntry->uAddrLen==sizeof(SOCKET))
		closesocket(psEntry->client.hSock);
	free(psEntry);
	if(bUseCriticalSection)
		LeaveCriticalSection(&g_sCritSect);
 }

static int InvalidEntryErrorOutput()
{
	OutputToLog(OUTPUT_ALL, "Invalid DNS answer detected while calculating TTL");
	return 0;	//mark as expired for CalculateTimeToLive
}

//updates the time to live fields of an entry and checks for expiration (returns 0 if expired)
static int CalculateTimeToLive(struct SEntry* psEntry)
{
	time_t iCurTime;
	int32_t i32TimeOffset;
	int32_t i32TimeToLive;
	uint16_t u16ContentLen;
	uint16_t u16AmountQuestions;
	uint16_t u16Len;
	uint8_t* pu8Pos;
	uint8_t* pu8AnswerEnd;
	uint8_t u8NameLen;

	if(psEntry->iTime==(time_t)-1)
		return 1;	//getting time failed last time; without a working timer we ignore time to live stuff completely
	u16Len=ntohs(*psEntry->u16aAnswer);
	if(u16Len<=12)
		return InvalidEntryErrorOutput();	//answer has no useful information; mark as expired
	pu8Pos=(uint8_t*)psEntry->u16aAnswer+2;
	pu8AnswerEnd=pu8Pos+u16Len;
	u16AmountQuestions=ntohs(((uint16_t*)pu8Pos)[2]);
	pu8Pos+=12;	//go behind header
	//ignore questions
	for(; u16AmountQuestions; --u16AmountQuestions)
	{
		//ignore name
		for(;;)
		{
			if(pu8Pos>=pu8AnswerEnd)
				return InvalidEntryErrorOutput();	//failed; mark as expired
			u8NameLen=*pu8Pos;
			if(!u8NameLen)
				break;	//end of name
			if(u8NameLen>=0xc0)	//compression used? (reference to other name via offset)
			{
				++pu8Pos;	//ignore 2nd part of offset
				break;
			}
			pu8Pos+=u8NameLen+1;
		}
		pu8Pos+=5;	//ignore type and class
	}
	//calculate amount of seconds since last delivery
	if(time(&iCurTime)==(time_t)-1)
		return 1;	//error, can't get current time -> ignore time to live stuff completely
	i32TimeOffset=(int32_t)(iCurTime-psEntry->iTime);
	psEntry->iTime=iCurTime;	//store current time for next delivery
	//for all records
	do
	{
		//ignore name of resource record
		for(;;)
		{
			if(pu8Pos>=pu8AnswerEnd)
				return InvalidEntryErrorOutput();	//failed; mark as expired
			u8NameLen=*pu8Pos;
			if(!u8NameLen)
				break;	//end of name
			if(u8NameLen>=0xc0)	//compression used? (reference to other name via offset)
			{
				++pu8Pos;	//ignore 2nd part of offset
				break;
			}
			pu8Pos+=u8NameLen+1;
		}
		pu8Pos+=5;	//ignore type and class
		if(pu8Pos>pu8AnswerEnd-4)
			return InvalidEntryErrorOutput();	//failed; mark as expired
		//check time to live field (0 means "omitted")
		i32TimeToLive=ntohl(*(uint32_t*)pu8Pos);
		if(i32TimeToLive>0)
		{
			i32TimeToLive-=i32TimeOffset;
			if(i32TimeToLive<=0)
				return 0;	//expired
			*(uint32_t*)pu8Pos=htonl(i32TimeToLive);	//update field
		}
		else if(i32TimeToLive<0)
			return 0;	//failed, TTL must be positive; mark as expired, however: no error output in this case
		//ignore record content
		if(pu8Pos>pu8AnswerEnd-6)
			return InvalidEntryErrorOutput();	//failed; mark as expired
		u16ContentLen=ntohs(*(uint16_t*)(pu8Pos+4));
		pu8Pos+=6+u16ContentLen;
		if(pu8Pos>pu8AnswerEnd)
			return InvalidEntryErrorOutput();	//failed; mark as expired
	}
	while(pu8Pos<pu8AnswerEnd);
	return 1;	//succeeded, not expired
}

//receives a specific amount of bytes
static int ReceiveBytes(SOCKET hSock, unsigned int uAmount, uint16_t* u16aBuf)
{
	unsigned int uPos=0;
	int iLen;
	char* szErrMsg;

	for(;;)
	{
		iLen=recv(hSock, (char*)u16aBuf+uPos, uAmount, 0);
		switch(iLen)
		{
		case SOCKET_ERROR:
			szErrMsg=GetSysError(WSAGetLastError());
			OutputToLog(OUTPUT_ALL, "Receiving from SOCKS server has failed: %s", szErrMsg);
			FreeSysError(szErrMsg);
			return 0;	//failed
		case 0:
			OutputToLog(OUTPUT_ALL, "The SOCKS server has closed the connection unexpectedly");
			return 0;	//failed
		default:
			uAmount-=iLen;
			if(!uAmount)
				return 1;	//succeeded
			uPos+=iLen;
		}
	}
}

//thread for connecting the SOCKS server and resolving the DNS request
THREAD_FUNCTION(SocksThread, pEntry)
{
	uint16_t u16aBuf[32754];	//max UDP packet length on Windows plus one byte - using uint16_t here for alignment
	struct SEntry* psEntry=(struct SEntry*)pEntry;
	char* szErrMsg;
	SOCKET hSock=socket(g_sSocksAddr.ss_family, SOCK_STREAM, IPPROTO_TCP);
	int iRet;
	int iPos;
	int iLen;

	if(hSock==SOCKET_ERROR)
	{
		szErrMsg=GetSysError(WSAGetLastError());
		OutputToLog(OUTPUT_ALL, "Creating a TCP socket has failed: %s", szErrMsg);
		FreeSysError(szErrMsg);
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	//connect SOCKS server
	if(connect(hSock, (struct sockaddr*)&g_sSocksAddr, GetAddrLen(&g_sSocksAddr))==SOCKET_ERROR)
	{
		szErrMsg=GetSysError(WSAGetLastError());
		OutputToLog(OUTPUT_ALL, "Connecting the SOCKS server has failed: %s", szErrMsg);
		FreeSysError(szErrMsg);
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	((uint8_t*)u16aBuf)[0]=5;	//version 5 as we use SOCKS5
	((uint8_t*)u16aBuf)[1]=1;	//number of authentication methods supported
	((uint8_t*)u16aBuf)[2]=g_uaUsrPwd?2:0;	//user/password authentication or no authentication
	iLen=send(hSock, (const char*)u16aBuf, 3, 0);
	if(iLen!=3)
	{
		szErrMsg=(iLen==SOCKET_ERROR)?GetSysError(WSAGetLastError()):"Invalid amount of sent bytes";
		OutputToLog(OUTPUT_ALL, "Sending to SOCKS server has failed: %s", szErrMsg);
		if(iLen==SOCKET_ERROR)
			FreeSysError(szErrMsg);
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	//check respons
	if(!ReceiveBytes(hSock, 2, u16aBuf+8))
	{
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	if(((uint8_t*)u16aBuf)[16]!=5 || ((uint8_t*)u16aBuf)[17]!=((uint8_t*)u16aBuf)[2])
	{
		if(((uint8_t*)u16aBuf)[16]!=5)
			OutputToLog(OUTPUT_ALL, "The SOCKS server has answered with a SOCKS version number unequal to 5");
		else if(g_uaUsrPwd)
			OutputToLog(OUTPUT_ALL, "The SOCKS server does not support user/password authentication");
		else
			OutputToLog(OUTPUT_ALL, "The SOCKS server wants an authentication");
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	//send authentication if enabled
	if(g_uaUsrPwd)
	{
		iLen=send(hSock, (const char*)g_uaUsrPwd, g_iUsrPwdLen, 0);
		if(iLen!=g_iUsrPwdLen)
		{
			szErrMsg=(iLen==SOCKET_ERROR)?GetSysError(WSAGetLastError()):"Invalid amount of sent bytes";
			OutputToLog(OUTPUT_ALL, "Sending to SOCKS server has failed: %s", szErrMsg);
			if(iLen==SOCKET_ERROR)
				FreeSysError(szErrMsg);
			RemoveEntry(psEntry, hSock, g_bCacheEnabled);
			return 0;
		}
		//check response
		if(!ReceiveBytes(hSock, 2, u16aBuf+8))
		{
			RemoveEntry(psEntry, hSock, g_bCacheEnabled);
			return 0;
		}
		if(((uint8_t*)u16aBuf)[17])	//2nd byte of answer must be 0 for "success"
		{
			OutputToLog(OUTPUT_ALL, "The SOCKS server authentication has failed (error code %u)", (unsigned int)((uint8_t*)u16aBuf)[17]);
			RemoveEntry(psEntry, hSock, g_bCacheEnabled);
			return 0;
		}
	}
	//connect DNS server via SOCKS, the DNS server must support TCP
	((uint8_t*)u16aBuf)[1]=1;	//establish a TCP/IP stream connection
	((uint8_t*)u16aBuf)[2]=0;	//reserved, must be 0x00
	switch(g_sDnsSrvAddr.ss_family)
	{
	case AF_UNSPEC:	//use name
		((uint8_t*)u16aBuf)[3]=3;	//name
		iLen=(int)((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_flowinfo;	//length in sin6_flowinfo (see ParseIpAndPort)
		((uint8_t*)u16aBuf)[4]=(uint8_t)iLen;	//maximum length is 255
		memcpy(((uint8_t*)u16aBuf)+5, *(char**)&((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_addr, iLen);	//copy name (see ParseIpAndPort)
		*(uint16_t*)(((uint8_t*)u16aBuf)+iLen+5)=((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_port;	//port
		iPos=iLen+7;
		break;
	case AF_INET:	//use IPv4
		((uint8_t*)u16aBuf)[3]=1;	//IPv4 address
		*(uint32_t*)(u16aBuf+2)=((struct sockaddr_in*)&g_sDnsSrvAddr)->sin_addr.s_addr;	//address
		*(uint16_t*)(u16aBuf+4)=((struct sockaddr_in*)&g_sDnsSrvAddr)->sin_port;		//port
		iPos=10;
		break;
	default:	//use IPv6
		((uint8_t*)u16aBuf)[3]=4;	//IPv6 address
		memcpy(u16aBuf+2, &((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_addr, 16);		//address
		*(uint16_t*)(u16aBuf+10)=((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_port;		//port
		iPos=22;
	}
	iLen=send(hSock, (const char*)u16aBuf, iPos, 0);
	if(iLen!=iPos)
	{
		szErrMsg=(iLen==SOCKET_ERROR)?GetSysError(WSAGetLastError()):"Invalid amount of sent bytes";
		OutputToLog(OUTPUT_ALL, "Connecting through SOCKS server has failed: %s", szErrMsg);
		if(iLen==SOCKET_ERROR)
			FreeSysError(szErrMsg);
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	//check expected answer (get first 5 bytes to detect address type)
	if(!ReceiveBytes(hSock, 5, u16aBuf))
	{
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	if(((uint8_t*)u16aBuf)[0]!=5 || ((uint8_t*)u16aBuf)[1]!=0 || (((uint8_t*)u16aBuf)[3]!=1 && ((uint8_t*)u16aBuf)[3]!=3 && ((uint8_t*)u16aBuf)[3]!=4))
	{
		szErrMsg="Unexpected answer from SOCKS server";
		//correct version -> try to resolve the error code
		if(((uint8_t*)u16aBuf)[0]==5)
			switch(((uint8_t*)u16aBuf)[1])
			{
			case 2:
				szErrMsg="Connection not allowed by ruleset";
				break;
			case 3:
				szErrMsg="Network unreachable";
				break;
			case 4:
				szErrMsg="Host unreachable";
				break;
			case 5:
				szErrMsg="Connection refused by destination host";
				break;
			case 6:
				szErrMsg="TTL expired";
				break;
			case 7:
				szErrMsg="Command not supported / protocol error";
				break;
			case 8:
				szErrMsg="Address type not supported";
			}
		OutputToLog(OUTPUT_ALL, "Connecting through SOCKS server has failed: %s", szErrMsg);
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	//get rest of answer
	//all bytes after that are part of the "normal" communication
	switch(((uint8_t*)u16aBuf)[3])
	{
	case 1:	//IPv4
		iLen=5;
		break;
	case 4:	//IPv6
		iLen=17;
		break;
	default:	//name
		iLen=2+((uint8_t*)u16aBuf)[4];	//port length plus length of name
	}
	if(!ReceiveBytes(hSock, iLen, u16aBuf))
	{
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}

	iPos=2+ntohs(*psEntry->u16aRequest);
	//send DNS request via SOCKS
	iLen=send(hSock, (const char*)psEntry->u16aRequest, iPos, 0);
	if(iLen!=iPos)
	{
		szErrMsg=(iLen==SOCKET_ERROR)?GetSysError(WSAGetLastError()):"Invalid amount of sent bytes";
		OutputToLog(OUTPUT_ALL, "Sending through SOCKS server has failed: %s", szErrMsg);
		if(iLen==SOCKET_ERROR)
			FreeSysError(szErrMsg);
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	//receive answer
	iPos=0;
	for(;;)
	{
		iRet=recv(hSock, (char*)u16aBuf+iPos, sizeof(u16aBuf)-iPos, 0);
		if(iRet<=0)
		{
			szErrMsg=(iRet==SOCKET_ERROR)?GetSysError(WSAGetLastError()):"Server has closed the connection unexpectedly";
			OutputToLog(OUTPUT_ALL, "Broken answer from DNS server: %s", szErrMsg);
			if(iRet==SOCKET_ERROR)
				FreeSysError(szErrMsg);
			RemoveEntry(psEntry, hSock, g_bCacheEnabled);
			return 0;
		}
		iPos+=iRet;
		//first 2 bytes contain length (Big Endian)
		if(iPos>=2)
		{
			iLen=2+ntohs(*u16aBuf);
			if(iPos>=iLen)
				break;	//answer completely received
		}
		//answer too long?
		if(iPos>=sizeof(u16aBuf))
		{
			OutputToLog(OUTPUT_ALL, "Answer from DNS server too long!");
			RemoveEntry(psEntry, hSock, g_bCacheEnabled);
			return 0;
		}
	}
	//invalid answer?
	if(iPos<=4)
	{
		OutputToLog(OUTPUT_ALL, "Answer from DNS server too short!");
		RemoveEntry(psEntry, hSock, g_bCacheEnabled);
		return 0;
	}
	closesocket(hSock);
	if(g_bCacheEnabled)
	{
		//store answer in cache
		EnterCriticalSection(&g_sCritSect);
		psEntry->u16aAnswer=(uint16_t*)malloc(iLen);
		memcpy(psEntry->u16aAnswer, u16aBuf, iLen);
		//copy ID
		psEntry->u16aAnswer[1]=psEntry->u16aRequest[1];
		//remember current time for time to live calculations
		psEntry->iTime=time(NULL);
		//send DNS answer to original requesting client via UDP or TCP
		SendAnswer(psEntry);
		LeaveCriticalSection(&g_sCritSect);
	}
	else
	{
		//send DNS answer to original requesting client via UDP or TCP
		psEntry->u16aAnswer=u16aBuf;
		SendAnswer(psEntry);
		free(psEntry);
	}
	return 0;
}

//searches the cache for the same request and sends the answer if there is a cache hit
//or creates a thread for forwarding the request to the DNS server via SOCKS
static void HandleDnsRequest(uint16_t* u16aRequest, int iLen, void* pClientAddr, socklen_t iAddrLen)
{
	uint8_t* pu8Pos;
	uint8_t* pu8End;
	struct SEntry* psEntry;
	unsigned char uLen;

	//search in cache
	EnterCriticalSection(&g_sCritSect);
	for(psEntry=g_psFirst; ; psEntry=psEntry->psNext)
	{
		if(!psEntry)
		{
			//create new entry (length of struct SEntry up to u16aRequest plus request length plus 2 for length)
			psEntry=(struct SEntry*)malloc(((uint8_t*)psEntry->u16aRequest-(uint8_t*)psEntry)+iLen+2);
			psEntry->iTime=(time_t)-1;
			psEntry->u16aAnswer=NULL;
			psEntry->uAddrLen=(unsigned char)iAddrLen;
			memcpy(&psEntry->client, pClientAddr, iAddrLen);
			*psEntry->u16aRequest=htons((uint16_t)iLen);
			memcpy(psEntry->u16aRequest+1, u16aRequest, iLen);
			//add entry to cache list in case cache is enabled
			psEntry->psNext=g_psFirst;
			if(g_bCacheEnabled)
				g_psFirst=psEntry;
			//create thread to resolve entry
			if(ThreadCreate(SocksThread, psEntry))
			{
				++g_uCacheCount;
				//output amount of entries and current entry
				pu8Pos=(uint8_t*)u16aRequest+12;
				pu8End=(uint8_t*)u16aRequest+iLen;
				while(pu8Pos<pu8End)
				{
					uLen=*pu8Pos;
					if(!uLen)
					{
						OutputToLog(OUTPUT_ALL, "%3u %s", g_uCacheCount, (char*)((uint8_t*)u16aRequest+13));
						break;
					}
					if(uLen>=0xc0)	//compression used? (reference to other name via offset)
						break;		//no output in this case
					*pu8Pos='.';	//replace length by .
					pu8Pos+=uLen+1;
				}
			}
			else
			{
				//remove entry from cache list
				g_psFirst=psEntry->psNext;
				free(psEntry);
			}
			break;
		}
		//cache hit? (do not compare ID in first 2 bytes of request)
		if((uint16_t)iLen==ntohs(*psEntry->u16aRequest) && memcmp(u16aRequest+1, psEntry->u16aRequest+2, iLen-2)==0)
		{
			//answer already received?
			if(psEntry->u16aAnswer)
			{
				//answer to current address
				psEntry->uAddrLen=(unsigned char)iAddrLen;
				memcpy(&psEntry->client, pClientAddr, iAddrLen);
				//check if expired
				if(!CalculateTimeToLive(psEntry))
				{
					//expired -> kill last answer
					free(psEntry->u16aAnswer);
					psEntry->u16aAnswer=NULL;
					//copy current ID
					psEntry->u16aRequest[1]=*u16aRequest;
					//create thread to resolve request again
					if(!ThreadCreate(SocksThread, psEntry))
						RemoveEntry(psEntry, (SOCKET)SOCKET_ERROR, 0);
				}
				else
				{
					//use current ID
					psEntry->u16aAnswer[1]=*u16aRequest;
					SendAnswer(psEntry);
				}
			}
			else
			{
				//copy current ID so the thread uses that one if it gets the answer
				psEntry->u16aRequest[1]=*u16aRequest;
				//overwrite address; currently we can only handle one request address while waiting for an answer through SOCKS
				//current address is TCP? -> need to close it before overwriting it
				if(psEntry->uAddrLen==sizeof(SOCKET))
					closesocket(psEntry->client.hSock);
				psEntry->uAddrLen=(unsigned char)iAddrLen;
				memcpy(&psEntry->client, pClientAddr, iAddrLen);
			}
			break;
		}
	}
	LeaveCriticalSection(&g_sCritSect);
}

//outputs an error caused by "bind" and closes the according socket
static void OutputBindError(SOCKET hSock, struct sockaddr_storage* psAddr, int bUdp)
{
	char szAddr[256];
	char szNo[16];
	char* szErrMsg=GetSysError(WSAGetLastError());

	closesocket(hSock);
	if(getnameinfo((struct sockaddr*)psAddr, GetAddrLen(psAddr), szAddr, sizeof(szAddr), szNo, sizeof(szNo), NI_NUMERICHOST|NI_NUMERICSERV))
	{
		//should never happen
		strcpy(szAddr, "unknown address");
		strcpy(szNo, "unknown");
	}
	//UDP is mandatory, TCP is optional
	if(bUdp)
	{
		//need { ... } on *nix as OutputFatal is a multi-line macro there
		OutputFatal("\nBinding on %s, UDP port %s has failed: %s\n", szAddr, szNo, szErrMsg);
	}
	else
		OutputToLog(OUTPUT_ALL, "Binding on %s, TCP port %s has failed: %s", szAddr, szNo, szErrMsg);
	FreeSysError(szErrMsg);
}

//thread for receiving DNS requests via TCP
THREAD_FUNCTION(TcpThread, pAddr)
{
	SOCKET hSockServer;
	SOCKET hSock;
	uint16_t u16aBuf[32769];	//maximum possible DNS request size plus one byte - using uint16_t here for alignment; the actual size is stored in the first 2 bytes which can have a maximum value of 0xffff
	socklen_t iAddrLen;
	int iCurBufLen;
	int iLen;
	uint16_t u16ReqLen;
	struct sockaddr_storage sAddr;
	char* szErrMsg;

	hSockServer=socket(((struct sockaddr_storage*)pAddr)->ss_family, SOCK_STREAM, IPPROTO_TCP);
	if(hSockServer==SOCKET_ERROR)
	{
		szErrMsg=GetSysError(WSAGetLastError());
		OutputToLog(OUTPUT_ALL, "Creating a TCP socket has failed: %s", szErrMsg);
		FreeSysError(szErrMsg);
		return 0;
	}
	//bind+listen on local TCP port (get DNS requests)
	if(bind(hSockServer, (struct sockaddr*)pAddr, GetAddrLen((struct sockaddr_storage*)pAddr))==SOCKET_ERROR)
	{
		OutputBindError(hSockServer, (struct sockaddr_storage*)pAddr, 0);
		return 0;
	}
	if(listen(hSockServer, 5)==SOCKET_ERROR)
	{
		szErrMsg=GetSysError(WSAGetLastError());
		closesocket(hSockServer);
		OutputToLog(OUTPUT_ALL, "Listening on TCP socket has failed: %s", szErrMsg);
		FreeSysError(szErrMsg);
		return 0;
	}
	//as long as "accept" is working
	for(;;)
	{
		iAddrLen=sizeof(sAddr);
		hSock=accept(hSockServer, (struct sockaddr*)&sAddr, &iAddrLen);
		if(hSock==SOCKET_ERROR)
		{
			szErrMsg=GetSysError(WSAGetLastError());
			OutputToLog(OUTPUT_ALL, "Accepting new connection on TCP socket has failed: %s", szErrMsg);
			FreeSysError(szErrMsg);
			break;
		}
		//collect the whole DNS request
		iCurBufLen=0;
		for(;;)
		{
			iLen=recv(hSock, (char*)u16aBuf+iCurBufLen, sizeof(u16aBuf)-iCurBufLen, 0);
			if(iLen<=0)
			{
				szErrMsg=GetSysError(WSAGetLastError());
				closesocket(hSock);
				OutputToLog(OUTPUT_ALL, "DNS request on TCP broken: %s", szErrMsg);
				FreeSysError(szErrMsg);
				break;
			}
			iCurBufLen+=iLen;
			//got the whole DNS request?
			if(iCurBufLen>=2 && (u16ReqLen=ntohs(*u16aBuf))+2>=iCurBufLen)
			{
				if(u16ReqLen>12)	//12 bytes header plus at least one byte of data
					HandleDnsRequest(u16aBuf+1, u16ReqLen, &hSock, sizeof(hSock));	//HandleDnsRequest takes care of hSock now
				else
					closesocket(hSock);
				break;
			}
		}
	}
	closesocket(hSockServer);
	return 0;
}

//parses a command line parameter of the format IPv4 or IPv4:port or IPv6 or [IPv6]:port
static int ParseIpAndPort(int iFlag, const char* szParamName, const char* szPort, char* szIpAndPort, struct sockaddr_storage* psAddr)
{
	struct addrinfo sHint;
	struct addrinfo* psResult;
	char* szPos;
	int iRet;

	if(strchr(szIpAndPort, '.'))
	{
		//seems to be IPv4
		szPos=strchr(szIpAndPort, ':');
		if(szPos)
		{
			*szPos='\0';	//overwrite ':' for getaddrinfo below
			szPort=szPos+1;
		}
	}
	else
	{
		//seems to be IPv6
		//format [IPv6]:port?
		if(*szIpAndPort=='[')
		{
			++szIpAndPort;
			szPos=strchr(szIpAndPort, ']');
			if(szPos)
			{
				*szPos='\0';	//overwrite ']' for getaddrinfo below
				//is there a port specification?
				if(szPos[1]==':')
					szPort=szPos+2;
			}
		}
	}

	//prepare hint: only numeric values
	memset(&sHint, 0, sizeof(sHint));
	sHint.ai_family=AF_UNSPEC;
	sHint.ai_flags=iFlag|AI_NUMERICHOST|AI_NUMERICSERV;
	//now resolve it
	iRet=getaddrinfo(szIpAndPort, szPort, &sHint, &psResult);
	if(iRet)
	{
		//only for the DNS server also support name
		if(&g_sDnsSrvAddr==psAddr)
		{
			size_t uLen=strlen(szIpAndPort);

			if(uLen<256 && uLen)
			{
				((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_port=htons((uint16_t)atoi(szPort));
				if(((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_port)
				{
					g_sDnsSrvAddr.ss_family=AF_UNSPEC;	//this marks usage of name
					*(char**)&((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_addr=szIpAndPort;	//use sin6_addr for pointer to name (128 bit -> large enough, alignment should also be fine)
					((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_flowinfo=(uint8_t)uLen;		//use sin6_flowinfo for length
					return 1;	//o.k.
				}
			}
		}
		OutputFatal("\nInvalid address '%s' and port '%s' specified for %s!\n", szIpAndPort, szPort, szParamName);
		return 0;	//error
	}
	if(psResult->ai_addrlen>sizeof(*psAddr))
	{
		//should never happen
		OutputFatal("\nAddress '%s' and port '%s' specified for %s too long for internal storage!\n", szIpAndPort, szPort, szParamName);
		return 0;	//error
	}
	//copy 1st result
	memcpy(psAddr, psResult->ai_addr, psResult->ai_addrlen);
	freeaddrinfo(psResult);
	return 1;	//o.k.
}

int main(int iArgCount, char** szaArgs)
{
	static struct sockaddr_storage sAddr;	//make it static so the lower array assignment causes no compiler warning
	const char* szUser;
	const char* szPassword;
	char** pszCurArg;
	char* szCurArg;
	char* szLogFilePath;
	char* szErrMsg;
	size_t uUserLen;
	size_t uPasswordLen;
	socklen_t iAddrLen;
	int iAddrCount;
	int iLen;
	int bAppend;	//append log file?
	int bQuiet;		//parameter q specified?
	uint16_t u16aBuf[32754];	//max UDP packet length on Windows plus one byte - using uint16_t here for alignment
	//struct array for the three addresses passed via command line
	struct SAddr
	{
		struct sockaddr_storage* psAddr;
		const char* szDefaultAddress;
		const char* szDefaultPort;
		const char* szName;
		int iFlag;
	} saAddresses[3]=
	{
		{ &g_sSocksAddr, DEFAULT_SOCKS_SERVER, DEFAULT_SOCKS_PORT, "SOCKS server", 0 },
		{ &g_sDnsSrvAddr, DEFAULT_DNS_SERVER, DEFAULT_DNS_PORT, "DNS server", 0 },
		{ &sAddr, DEFAULT_LISTEN_IP, DEFAULT_DNS_PORT, "listening", AI_PASSIVE }	//AI_PASSIVE for "getaddrinfo" as we use it for "bind"
	};
	
	//parse command line - use e.g. "/?" to display the usage
	bQuiet=0;
	bAppend=0;
	iAddrCount=0;
	szLogFilePath=NULL;
	szUser=NULL;
	szPassword=NULL;
	for(pszCurArg=szaArgs+1; --iArgCount; ++pszCurArg)
	{
		szCurArg=*pszCurArg;
		//no address parameter?
		if(*szCurArg=='-' || *szCurArg=='/')
		{
			switch(szCurArg[1])
			{
			case 'd':	//disable cache?
			case 'D':
				if(!szCurArg[2])
				{
					g_bCacheEnabled=0;
					continue;	//correct parameter, go to next one
				}
				break;
			case 'q':	//no console output?
			case 'Q':
				if(!szCurArg[2])
				{
					bQuiet=1;
					continue;	//correct parameter, go to next one
				}
				break;
			case 'l':	//log output?
			case 'L':
				if(!szLogFilePath)	//only allowed once
				{
					bAppend=(szCurArg[2]=='a' || szCurArg[2]=='A');	//append?
					if(szCurArg[2+bAppend]==':')
					{
						szLogFilePath=szCurArg+3+bAppend;
						continue;	//correct parameter, go to next one
					}
				}
				break;
			case 'u':	//user?
			case 'U':
				if(!szUser && szCurArg[2]==':')	//only allowed once and : must be 2nd char
				{
					szUser=szCurArg+3;
					continue;
				}
				break;
			case 'p':	//password?
			case 'P':
				if(!szPassword && szCurArg[2]==':')	//only allowed once and : must be 2nd char
				{
					szPassword=szCurArg+3;
					continue;
				}
				break;
			}
		}
		else
		{
			//try to parse address
			if(iAddrCount<sizeof(saAddresses)/sizeof(*saAddresses))
			{
				if(!ParseIpAndPort(saAddresses[iAddrCount].iFlag, saAddresses[iAddrCount].szName, saAddresses[iAddrCount].szDefaultPort, szCurArg, saAddresses[iAddrCount].psAddr))
					return 1;
				++iAddrCount;
				continue;	//correct parameter, go to next one
			}
		}

		//either correct help request or unknown parameter -> display usage and stop
		OutputFatal("\nDNS2SOCKS tunnels DNS requests via SOCKS5 and caches the answers.\n\n\n"
			"Usage:\n\n"
			"DNS2SOCKS [/?] [/d] [/q] [/l[a]:FilePath] [/u:User /p:Password]\n"
			"          [Socks5ServerIP[:Port]] [DNSServerIPorName[:Port]] [ListenIP[:Port]]\n\n"
			"/?            to view this help\n"
			"/d            to disable the cache\n"
			"/q            to suppress the text output\n"
			"/l:FilePath   to create a new log file \"FilePath\"\n"
			"/la:FilePath  to create a new log file or append to the existing \"FilePath\"\n"
			"/u:User       user name if your SOCKS server uses user/password authentication\n"
			"/p:Password   password if your SOCKS server uses user/password authentication\n\n"
			"Default Socks5ServerIP:Port = %s:%s\n"
			"Default DNSServerIPorName:Port = %s:%s\n"
			"Default ListenIP:Port = %s:%s\n",
			DEFAULT_SOCKS_SERVER, DEFAULT_SOCKS_PORT,
			DEFAULT_DNS_SERVER, DEFAULT_DNS_PORT,
			DEFAULT_LISTEN_IP, DEFAULT_DNS_PORT);
		return 1;
	}
	if(szPassword && !szUser)
	{
		OutputFatal("\nPassword specified but no user!\n");
		return 1;
	}
	if(!szPassword && szUser)
	{
		OutputFatal("\nUser specified but no password!\n");
		return 1;
	}
	if(szUser)
	{
		uUserLen=strlen(szUser);
		if(uUserLen>255)
		{
			OutputFatal("\nUser exceeds 255 characters!\n");
			return 1;
		}
		uPasswordLen=strlen(szPassword);
		if(uPasswordLen>255)
		{
			OutputFatal("\nPassword exceeds 255 characters!\n");
			return 1;
		}
	}
	else
	{
		//initialize uUserLen and uPasswordLen - otherwise VC++ 2010 outputs a wrong warning
		uUserLen=0;
		uPasswordLen=0;
	}

	//fill unspecified addresses with default values
	while(iAddrCount<sizeof(saAddresses)/sizeof(*saAddresses))
	{
		if(!ParseIpAndPort(saAddresses[iAddrCount].iFlag, saAddresses[iAddrCount].szName, saAddresses[iAddrCount].szDefaultPort, (char*)saAddresses[iAddrCount].szDefaultAddress, saAddresses[iAddrCount].psAddr))
			return 1;
		++iAddrCount;
	}

	g_hSockUdp=socket(sAddr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	if(g_hSockUdp==SOCKET_ERROR)
	{
		szErrMsg=GetSysError(WSAGetLastError());
		OutputFatal("\nCreating a UDP socket has failed: %s\n", szErrMsg);
		FreeSysError(szErrMsg);
		return 1;
	}
	//listen on local UDP port (get DNS requests)
	if(bind(g_hSockUdp, (struct sockaddr*)&sAddr, GetAddrLen(&sAddr))==SOCKET_ERROR)
	{
		OutputBindError(g_hSockUdp, &sAddr, 1);
		return 1;
	}
	if(!bQuiet)
		OpenConsole();
	//convert adresses+ports to strings
	if(getnameinfo((struct sockaddr*)&g_sSocksAddr, GetAddrLen(&g_sSocksAddr), (char*)u16aBuf, 256, (char*)u16aBuf+256, 256, NI_NUMERICHOST|NI_NUMERICSERV))
	{
		//should never happen
		strcpy((char*)u16aBuf, "unknown address");
		strcpy((char*)u16aBuf+256, "unknown");
	}
	if(g_sDnsSrvAddr.ss_family==AF_UNSPEC)	//name for DNS server?
	{
		//sin6_addr contains pointer to name, see ParseIpAndPort (max. length is 255)
		strcpy((char*)u16aBuf+512, *(char**)&((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_addr);
		sprintf((char*)u16aBuf+768, "%hu", ntohs(((struct sockaddr_in6*)&g_sDnsSrvAddr)->sin6_port));
	}
	else if(getnameinfo((struct sockaddr*)&g_sDnsSrvAddr, GetAddrLen(&g_sDnsSrvAddr), (char*)u16aBuf+512, 256, (char*)u16aBuf+768, 256, NI_NUMERICHOST|NI_NUMERICSERV))
	{
		//should never happen
		strcpy((char*)u16aBuf+512, "unknown address");
		strcpy((char*)u16aBuf+768, "unknown");
	}
	if(getnameinfo((struct sockaddr*)&sAddr, GetAddrLen(&sAddr), (char*)u16aBuf+1024, 256, (char*)u16aBuf+1280, 256, NI_NUMERICHOST|NI_NUMERICSERV))
	{
		//should never happen
		strcpy((char*)u16aBuf+1024, "unknown address");
		strcpy((char*)u16aBuf+1280, "unknown");
	}
	//output configuration
	OutputToLog(OUTPUT_LINE_BREAK|OUTPUT_CONSOLE, "SOCKS server %s port %s\n"
		"DNS server   %s port %s\n"
		"listening on %s port %s\n"
		"cache %s\n"
		"authentication %s\n",
		(char*)u16aBuf, (char*)u16aBuf+256,
		(char*)u16aBuf+512, (char*)u16aBuf+768,
		(char*)u16aBuf+1024, (char*)u16aBuf+1280,
		g_bCacheEnabled?"enabled":"disabled",
		szUser?"enabled":"disabled");

	InitializeCriticalSection(&g_sCritSect);

	//log file was requested?
	if(szLogFilePath)
		OpenLogFile(szLogFilePath, bAppend);

	//create authentication package if user/password was specified
	if(szUser)
	{
		g_iUsrPwdLen=uUserLen+uPasswordLen+3;
		g_uaUsrPwd=(unsigned char*)malloc(g_iUsrPwdLen);
		g_uaUsrPwd[0]=1;	//version 1
		g_uaUsrPwd[1]=(unsigned char)uUserLen;
		memcpy(g_uaUsrPwd+2, szUser, uUserLen);
		g_uaUsrPwd[uUserLen+2]=(unsigned char)uPasswordLen;
		memcpy(g_uaUsrPwd+uUserLen+3, szPassword, uPasswordLen);
	}

	//create thread for TCP connection
	ThreadCreate(TcpThread, &sAddr);

	//endless loop
	for(;;)
	{
		//receive DNS request
		iAddrLen=sizeof(sAddr);
		iLen=recvfrom(g_hSockUdp, (char*)u16aBuf, sizeof(u16aBuf), 0, (struct sockaddr*)&sAddr, &iAddrLen);
		if(iLen>12)	//12 bytes header plus at least one byte of data
			HandleDnsRequest(u16aBuf, iLen, &sAddr, iAddrLen);
	}
	/*no cleanup code as we have an endless loop
	  we would need something like this:
	terminate all threads and clear their resources
	free(g_uaUsrPwd);
	DeleteCriticalSection(&g_sCritSect);
	closesocket(g_hSockUdp);
	free all cache entries
	close the console
	close the log file
	return 0;*/
}

#ifdef _WIN32

//entry function for Windows applications - just forwards to "main"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
{
	WSADATA sData;
	char* szErrMsg;
	int iErrNo;

	hInstance;
	hPrevInstance;
	szCmdLine;
	iCmdShow;

	iErrNo=WSAStartup(0x202, &sData);
	if(iErrNo)
	{
		szErrMsg=GetSysError(iErrNo);
		OutputFatal("\nWSAStartup has failed: %s\n", szErrMsg);
		FreeSysError(szErrMsg);
		return 1;
	}

	//call "main" with the already parsed command line parameters stored in global variables by CRT
	iErrNo=main(__argc, __argv);

	WSACleanup();
	return iErrNo;
}

#endif //#ifdef _WIN32

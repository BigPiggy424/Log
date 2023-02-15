/**
 * @file log.cpp
 * @author ldk
 * @brief 用于记录中文日志的日志模块
 * @version 0.1
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "log.hpp"

#ifdef _WIN32

#include <Windows.h>

inline DWORD gettid() { return GetCurrentThreadId(); }

int StrToWStr(const char* _SrcBuf, wchar_t* _DstBuf)
{
	int nLen = MultiByteToWideChar(CP_ACP, 0, _SrcBuf, -1, NULL, 0);
	return MultiByteToWideChar(CP_ACP, 0, _SrcBuf, -1, _DstBuf, nLen);
}

int WStrToStr(const wchar_t* _SrcBuf, char* _DstBuf)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, _SrcBuf, -1, NULL, 0, NULL, NULL);
	return WideCharToMultiByte(CP_ACP, 0, _SrcBuf, -1, _DstBuf, nLen, NULL, NULL);
}

#elif __linux__

#include <unistd.h>
/* 待实现 */

#else

int StrToWStr(const char* _SrcBuf, wchar_t* _DstBuf)
{
	size_t size = strlen(_SrcBuf);
	mbstowcs(_DstBuf, _SrcBuf, size);
	_DstBuf[size] = '\0'
	return size;
}

int WStrToStr(const wchar_t* _SrcBuf, char* _DstBuf)
{
	size_t size = strlen(_SrcBuf);
	wcstombs(_DstBuf, _SrcBuf, size);
	_DstBuf[size] = '\0'
	return size;
}

#endif

std::shared_ptr<Log>    Log::m_Log              { nullptr };
std::wstring            Log::m_wstrLogBuffer	{ 0 };
std::wstring            Log::m_wstrLogFile      { L"./Log.txt" };
LOGLEVEL                Log::m_LogLevel         { LOG_LEVEL_NONE };
LOGTARGET               Log::m_LogTarget        { LOG_TARGET_NONE };
std::once_flag          Log::m_ResourceFlag     { std::once_flag() };
std::shared_mutex       Log::m_LogMutex         { std::shared_mutex() };

void Log::Init(LOGLEVEL _LogLevel, LOGTARGET _LogTarget, std::wstring _Path)
{
	if (!m_Log)
		m_Log.reset(new Log());
	
	setLogLevel(_LogLevel);
	setLogTarget(_LogTarget);
	setLogFile(_Path);
	// 支持中文字符
	setlocale(LC_ALL, "chs");
}

void Log::writeLog
(
	const LOGLEVEL _LogLevel,	// Log等级
	const wchar_t* _FileName,	// 函数所在文件名
	const wchar_t* _Function,	// 函数名
	const uint   _LineNumber,	// 行号
	const char*      _Format,	// 格式化
	...							// 参数列表
)
{
	// 写锁
	std::scoped_lock<std::shared_mutex> writeLock(m_LogMutex);

	if (_LogLevel > m_LogLevel)
		return;

	// 清空之前的日志
	if (m_wstrLogBuffer.size())
		m_wstrLogBuffer.clear();

	m_wstrLogBuffer += std::wstring(L"\n");
	m_wstrLogBuffer += std::wstring(60, L'*');
	m_wstrLogBuffer += std::wstring(L" ");
	m_wstrLogBuffer += std::wstring(LOGLEVEL_WSTRING.at(_LogLevel));
	m_wstrLogBuffer += std::wstring(L" ");
	m_wstrLogBuffer += std::wstring(60, L'*');
	m_wstrLogBuffer += std::wstring(L"\n");

	// 获取日期和时间
	char timeBuffer[20];
	getCurrentLocalTime(timeBuffer);
	wchar_t wTimeBuffer[strlen(timeBuffer)];
	StrToWStr(timeBuffer, wTimeBuffer);
	m_wstrLogBuffer += std::wstring(wTimeBuffer);

	// [进程号] [线程号] [文件名] [函数名:行号]
	wchar_t logInfo[100];
	swprintf(logInfo, 100,
             L" [PID : %-5d] [TID : %-5d] [%-ls] [%-ls : %-4d] ",
             getpid(),
             gettid(),
             _FileName,
             _Function,
             _LineNumber);
	m_wstrLogBuffer += std::wstring(logInfo);

	// 日志正文
	size_t nLen = sizeof _Format;
	wchar_t Format[nLen];
	StrToWStr(_Format, Format);
	wchar_t logInfo2[256];
	va_list args;
	va_start(args, Format);
	vswprintf(logInfo2, 256, Format, args);
	va_end(args);
	m_wstrLogBuffer += std::wstring(logInfo2);

	m_wstrLogBuffer += std::wstring(L"\n");
	m_wstrLogBuffer += std::wstring(60, L'*');
	m_wstrLogBuffer += std::wstring(L" ");
	m_wstrLogBuffer += std::wstring(LOGLEVEL_WSTRING.at(_LogLevel));
	m_wstrLogBuffer += std::wstring(L" ");
	m_wstrLogBuffer += std::wstring(60, L'*');
	m_wstrLogBuffer += std::wstring(L"\n");

	outputToTarget();
}

bool Log::getLogFromFile(std::vector<std::wstring>& _LogTable)
{
	// 读锁
	std::shared_lock<std::shared_mutex> readLock(m_LogMutex);
	std::wifstream wFileInput;
	wFileInput.open(m_wstrLogFile.c_str());
	int ret = 0;
	if (!wFileInput.is_open())
		return false;
	while (wFileInput.good())
	{
		std::wstring line;
		wFileInput >> line;
		_LogTable.emplace_back(line);
	}
	wFileInput.close();
	return true;
}

void Log::outputToTarget()
{
	LOGTARGET target = getLogTarget();
	if (target & LOG_TARGET_CONSOLE)
	{
		std::wcout << m_wstrLogBuffer;
	}
	if (target & LOG_TARGET_FILE)
	{
		std::wofstream wFileOutPut;
		wFileOutPut.open(m_wstrLogFile.c_str(), std::ios::app);
		wFileOutPut << m_wstrLogBuffer;
		wFileOutPut.close();
	}
}

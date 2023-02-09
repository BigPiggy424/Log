/**
 * @file log.hpp
 * @author ldk
 * @brief 用于记录中文日志的日志模块
 * @version 0.1
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <cstring>
#include <memory>
#include <locale>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <chrono>

#if __cplusplus == 202002L
#define CPP20
#include <source_location>
#endif // __cplusplus == 202002L

#ifndef LOG_INIT
#define LOG_INIT Log::Init
#endif // LOG_INIT

#ifndef LOG

#ifdef CPP20
#include <source_location>
#define LOG(logLevel, format, ...)\
        {const std::source_location location{std::source_location::current()};\
		size_t fn_size = strlen(location.file_name());\
		size_t func_size = strlen(location.function_name());\
        char fn[fn_size];\
		char func[func_size];\
		strncpy(fn, location.file_name(), fn_size);\
		strncpy(func, location.function_name(), func_size);\
		wchar_t wfn[fn_size];\
		wchar_t wfunc[func_size];\
		StrToWStr(fn, wfn);\
        StrToWStr(func, wfunc);\
		Log::writeLog(\
			logLevel,\
			wfn,\
			wfunc,\
			location.line(),\
			format,\
			__VA_ARGS__);}\

#else
#define LOG(logLevel, format, ...)\
		{size_t fn_size = strlen(__FILE__);\
		size_t func_size = strlen(__func__);\
        char fn[fn_size];\
		char func[func_size];\
		strncpy(fn, __FILE__, fn_size);\
		strncpy(func, __func__, func_size);\
		wchar_t wfn[fn_size];\
		wchar_t wfunc[func_size];\
		StrToWStr(fn, wfn);\
        StrToWStr(func, wfunc);\
        Log::writeLog(\
			logLevel,\
			wfn,\
			wfunc,\
			__LINE__,\
			format,\
			__VA_ARGS__);}\

#endif // CPP20

#endif // LOG

using std::chrono::system_clock;

typedef unsigned int uint;

enum LOGLEVEL
{
	LOG_LEVEL_NONE,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO
};

enum LOGTARGET
{
	LOG_TARGET_NONE             = 0b00,
	LOG_TARGET_CONSOLE          = 0b01,
	LOG_TARGET_FILE             = 0b10,
	LOG_TARGET_CONSOLE_AND_FILE = 0b11
};

/**
 * @brief char* 转为 wchar_t*
 *
 * @param _SrcBuf IN     源字符串
 * @param _DstBuf OUT    转换后的字符串
 * @return int           若转换成功返回字符串长度，否则返回0
 */
int StrToWStr(const char* _SrcBuf, wchar_t* _DstBuf);

/**
 * @brief whar_t* 转为 char*
 * 
 * @param _SrcBuf IN     源字符串
 * @param _DstBuf OUT    转换后的字符串
 * @return int           若转换成功返回字符串长度，否则返回0
 */
int WStrToStr(const wchar_t* _SrcBuf, char* _DstBuf);

/**
 * @brief 获取当前本地时间
 *
 * @param time OUT 当前本地时间
 */
inline
void getCurrentLocalTime(char* _TimeBuffer)
{
	time_t now = system_clock::to_time_t(system_clock::now());
	struct tm* tmNow(localtime(&now));
	snprintf(_TimeBuffer, 20, "%d-%02d-%02d %02d:%02d:%02d",
		(int)tmNow->tm_year + 1900, (int)tmNow->tm_mon + 1, (int)tmNow->tm_mday,
		(int)tmNow->tm_hour, (int)tmNow->tm_min, (int)tmNow->tm_sec);
}

/* 以键值对形式存储日志等级对应的宽字符串 */
static const std::unordered_map<LOGLEVEL, const wchar_t*> LOGLEVEL_WSTRING
{
	std::pair<LOGLEVEL, const wchar_t*>(LOG_LEVEL_ERROR,   L"ERROR"),
	std::pair<LOGLEVEL, const wchar_t*>(LOG_LEVEL_WARNING, L"WARNING"),
	std::pair<LOGLEVEL, const wchar_t*>(LOG_LEVEL_DEBUG,   L"DEBUG"),
	std::pair<LOGLEVEL, const wchar_t*>(LOG_LEVEL_INFO,    L"INFO")
};

/* 日志类 */
class Log
{
	friend class std::shared_ptr<Log>;

public:
	~Log() = default;
	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;

	/**
	 * @brief 初始化日志对象
	 * 
	 * @param _LogLevel     日志等级
	 * @param _LogTarget    日志输出目标，命令行或者文件系统。
	 */
	static void Init
	(
		LOGLEVEL _LogLevel,
		LOGTARGET _LogTarget,
		std::wstring _Path = m_wstrLogFile
	);
	/**
	 * @brief 记录日志
	 * 
	 * @param   _LogLevel    日志等级
	 * @param   _FileName    函数所在文件名
	 * @param   _Function    函数名
	 * @param _LineNumber    行号
	 * @param     _Format    格式化
	 * @param ...            参数列表
	 */
	static void __cdecl writeLog
	(
		const LOGLEVEL _LogLevel,
		const wchar_t* _FileName,
		const wchar_t* _Function,
		const uint   _LineNumber,
		const char*      _Format,
		...
	);
	/**
	 * @brief 从文件中读取日志
	 * 
	 * @param _LogTable    用于存储从文件读出的日志
	 * @return true        日志读取成功 
	 * @return false       日志读取失败
	 */
	static bool getLogFromFile(std::vector<std::wstring>& _LogTable);
	/* 输出日志 */
	static void outputToTarget();
	/**
	 * @brief 获取日志实例
	 * 
	 * @return std::shared_ptr<Log> 当前日志对象的指针
	 */
	static std::shared_ptr<Log> Instance() noexcept
	{
		// 保证初始化函数唯一执行
		std::call_once(m_ResourceFlag, Init, m_LogLevel, m_LogTarget, m_wstrLogFile);
		return m_Log;
	}
	/* 获取Log等级 */
	static LOGLEVEL getLogLevel() noexcept { return m_LogLevel; }
	/* 设置Log等级 */
	static void setLogLevel(LOGLEVEL _LogLevel) noexcept { m_LogLevel = _LogLevel; }
	/* 获取Log输出位置 */
	static LOGTARGET getLogTarget() noexcept { return m_LogTarget; }
	/* 设置Log输出位置 */
	static void setLogTarget(LOGTARGET _LogTarget) noexcept { m_LogTarget = _LogTarget; }
	/* 获取Log输出文件路径 */
	static std::wstring getLogFile() noexcept { return m_wstrLogFile; }
	/* 设置Log输出文件路径 */
	static void setLogFile(const std::wstring& _Path) noexcept { m_wstrLogFile = _Path; }

protected:
	Log() = default;

private:
	static std::shared_ptr<Log>    m_Log;              // 唯一实例
	static std::wstring            m_wstrLogBuffer;    // 存储Log
	static std::wstring            m_wstrLogFile;      // Log输出文件夹
	static LOGLEVEL                m_LogLevel;         // Log等级
	static LOGTARGET               m_LogTarget;        // Log输出位置
	static std::once_flag          m_ResourceFlag;     // 用于初始化线程同步
	static std::shared_mutex       m_LogMutex;         // 读写互斥
};

#endif // _LOG_HPP_
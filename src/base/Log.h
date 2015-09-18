#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <sstream>
#include <errno.h>
#include <stdio.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//! MSVC >= VS2005, 使用 LOGI, LOGD, LOG_DEBUG, LOG_STREAM ...
#if _MSC_VER >= 1400 //MSVC >= VS2005
#define LOG_FORMAT_INPUT_ENABLE
#endif

#ifndef WIN32
#define LOG_FORMAT_INPUT_ENABLE
#endif

const int LOG_MAIN_LOGGER_ID = 0; //! the main logger id. 

//! 主日志名字 
const char*const LOG_MAIN_LOGGER_KEY = "Main";



//! 日志等级
enum ENUM_LOG_LEVEL
{
	LOG_LEVEL_TRACE = 0,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_ALARM,
	LOG_LEVEL_FATAL,
};

//////////////////////////////////////////////////////////////////////////
//! -----------------默认日志配置.-----------
//////////////////////////////////////////////////////////////////////////
//! 最大日志数量
const int LOG_LOGGER_MAX = 10;

//! 单条日志最大字节数.
const int LOG_LOG_BUF_SIZE = 1024;

//! 是否同步输出
const bool LOG_SYNCHRONOUS_OUTPUT = true;

//! 输出到windows调试窗口（win32专用，非控制台程序使用）
const bool LOG_ALL_DEBUGOUTPUT_DISPLAY = false;

//! 默认日志输出文件目录
const char* const LOG_DEFAULT_PATH = "./log/";

//! 默认日志等级
const int LOG_DEFAULT_LEVEL = LOG_LEVEL_DEBUG;

//! 是否输出到控制台
const bool LOG_DEFAULT_OUTCONSOLE = true;

//! 是否输出到文件
const bool LOG_DEFAULT_OUTFILE = false;

//! default logger month dir used status
const bool LOG_DEFAULT_MONTHDIR = false;

//! 日志文件大小 M byte.
const int LOG_DEFAULT_LIMITSIZE = 80;

//! 日志风格 (file name and line number) 
const bool LOG_DEFAULT_SHOWSUFFIX = true;


///////////////////////////////////////////////////////////////////////////
namespace logger {

	class ILogManager
	{
	public:
		ILogManager() {};
		virtual ~ILogManager() {};

		//! 单例
		static ILogManager * get();

		//! 在ILogManager::Start之前调用
		//virtual bool config(const char * configPath) = 0;
		//virtual bool configFromString(const char * configContent) = 0;

		//! 在ILogManager::Start之前调用
		virtual int createLogger(const char* key) = 0;

		//! 启动日志线程
		virtual bool start() = 0;

		//! 一般不需要外部调用
		virtual bool stop() = 0;

		//! 
		virtual int findLogger(const char* key) = 0;

		// 写日之前过滤
		virtual bool prePushLog(int id, int level) = 0;

		//线程安全
		virtual bool pushLog(int id, int level, const char * log, const char * file = NULL, int line = 0) = 0;

		//! 设置属性, 线程安全.
		virtual bool enableLogger(int id, bool enable) = 0;
		virtual bool setLoggerName(int id, const char * name) = 0;
		virtual bool setLoggerPath(int id, const char * path) = 0;
		virtual bool setLoggerLevel(int id, int nLevel) = 0;
		virtual bool setLoggerFileLine(int id, bool enable) = 0;
		virtual bool setLoggerOutConsole(int id, bool enable) = 0;
		virtual bool setLoggerOutFile(int id, bool enable) = 0;
		virtual bool setLoggerLimitsize(int id, unsigned int limitsize) = 0;
		virtual bool setLoggerMonthdir(int id, bool enable) = 0;
		virtual bool setLoggerSync(bool enable) = 0;

		//! 状态统计, 线程安全.
		virtual bool isLoggerEnable(int id) = 0;
		virtual unsigned long long getStatusTotalWriteCount() = 0;
		virtual unsigned long long getStatusTotalWriteBytes() = 0;
		virtual unsigned long long getStatusWaitingCount() = 0;
		virtual unsigned int getStatusActiveLoggers() = 0;
	};

	class LogStream;
	class LogBinary;
}



//! base micro.
#define LOG_STREAM(id, level, log)\
{\
	if (logger::ILogManager::get()->prePushLog(id,level)) \
					{\
		char logBuf[LOG_LOG_BUF_SIZE];\
		logger::LogStream ss(logBuf, LOG_LOG_BUF_SIZE);\
		ss << log;\
		logger::ILogManager::get()->pushLog(id, level, logBuf, __FILE__, __LINE__);\
					}\
}


//! fast micro
#define LOG_TRACE(id, log) LOG_STREAM(id, LOG_LEVEL_TRACE, log)
#define LOG_DEBUG(id, log) LOG_STREAM(id, LOG_LEVEL_DEBUG, log)
#define LOG_INFO(id, log)  LOG_STREAM(id, LOG_LEVEL_INFO, log)
#define LOG_WARN(id, log)  LOG_STREAM(id, LOG_LEVEL_WARN, log)
#define LOG_ERROR(id, log) LOG_STREAM(id, LOG_LEVEL_ERROR, log)
#define LOG_ALARM(id, log) LOG_STREAM(id, LOG_LEVEL_ALARM, log)
#define LOG_FATAL(id, log) LOG_STREAM(id, LOG_LEVEL_FATAL, log)

//! super micro.
#define LOGT( log ) LOG_TRACE(LOG_MAIN_LOGGER_ID, log )
#define LOGD( log ) LOG_DEBUG(LOG_MAIN_LOGGER_ID, log )
#define LOGI( log ) LOG_INFO(LOG_MAIN_LOGGER_ID, log )
#define LOGW( log ) LOG_WARN(LOG_MAIN_LOGGER_ID, log )
#define LOGE( log ) LOG_ERROR(LOG_MAIN_LOGGER_ID, log )
#define LOGA( log ) LOG_ALARM(LOG_MAIN_LOGGER_ID, log )
#define LOGF( log ) LOG_FATAL(LOG_MAIN_LOGGER_ID, log )


//! format input log.
#ifdef LOG_FORMAT_INPUT_ENABLE
#ifdef WIN32
#define LOG_FORMAT(id, level, logformat, ...) \
{ \
    auto log = logger::ILogManager::get(); \
	if (log->prePushLog(id,level)) \
					{\
		char logbuf[LOG_LOG_BUF_SIZE]; \
		_snprintf_s(logbuf, LOG_LOG_BUF_SIZE, _TRUNCATE, logformat, ##__VA_ARGS__); \
		log->pushLog(id, level, logbuf, __FILE__, __LINE__); \
					}\
 }
#else
#define LOG_FORMAT(id, level, logformat, ...) \
{ \
    auto log = logger::ILogManager::get(); \
	if (log->prePushLog(id,level)) \
					{\
        char logbuf[LOG_LOG_BUF_SIZE]; \
        snprintf(logbuf, LOG_LOG_BUF_SIZE,logformat, ##__VA_ARGS__); \
        log->pushLog(id, level, logbuf, __FILE__, __LINE__); \
					} \
}
#endif
//!format string
#define LOGFMT_TRACE(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define LOGFMT_DEBUG(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOGFMT_INFO(id, fmt, ...)   LOG_FORMAT(id, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOGFMT_WARN(id, fmt, ...)   LOG_FORMAT(id, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOGFMT_ERROR(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOGFMT_ALARM(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_ALARM, fmt, ##__VA_ARGS__)
#define LOGFMT_FATAL(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#define LOGFMTT( fmt, ...) LOGFMT_TRACE(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTD( fmt, ...) LOGFMT_DEBUG(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTI( fmt, ...) LOGFMT_INFO(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTW( fmt, ...) LOGFMT_WARN(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTE( fmt, ...) LOGFMT_ERROR(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTA( fmt, ...) LOGFMT_ALARM(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTF( fmt, ...) LOGFMT_FATAL(LOG_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#else
inline void empty_log_format_function1(int id, const char*, ...) {}
inline void empty_log_format_function2(const char*, ...) {}
#define LOGFMT_TRACE empty_log_format_function1
#define LOGFMT_DEBUG LOGFMT_TRACE
#define LOGFMT_INFO LOGFMT_TRACE
#define LOGFMT_WARN LOGFMT_TRACE
#define LOGFMT_ERROR LOGFMT_TRACE
#define LOGFMT_ALARM LOGFMT_TRACE
#define LOGFMT_FATAL LOGFMT_TRACE
#define LOGFMTT empty_log_format_function2
#define LOGFMTD LOGFMTT
#define LOGFMTI LOGFMTT
#define LOGFMTW LOGFMTT
#define LOGFMTE LOGFMTT
#define LOGFMTA LOGFMTT
#define LOGFMTF LOGFMTT
#endif


namespace logger {

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif

	class LogBinary
	{
	public:
		LogBinary(const char * buf, int len) :
			_buf(buf),
			_len(len)
		{
		}
		int  _len;
		const char * _buf;
	};

	class LogStream
	{
	public:
		inline LogStream(char * buf, int len);
		inline int getCurrentLen() { return (int)(_cur - _begin); }
	private:
		template<class T>
		inline LogStream & writeData(const char * ft, T t);
		inline LogStream & writeLongLong(long long t);
		inline LogStream & writeULongLong(unsigned long long t);
		inline LogStream & writePointer(const void * t);
		inline LogStream & writeString(const wchar_t* t) { return writeData("%s", t); }
		inline LogStream & writeWString(const wchar_t* t);
		inline LogStream & writeBinary(const LogBinary & t);
	public:
		inline LogStream & operator <<(const void * t) { return  writePointer(t); }

		inline LogStream & operator <<(const char * t) { return writeData("%s", t); }
#ifdef WIN32
		inline LogStream & operator <<(const wchar_t * t) { return writeWString(t); }
#endif
		inline LogStream & operator <<(bool t) { return (t ? writeData("%s", "true") : writeData("%s", "false")); }

		inline LogStream & operator <<(char t) { return writeData("%c", t); }

		inline LogStream & operator <<(unsigned char t) { return writeData("%u", (unsigned int)t); }

		inline LogStream & operator <<(short t) { return writeData("%d", (int)t); }

		inline LogStream & operator <<(unsigned short t) { return writeData("%u", (unsigned int)t); }

		inline LogStream & operator <<(int t) { return writeData("%d", t); }

		inline LogStream & operator <<(unsigned int t) { return writeData("%u", t); }

		inline LogStream & operator <<(long t) { return writeLongLong(t); }

		inline LogStream & operator <<(unsigned long t) { return writeULongLong(t); }

		inline LogStream & operator <<(long long t) { return writeLongLong(t); }

		inline LogStream & operator <<(unsigned long long t) { return writeULongLong(t); }

		inline LogStream & operator <<(float t) { return writeData("%.4f", t); }

		inline LogStream & operator <<(double t) { return writeData("%.4lf", t); }

		template<class _Elem, class _Traits, class _Alloc> //support std::string, std::wstring
		inline LogStream & operator <<(const std::basic_string<_Elem, _Traits, _Alloc> & t) { return *this << t.c_str(); }

		inline LogStream & operator << (const logger::LogBinary & binary) { return writeBinary(binary); }

	private:
		LogStream() {}
		LogStream(LogStream &) {}
		char *  _begin;
		char *  _end;
		char *  _cur;
	};

	inline LogStream::LogStream(char * buf, int len) :
		_begin(buf),
		_end(buf + len),
		_cur(_begin)
	{
	}

	template<class T>
	inline LogStream& LogStream::writeData(const char * ft, T t)
	{
		if (_cur < _end)
		{
			int len = 0;
			int count = (int)(_end - _cur);
#ifdef WIN32
			len = _snprintf(_cur, count, ft, t);
			if (len == count || (len == -1 && errno == ERANGE))
			{
				len = count;
				*(_end - 1) = '\0';
			}
			else if (len < 0)
			{
				*_cur = '\0';
				len = 0;
			}
#else
			len = snprintf(_cur, count, ft, t);
			if (len < 0)
			{
				*_cur = '\0';
				len = 0;
			}
			else if (len >= count)
			{
				len = count;
				*(_end - 1) = '\0';
			}
#endif
			_cur += len;
		}
		return *this;
	}

	inline LogStream & LogStream::writeLongLong(long long t)
	{
#ifdef WIN32  
		writeData("%I64d", t);
#else
		writeData("%lld", t);
#endif
		return *this;
	}

	inline LogStream & LogStream::writeULongLong(unsigned long long t)
	{
#ifdef WIN32  
		writeData("%I64u", t);
#else
		writeData("%llu", t);
#endif
		return *this;
	}

	inline LogStream & LogStream::writePointer(const void * t)
	{
#ifdef WIN32
		sizeof(t) == 8 ? writeData("%016I64x", (unsigned long long)t) : writeData("%08I64x", (unsigned long long)t);
#else
		sizeof(t) == 8 ? writeData("%016llx", (unsigned long long)t) : writeData("%08llx", (unsigned long long)t);
#endif
		return *this;
	}

	inline LogStream & LogStream::writeBinary(const LogBinary & t)
	{
		writeData("%s", "\r\n\t[");
		for (int i = 0; i < t._len; i++)
		{
			if (i % 16 == 0)
			{
				writeData("%s", "\r\n\t");
				*this << (void*)(t._buf + i);
				writeData("%s", ": ");
			}
			writeData("%02x ", (unsigned char)t._buf[i]);
		}
		writeData("%s", "\r\n\t]\r\n\t");
		return *this;
	}

	inline logger::LogStream & logger::LogStream::writeWString(const wchar_t* t)
	{
#ifdef WIN32
		DWORD dwLen = WideCharToMultiByte(CP_ACP, 0, t, -1, NULL, 0, NULL, NULL);
		if (dwLen < LOG_LOG_BUF_SIZE)
		{
			std::string str;
			str.resize(dwLen, '\0');
			dwLen = WideCharToMultiByte(CP_ACP, 0, t, -1, &str[0], dwLen, NULL, NULL);
			if (dwLen > 0)
			{
				writeData("%s", str.c_str());
			}
		}
#else
		//not support
#endif
		return *this;
	}


#ifdef WIN32
#pragma warning(pop)
#endif

}

#endif

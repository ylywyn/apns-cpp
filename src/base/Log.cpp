#include "Log.h"
#include <time.h>
#include <string.h>
#include <vector>
#include <map>
#include <list>
#include <queue>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <thread>
#include <condition_variable>

#ifdef WIN32
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h> //?
#endif


#ifdef __APPLE__
#include <dispatch/dispatch.h>
#include <libproc.h>
#endif

namespace logger {
	const int LOG4Z_INVALID_LOGGER_ID = -1;
	static const char *const LOG_STRING[] =
	{
		"TRACE",
		"DEBUG",
		"INFO ",
		"WARN ",
		"ERROR",
		"ALARM",
		"FATAL",
	};

#ifdef WIN32
	const static WORD LOG_COLOR[LOG_LEVEL_FATAL + 1] = {
		0,
		0,
		FOREGROUND_BLUE | FOREGROUND_GREEN,
		FOREGROUND_GREEN | FOREGROUND_RED,
		FOREGROUND_RED,
		FOREGROUND_GREEN,
		FOREGROUND_RED | FOREGROUND_BLUE };
#else

	const static char LOG_COLOR[LOG_LEVEL_FATAL + 1][50] = {
		"\e[0m",
		"\e[0m",
		"\e[34m\e[1m",//hight blue
		"\e[33m", //yellow
		"\e[31m", //red
		"\e[32m", //green
		"\e[35m" };
#endif


	class ThreadHelper
	{
	public:
		ThreadHelper() {  }
		virtual ~ThreadHelper() {}

	public:
		inline void start()
		{
            std::thread temp(threadProc, (void *) this);
            _t = std::move(temp);
		}

		inline void wait()
		{
			_t.join();
		}

		virtual void run() = 0;

	private:
		static void * threadProc(void * pParam)
		{
			ThreadHelper * p = (ThreadHelper *)pParam;
			p->run();
			return NULL;
		}
	private:
        std::thread _t;
	};


	//////////////////////////////////////////////////////////////////////////
	//! LogFileHandler
	class LogFileHandler
	{
	public:
		LogFileHandler() { _file = NULL; }
		~LogFileHandler() { close(); }
		inline bool isOpen() { return _file != NULL; }
		inline bool open(const char *path, const char * mod)
		{
			if (_file != NULL) { fclose(_file); _file = NULL; }
			_file = fopen(path, mod);
			return _file != NULL;
		}
		inline void close()
		{
			if (_file != NULL) { fclose(_file); _file = NULL; }
		}
		inline void write(const char * data, size_t len)
		{
			if (_file && len > 0)
			{
				if (fwrite(data, 1, len, _file) != len)
				{
					close();
				}
			}
		}
		inline void flush() { if (_file) fflush(_file); }

		inline std::string readLine()
		{
			char buf[500] = { 0 };
			if (_file && fgets(buf, 500, _file) != NULL)
			{
				return std::string(buf);
			}
			return std::string();
		}

		const std::string readContent()
		{
			std::string content;
			if (!_file)
			{
				return content;
			}

			fseek(_file, 0, SEEK_END);
			long filelen = ftell(_file);
			if (filelen > 2 * 1024 * 1024 || filelen <= 0)
			{
				return content;
			}
			content.resize(filelen);

			fseek(_file, 0, SEEK_SET);
			size_t n = fread(&content[0], 1, filelen, _file);
			while (n < (size_t)filelen)
			{
				n += fread(&content[0] + n, 1, filelen - n, _file);
			}
			return std::move(content);
		}

	public:
		FILE *_file;
	};


	template<class T>
	class BlockingQueue
	{
	public:
		BlockingQueue() :wait_(true) { cache_.reserve(50); }
		~BlockingQueue()
		{
			{
				std::unique_lock<std::mutex> lock(mtx_);
				while (!queue_.empty())
				{
					T* t = queue_.back();
					queue_.pop_back();
					delete t;
				}
			}

			{
				std::unique_lock<std::mutex> lock(mtx_cache_);
				int n = cache_.size();
				for (int i = 0; i < n; ++i)
				{
					delete cache_[i];
				}
			}
		}

		bool Empty()
		{
			std::unique_lock<std::mutex> lock(mtx_);
			return queue_.empty();
		}

		T* Take()
		{
			T* t = NULL;
			std::unique_lock<std::mutex> lock(mtx_);
			while (queue_.empty() && wait_)
			{
				not_empty_.wait(lock);
			}

			if (wait_)
			{
				t = queue_.front();
				queue_.pop_front();
			}
			return t;
		}

		void Put(T *t)
		{
			std::unique_lock<std::mutex> lock(mtx_);
			queue_.push_back(t);
			not_empty_.notify_one();
		}

		T* GetCache()
		{
			T* t = NULL;

			std::unique_lock<std::mutex> lock(mtx_cache_);
			if (cache_.empty())
			{
				return t;
			}

			t = cache_.back();
			cache_.pop_back();
			return t;
		}

		void PutCache(T *&t)
		{
			std::unique_lock<std::mutex> lock(mtx_cache_);
			cache_.push_back(t);
			t = NULL;
		}

		void StopBlock()
		{
			std::unique_lock<std::mutex> lock(mtx_);
			wait_ = false;
			not_empty_.notify_all();
		}
	private:
		BlockingQueue(const BlockingQueue&);
		const BlockingQueue& operator=(const BlockingQueue&);
	private:
		bool wait_;
        std::mutex mtx_;
        std::mutex mtx_cache_;
        std::condition_variable not_empty_;

		std::deque<T*>queue_;
		std::vector<T*>cache_;
	};

	//////////////////////////////////////////////////////////////////////////
	//! LogData
	struct LogData
	{
		int _id;
		int	_level;
		time_t _time;		//create time
		unsigned int _precise; //create time 
		int  _contentLen;
		char _content[LOG_LOG_BUF_SIZE];
	};

	//////////////////////////////////////////////////////////////////////////
	//! LoggerInfo
	struct LoggerInfo
	{
		//! attribute
		std::string _key;   // key
		std::string _name;	// name.
		std::string _path;	//path for log file.
		int  _level;		//filter level
		bool _display;		//display to screen 
		bool _outfile;		//output to file
		bool _monthdir;		//create directory per month 
		unsigned int _limitsize; //limit file's size, unit Million byte.
		bool _enable;		//logger is enable 
		bool _fileLine;		//enable/disable the log's suffix.(file name:line number)

		//! runtime info
		time_t _curFileCreateTime;	//file create time
		unsigned int _curFileIndex; //rolling file index
		unsigned int _curWriteLen;  //current file length
		LogFileHandler	_handle;	//file handle.

		LoggerInfo()
		{
			_enable = false;
			_path = LOG_DEFAULT_PATH;
			_level = LOG_DEFAULT_LEVEL;
			_display = LOG_DEFAULT_OUTCONSOLE;
			_outfile = LOG_DEFAULT_OUTFILE;

			_monthdir = LOG_DEFAULT_MONTHDIR;
			_limitsize = LOG_DEFAULT_LIMITSIZE;
			_fileLine = LOG_DEFAULT_SHOWSUFFIX;

			_curFileCreateTime = 0;
			_curFileIndex = 0;
			_curWriteLen = 0;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	//! UTILITY
	static void sleepMillisecond(unsigned int ms);
	static void timeToTm(struct tm& tt, time_t t);
	static bool isSameDay(time_t t1, time_t t2);

	static void fixPath(std::string &path);
	static void trimLogConfig(std::string &str, std::string extIgnore = std::string());
	//static bool parseConfigFromString(std::string content, std::map<std::string, LoggerInfo> & outInfo);
	static std::pair<std::string, std::string> splitPairString(const std::string & str, const std::string & delimiter);


	static bool isDirectory(std::string path);
	static bool createRecursionDir(std::string path);
	static std::string getProcessID();
	static std::string getProcessName();

	//////////////////////////////////////////////////////////////////////////
	//! LogerManager
	class LogerManager : public ThreadHelper, public ILogManager
	{
	public:
		LogerManager();
		virtual ~LogerManager();

		//bool configFromStringImpl(std::string content, bool isUpdate);
		////! 读取配置文件并覆写
		//virtual bool config(const char* configPath);
		//virtual bool configFromString(const char* configContent);

		//! 覆写式创建
		virtual int createLogger(const char* key);
		virtual bool start();
		virtual bool stop();
		virtual bool prePushLog(int id, int level);
		virtual bool pushLog(int id, int level, const char * log, const char * file, int line);
		//! 查找ID
		virtual int findLogger(const char*  key);

		virtual bool enableLogger(int id, bool enable);
		virtual bool setLoggerName(int id, const char * name);
		virtual bool setLoggerPath(int id, const char * path);
		virtual bool setLoggerLevel(int id, int nLevel);
		virtual bool setLoggerFileLine(int id, bool enable);
		virtual bool setLoggerOutConsole(int id, bool enable);
		virtual bool setLoggerOutFile(int id, bool enable);
		virtual bool setLoggerLimitsize(int id, unsigned int limitsize);
		virtual bool setLoggerMonthdir(int id, bool enable);
		virtual bool setLoggerSync(bool enable);

		virtual bool isLoggerEnable(int id);
		virtual unsigned long long getStatusTotalWriteCount() { return _writeFileCounts; }
		virtual unsigned long long getStatusTotalWriteBytes() { return _writeFileBytes; }
		virtual unsigned long long getStatusWaitingCount() { return _pushLogs - _popLogs; }
		virtual unsigned int getStatusActiveLoggers();

	protected:
		void showColorText(const char *text, int level = LOG_LEVEL_DEBUG);
		bool openLogger(LogData * log);
		bool closeLogger(int id);
		bool popLog(LogData *& log);
		virtual void run();

	private:
		//! thread status.
		bool _runing;
		bool _sync_output;

		//! the process info.
		std::string _pid;
		std::string _proName;

		//! config file name
		std::string _configFile;

		//! logger id manager, [logger name]:[logger id].
		std::map<std::string, int> _ids;

		// the last used id of _loggers
		int   _lastId;
		LoggerInfo _loggers[LOG_LOGGER_MAX];


		//颜色控制台输出锁
        std::mutex _scLock;

		//! log queue
		BlockingQueue<LogData> _queue;

		//同步写日志的锁
        std::mutex	_logLock;

		//status statistics
		//write file
		unsigned long long _writeFileCounts;
		unsigned long long _writeFileBytes;

		//Log queue statistics
		unsigned long long _pushLogs;
		unsigned long long _popLogs;

	};



	LogerManager::LogerManager()
	{
		_runing = false;
		_sync_output = LOG_SYNCHRONOUS_OUTPUT;
		_lastId = LOG_MAIN_LOGGER_ID;

		_pushLogs = 0;
		_popLogs = 0;
		_writeFileCounts = 0;
		_writeFileBytes = 0;

		_pid = getProcessID();
		_proName = getProcessName();
		_loggers[LOG_MAIN_LOGGER_ID]._enable = true;
		_ids[LOG_MAIN_LOGGER_KEY] = LOG_MAIN_LOGGER_ID;
		_loggers[LOG_MAIN_LOGGER_ID]._key = LOG_MAIN_LOGGER_KEY;
		_loggers[LOG_MAIN_LOGGER_ID]._name = _proName;

	}
	LogerManager::~LogerManager()
	{
		stop();
	}


	void LogerManager::showColorText(const char *text, int level)
	{
		//printf本事是线程安全的， cout不是
		if (level <= LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
		{
			printf("%s", text);
			return;
		}
#ifndef WIN32
		printf("%s%s\e[0m", LOG_COLOR[level], text);
#else
		{
			std::lock_guard<std::mutex>lock(_scLock);
			HANDLE hStd = ::GetStdHandle(STD_OUTPUT_HANDLE);
			if (hStd == INVALID_HANDLE_VALUE) return;
			CONSOLE_SCREEN_BUFFER_INFO oldInfo;
			if (!GetConsoleScreenBufferInfo(hStd, &oldInfo))
			{
				return;
			}
			else
			{
				SetConsoleTextAttribute(hStd, LOG_COLOR[level]);
				printf("%s", text);
				SetConsoleTextAttribute(hStd, oldInfo.wAttributes);
			}
		}
#endif
		return;
	}

	//bool LogerManager::configFromStringImpl(std::string content, bool isUpdate)
	//{
	//	std::map<std::string, LoggerInfo> loggerMap;
	//	if (!parseConfigFromString(content, loggerMap))
	//	{
	//		std::cout << " !!! !!! !!! !!!" << std::endl;
	//		std::cout << " !!! !!! logger load config file error" << std::endl;
	//		std::cout << " !!! !!! !!! !!!" << std::endl;
	//		return false;
	//	}
	//	for (std::map<std::string, LoggerInfo>::iterator iter = loggerMap.begin(); iter != loggerMap.end(); ++iter)
	//	{
	//		int id = LOG4Z_INVALID_LOGGER_ID ;
	//		id = findLogger(iter->second._key.c_str());
	//		if (id == LOG4Z_INVALID_LOGGER_ID )
	//		{
	//			if (isUpdate)
	//			{
	//				continue;
	//			}
	//			else
	//			{
	//				id = createLogger(iter->second._key.c_str());
	//				if (id == LOG4Z_INVALID_LOGGER_ID )
	//				{
	//					continue;
	//				}
	//			}
	//		}
	//		enableLogger(id, iter->second._enable);
	//		setLoggerName(id, iter->second._name.c_str());
	//		setLoggerPath(id, iter->second._path.c_str());
	//		setLoggerLevel(id, iter->second._level);
	//		setLoggerFileLine(id, iter->second._fileLine);
	//		setLoggerOutConsole(id, iter->second._display);
	//		setLoggerOutFile(id, iter->second._outfile);
	//		setLoggerLimitsize(id, iter->second._limitsize);
	//		setLoggerMonthdir(id, iter->second._monthdir);
	//	}
	//	return true;
	//}

	////! read configure and create with overwriting
	//bool LogerManager::config(const char* configPath)
	//{
	//	if (!_configFile.empty())
	//	{
	//		std::cout << " !!! !!! !!! !!!" << std::endl;
	//		std::cout << " !!! !!! logger configure error: too many calls to Config. the old config file=" << _configFile << ", the new config file=" << configPath << " !!! !!! " << std::endl;
	//		std::cout << " !!! !!! !!! !!!" << std::endl;
	//		return false;
	//	}
	//	_configFile = configPath;

	//	LogFileHandler f;
	//	f.open(_configFile.c_str(), "r");
	//	if (!f.isOpen())
	//	{
	//		std::cout << " !!! !!! !!! !!!" << std::endl;
	//		std::cout << " !!! !!! logger load config file error. filename=" << configPath << " !!! !!! " << std::endl;
	//		std::cout << " !!! !!! !!! !!!" << std::endl;
	//		return false;
	//	}
	//	return configFromStringImpl(f.readContent().c_str(), false);
	//}

	////! read configure and create with overwriting
	//bool LogerManager::configFromString(const char* configContent)
	//{
	//	return configFromStringImpl(configContent, false);
	//}

	//! create with overwriting
	int LogerManager::createLogger(const char* key)
	{
		if (key == NULL)
		{
			return LOG4Z_INVALID_LOGGER_ID;
		}

		std::string copyKey = key;
		trimLogConfig(copyKey);

		int newID = LOG4Z_INVALID_LOGGER_ID;
		{
			std::map<std::string, int>::iterator iter = _ids.find(copyKey);
			if (iter != _ids.end())
			{
				newID = iter->second;
			}
		}
		if (newID == LOG4Z_INVALID_LOGGER_ID)
		{
			if (_lastId + 1 >= LOG_LOGGER_MAX)
			{
				showColorText("logger: CreateLogger can not create|writeover, because int need < LOGGER_MAX! \r\n", LOG_LEVEL_FATAL);
				return LOG4Z_INVALID_LOGGER_ID;
			}
			newID = ++_lastId;
			_ids[copyKey] = newID;
			_loggers[newID]._enable = true;
			_loggers[newID]._key = copyKey;
			_loggers[newID]._name = copyKey;
		}

		return newID;
	}


	bool LogerManager::start()
	{
		if (_runing)
		{
			return false;
		}
		if (_sync_output)
		{
			_runing = true;
		}
		else
		{
			ThreadHelper::start();
		}
		return  true;
	}
	bool LogerManager::stop()
	{
		if (_runing == true)
		{
			_runing = false;
			if (_sync_output)
			{
				for (int i = 0; i < _lastId; ++i)
				{
					closeLogger(i);
				}
			}
			else
			{
				if (_queue.Empty())
				{
					_queue.StopBlock();
				}
				wait();
			}
			return true;
		}
		return false;
	}
	bool LogerManager::prePushLog(int id, int level)
	{
		if (id < 0 || id > _lastId || !_runing || !_loggers[id]._enable)
		{
			return false;
		}
		if (level < _loggers[id]._level)
		{
			return false;
		}
		return true;
	}
	bool LogerManager::pushLog(int id, int level, const char * log, const char * file, int line)
	{
		//create log data
		LogData * pLog = _queue.GetCache();
		if (pLog == NULL)
		{
			pLog = new LogData;
		}

		pLog->_id = id;
		pLog->_level = level;

		//append precise time to log
		{
#ifdef WIN32
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);
			unsigned long long now = ft.dwHighDateTime;
			now <<= 32;
			now |= ft.dwLowDateTime;
			now /= 10;
			now -= 11644473600000000ULL;
			now /= 1000;
			pLog->_time = now / 1000;
			pLog->_precise = (unsigned int)(now % 1000);
#else
			struct timeval tm;
			gettimeofday(&tm, NULL);
			pLog->_time = tm.tv_sec;
			pLog->_precise = tm.tv_usec / 1000;
#endif
		}

		//format log
		{
			tm tt;
			timeToTm(tt, pLog->_time);
			if (file == NULL || !_loggers[pLog->_id]._fileLine)
			{
#ifdef WIN32
				int ret = _snprintf_s(pLog->_content, LOG_LOG_BUF_SIZE, _TRUNCATE, "%d-%02d-%02d %02d:%02d:%02d.%03d %s %s \r\n",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, pLog->_precise,
					LOG_STRING[pLog->_level], log);
				if (ret == -1)
				{
					ret = LOG_LOG_BUF_SIZE - 1;
				}
				pLog->_contentLen = ret;
#else
				int ret = snprintf(pLog->_content, LOG_LOG_BUF_SIZE, "%d-%02d-%02d %02d:%02d:%02d.%03d %s %s \r\n",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, pLog->_precise,
					LOG_STRING[pLog->_level], log);
				if (ret == -1)
				{
					ret = 0;
				}
				if (ret >= LOG_LOG_BUF_SIZE)
				{
					ret = LOG_LOG_BUF_SIZE - 1;
				}

				pLog->_contentLen = ret;
#endif
			}
			else
			{
				const char * pNameBegin = file + strlen(file);
				do
				{
					if (*pNameBegin == '\\' || *pNameBegin == '/') { pNameBegin++; break; }
					if (pNameBegin == file) { break; }
					pNameBegin--;
				} while (true);


#ifdef WIN32
				int ret = _snprintf_s(pLog->_content, LOG_LOG_BUF_SIZE, _TRUNCATE, "%d-%02d-%02d %02d:%02d:%02d.%03d %s %s:%d] %s\r\n",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, pLog->_precise,
					LOG_STRING[pLog->_level], pNameBegin, line, log);
				if (ret == -1)
				{
					ret = LOG_LOG_BUF_SIZE - 1;
				}
				pLog->_contentLen = ret;
#else
				int ret = snprintf(pLog->_content, LOG_LOG_BUF_SIZE, "%d-%02d-%02d %02d:%02d:%02d.%03d %s %s:%d] %s\r\n",
					tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, pLog->_precise,
					LOG_STRING[pLog->_level], pNameBegin, line, log);
				if (ret == -1)
				{
					ret = 0;
				}
				if (ret >= LOG_LOG_BUF_SIZE)
				{
					ret = LOG_LOG_BUF_SIZE - 1;
				}

				pLog->_contentLen = ret;
#endif
			}

			if (pLog->_contentLen >= 2)
			{
				pLog->_content[pLog->_contentLen - 2] = '\r';
				pLog->_content[pLog->_contentLen - 1] = '\n';
			}

		}

		if (_loggers[pLog->_id]._display && _sync_output)
		{
			showColorText(pLog->_content, pLog->_level);
		}

		if (LOG_ALL_DEBUGOUTPUT_DISPLAY && _sync_output)
		{
#ifdef WIN32
			OutputDebugStringA(pLog->_content);
#endif
		}

		if (_loggers[pLog->_id]._outfile && _sync_output)
		{
			std::lock_guard<std::mutex>lock(_logLock);
			if (openLogger(pLog))
			{
				_loggers[pLog->_id]._handle.write(pLog->_content, pLog->_contentLen);
				_loggers[pLog->_id]._handle.flush();
				_loggers[pLog->_id]._curWriteLen += (unsigned int)pLog->_contentLen;
				_writeFileCounts++;
				_writeFileBytes += pLog->_contentLen;
			}
		}

		if (_sync_output)
		{
			_queue.PutCache(pLog);
			return true;
		}

		//异步
		{
			_queue.Put(pLog);
			std::lock_guard<std::mutex>lock(_logLock);
			_pushLogs++;
		}
		return true;
	}

	//! 查找ID
	int LogerManager::findLogger(const char * key)
	{
		std::map<std::string, int>::iterator iter;
		iter = _ids.find(key);
		if (iter != _ids.end())
		{
			return iter->second;
		}
		return LOG4Z_INVALID_LOGGER_ID;
	}
	bool LogerManager::enableLogger(int id, bool enable)
	{
		if (id <0 || id > _lastId) return false;
		_loggers[id]._enable = enable;

		return true;
	}
	bool LogerManager::setLoggerLevel(int id, int level)
	{
		if (id <0 || id > _lastId || level < LOG_LEVEL_TRACE || level >LOG_LEVEL_FATAL) return false;
		_loggers[id]._level = level;
		return true;
	}
	bool LogerManager::setLoggerOutConsole(int id, bool enable)
	{
		if (id <0 || id > _lastId) return false;
		_loggers[id]._display = enable;
		return true;
	}
	bool LogerManager::setLoggerOutFile(int id, bool enable)
	{
		if (id <0 || id > _lastId) return false;
		_loggers[id]._outfile = enable;
		return true;
	}
	bool LogerManager::setLoggerMonthdir(int id, bool enable)
	{
		if (id <0 || id > _lastId) return false;
		_loggers[id]._monthdir = enable;
		return true;
	}
	bool LogerManager::setLoggerLimitsize(int id, unsigned int limitsize)
	{
		if (id <0 || id > _lastId) return false;
		if (limitsize == 0) { limitsize = (unsigned int)-1; }
		_loggers[id]._limitsize = limitsize;
		return true;
	}
	bool LogerManager::setLoggerFileLine(int id, bool enable)
	{
		if (id <0 || id > _lastId) return false;
		_loggers[id]._fileLine = enable;
		return true;
	}

	bool LogerManager::setLoggerName(int id, const char * name)
	{
		if (id <0 || id > _lastId) return false;
		//the name by main logger is the process name and it's can't change. 
		if (id == LOG_MAIN_LOGGER_ID) return false;

		if (name == NULL || strlen(name) == 0)
		{
			return false;
		}


		if (_loggers[id]._name != name)
		{
			_loggers[id]._name = name;
		}

		return true;
	}

	bool LogerManager::setLoggerPath(int id, const char * path)
	{
		if (id <0 || id > _lastId) return false;

		std::string copyPath;
		if (path == NULL || strlen(path) == 0)
		{
			copyPath = LOG_DEFAULT_PATH;
		}
		else
		{
			copyPath = path;
		}

		{
			char ch = copyPath.at(copyPath.length() - 1);
			if (ch != '\\' && ch != '/')
			{
				copyPath.append("/");
			}
		}


		if (copyPath != _loggers[id]._path)
		{
			_loggers[id]._path = copyPath;
		}
		return true;
	}
	bool LogerManager::setLoggerSync(bool enable)
	{
		_sync_output = enable;
		return true;
	}
	bool LogerManager::isLoggerEnable(int id)
	{
		if (id <0 || id > _lastId) return false;
		return _loggers[id]._enable;
	}

	unsigned int LogerManager::getStatusActiveLoggers()
	{
		unsigned int actives = 0;
		for (int i = 0; i <= _lastId; i++)
		{
			if (_loggers[i]._enable)
			{
				actives++;
			}
		}
		return actives;
	}


	bool LogerManager::openLogger(LogData * pLog)
	{
		int id = pLog->_id;
		if (id < 0 || id >_lastId)
		{
			return false;
		}

		LoggerInfo * pLogger = &_loggers[id];
		if (!pLogger->_enable || !pLogger->_outfile)
		{
			return false;
		}

		bool sameday = isSameDay(pLog->_time, pLogger->_curFileCreateTime);
		bool needChageFile = pLogger->_curWriteLen > pLogger->_limitsize * 1024 * 1024;
		if (!sameday || needChageFile)
		{
			if (!sameday)
			{
				pLogger->_curFileIndex = 0;
			}
			else
			{
				pLogger->_curFileIndex++;
			}
			if (pLogger->_handle.isOpen())
			{
				pLogger->_handle.close();
			}
		}
		if (!pLogger->_handle.isOpen())
		{
			pLogger->_curFileCreateTime = pLog->_time;
			pLogger->_curWriteLen = 0;

			tm t;
			timeToTm(t, pLogger->_curFileCreateTime);
			std::string name;
			std::string path;


			name = pLogger->_name;
			path = pLogger->_path;

			char buf[100] = { 0 };
			if (pLogger->_monthdir)
			{
				sprintf(buf, "%04d_%02d/", t.tm_year + 1900, t.tm_mon + 1);
				path += buf;
			}

			if (!isDirectory(path))
			{
				createRecursionDir(path);
			}

			sprintf(buf, "%s_%04d%02d%02d%02d%02d_%s_%03d.log",
				name.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
				t.tm_hour, t.tm_min, _pid.c_str(), pLogger->_curFileIndex);
			path += buf;
			pLogger->_handle.open(path.c_str(), "ab");
			if (!pLogger->_handle.isOpen())
			{
				pLogger->_outfile = false;
				return false;
			}
			return true;
		}
		return true;
	}
	bool LogerManager::closeLogger(int id)
	{
		if (id < 0 || id >_lastId)
		{
			showColorText("logger: closeLogger can not close, invalide logger id! \r\n", LOG_LEVEL_FATAL);
			return false;
		}
		LoggerInfo * pLogger = &_loggers[id];
		if (pLogger->_handle.isOpen())
		{
			pLogger->_handle.close();
			return true;
		}
		return false;
	}
	bool LogerManager::popLog(LogData *& log)
	{
		log = NULL;
		log = _queue.Take();
		return log != NULL;
	}

	void LogerManager::run()
	{
		_runing = true;
		LogData * pLog = NULL;
		while (_runing)
		{
			while (popLog(pLog))
			{
				//
				_popLogs++;
				//discard
				LoggerInfo & curLogger = _loggers[pLog->_id];
				if (!curLogger._enable)
				{
					_queue.PutCache(pLog);
					continue;
				}

				//控制台
				if (curLogger._display)
				{
					showColorText(pLog->_content, pLog->_level);
				}

				//调试窗口
				if (LOG_ALL_DEBUGOUTPUT_DISPLAY)
				{
#ifdef WIN32
					OutputDebugStringA(pLog->_content);
#endif
				}

				//文件
				if (curLogger._outfile)
				{
					if (!openLogger(pLog))
					{
						_queue.PutCache(pLog);
						continue;
					}

					//写入，刷新
					curLogger._handle.write(pLog->_content, pLog->_contentLen);
					curLogger._handle.flush();

					curLogger._curWriteLen += (unsigned int)pLog->_contentLen;
					_writeFileCounts++;
					_writeFileBytes += pLog->_contentLen;
				}

				//放回
				if (pLog != NULL)
				{
					_queue.PutCache(pLog);
				}
			}


			for (int i = 0; i <= _lastId; i++)
			{
				if (_loggers[i]._enable)
				{
					_loggers[i]._handle.flush();
					closeLogger(i);
					_loggers[i]._enable = false;
				}
			}

		}
	}

	//////////////////////////////////////////////////////////////////////////
	//ILogManager::getInstance
	ILogManager * ILogManager::get()
	{
		static LogerManager m;
		return &m;
	}



	//////////////////////////////////////////////////////////////////////////
	//! utility

	void sleepMillisecond(unsigned int ms)
	{
#ifdef WIN32
		::Sleep(ms);
#else
		usleep(1000 * ms);
#endif
	}

	void timeToTm(struct tm& tt, time_t t)
	{
#ifdef WIN32
#if _MSC_VER < 1400 //VS2003
		tt = *localtime(&t)
#else //vs2005->vs2013->
		localtime_s(&tt, &t);
#endif
#else //linux
		localtime_r(&t, &tt);
#endif
	}

	bool isSameDay(time_t t1, time_t t2)
	{
		tm tm1, tm2;
		timeToTm(tm1, t1);
		timeToTm(tm2, t2);
		if (tm1.tm_yday == tm2.tm_yday && tm1.tm_year == tm2.tm_year)
		{
			return true;
		}
		return false;
	}


	void fixPath(std::string &path)
	{
		if (path.empty()) { return; }
		for (std::string::iterator iter = path.begin(); iter != path.end(); ++iter)
		{
			if (*iter == '\\') { *iter = '/'; }
		}
		if (path[path.length() - 1] != '/') { path.append("/"); }
	}

	static void trimLogConfig(std::string &str, std::string extIgnore)
	{
		if (str.empty()) { return; }
		extIgnore += "\r\n\t ";
		int length = (int)str.length();
		int posBegin = 0;
		int posEnd = 0;

		//trim utf8 file bom
		if (str.length() >= 3
			&& (unsigned char)str[0] == 0xef
			&& (unsigned char)str[1] == 0xbb
			&& (unsigned char)str[2] == 0xbf)
		{
			posBegin = 3;
		}

		//trim character 
		for (int i = posBegin; i < length; i++)
		{
			bool bCheck = false;
			for (int j = 0; j < (int)extIgnore.length(); j++)
			{
				if (str[i] == extIgnore[j])
				{
					bCheck = true;
				}
			}
			if (bCheck)
			{
				if (i == posBegin)
				{
					posBegin++;
				}
			}
			else
			{
				posEnd = i + 1;
			}
		}
		if (posBegin < posEnd)
		{
			str = str.substr(posBegin, posEnd - posBegin);
		}
		else
		{
			str.clear();
		}
	}

	//split
	static std::pair<std::string, std::string> splitPairString(const std::string & str, const std::string & delimiter)
	{
		std::string::size_type pos = str.find(delimiter.c_str());
		if (pos == std::string::npos)
		{
			return std::make_pair(str, "");
		}
		return std::make_pair(str.substr(0, pos), str.substr(pos + delimiter.length()));
	}

	//static bool parseConfigLine(const std::string& line, int curLineNum, std::string & key, std::map<std::string, LoggerInfo> & outInfo)
	//{
	//	std::pair<std::string, std::string> kv = splitPairString(line, "=");
	//	if (kv.first.empty())
	//	{
	//		return false;
	//	}

	//	trimLogConfig(kv.first);
	//	trimLogConfig(kv.second);
	//	if (kv.first.empty() || kv.first.at(0) == '#')
	//	{
	//		return true;
	//	}

	//	if (kv.first.at(0) == '[')
	//	{
	//		trimLogConfig(kv.first, "[]");
	//		key = kv.first;
	//		{
	//			std::string tmpstr = kv.first;
	//			std::transform(tmpstr.begin(), tmpstr.end(), tmpstr.begin(), ::tolower);
	//			if (tmpstr == "main")
	//			{
	//				key = "Main";
	//			}
	//		}
	//		std::map<std::string, LoggerInfo>::iterator iter = outInfo.find(key);
	//		if (iter == outInfo.end())
	//		{
	//			LoggerInfo li;
	//			li._enable = true;
	//			li._key = key;
	//			li._name = key;
	//			outInfo.insert(std::make_pair(li._key, li));
	//		}
	//		else
	//		{
	//			std::cout << "logger configure warning: duplicate logger key:[" << key << "] at line:" << curLineNum << std::endl;
	//		}
	//		return true;
	//	}
	//	trimLogConfig(kv.first);
	//	trimLogConfig(kv.second);
	//	std::map<std::string, LoggerInfo>::iterator iter = outInfo.find(key);
	//	if (iter == outInfo.end())
	//	{
	//		std::cout << "logger configure warning: not found current logger name:[" << key << "] at line:" << curLineNum
	//			<< ", key=" << kv.first << ", value=" << kv.second << std::endl;
	//		return true;
	//	}
	//	std::transform(kv.first.begin(), kv.first.end(), kv.first.begin(), ::tolower);
	//	//! path
	//	if (kv.first == "path")
	//	{
	//		iter->second._path = kv.second;
	//		return true;
	//	}
	//	else if (kv.first == "name")
	//	{
	//		iter->second._name = kv.second;
	//		return true;
	//	}
	//	std::transform(kv.second.begin(), kv.second.end(), kv.second.begin(), ::tolower);
	//	//! level
	//	if (kv.first == "level")
	//	{
	//		if (kv.second == "trace" || kv.second == "all")
	//		{
	//			iter->second._level = LOG_LEVEL_TRACE;
	//		}
	//		else if (kv.second == "debug")
	//		{
	//			iter->second._level = LOG_LEVEL_DEBUG;
	//		}
	//		else if (kv.second == "info")
	//		{
	//			iter->second._level = LOG_LEVEL_INFO;
	//		}
	//		else if (kv.second == "warn" || kv.second == "warning")
	//		{
	//			iter->second._level = LOG_LEVEL_WARN;
	//		}
	//		else if (kv.second == "error")
	//		{
	//			iter->second._level = LOG_LEVEL_ERROR;
	//		}
	//		else if (kv.second == "alarm")
	//		{
	//			iter->second._level = LOG_LEVEL_ALARM;
	//		}
	//		else if (kv.second == "fatal")
	//		{
	//			iter->second._level = LOG_LEVEL_FATAL;
	//		}
	//	}
	//	//! display
	//	else if (kv.first == "display")
	//	{
	//		if (kv.second == "false" || kv.second == "0")
	//		{
	//			iter->second._display = false;
	//		}
	//		else
	//		{
	//			iter->second._display = true;
	//		}
	//	}
	//	//! output to file
	//	else if (kv.first == "outfile")
	//	{
	//		if (kv.second == "false" || kv.second == "0")
	//		{
	//			iter->second._outfile = false;
	//		}
	//		else
	//		{
	//			iter->second._outfile = true;
	//		}
	//	}
	//	//! monthdir
	//	else if (kv.first == "monthdir")
	//	{
	//		if (kv.second == "false" || kv.second == "0")
	//		{
	//			iter->second._monthdir = false;
	//		}
	//		else
	//		{
	//			iter->second._monthdir = true;
	//		}
	//	}
	//	//! limit file size
	//	else if (kv.first == "limitsize")
	//	{
	//		iter->second._limitsize = atoi(kv.second.c_str());
	//	}
	//	//! display log in file line
	//	else if (kv.first == "fileline")
	//	{
	//		if (kv.second == "false" || kv.second == "0")
	//		{
	//			iter->second._fileLine = false;
	//		}
	//		else
	//		{
	//			iter->second._fileLine = true;
	//		}
	//	}
	//	//! enable/disable one logger
	//	else if (kv.first == "enable")
	//	{
	//		if (kv.second == "false" || kv.second == "0")
	//		{
	//			iter->second._enable = false;
	//		}
	//		else
	//		{
	//			iter->second._enable = true;
	//		}
	//	}
	//	return true;
	//}

	//static bool parseConfigFromString(std::string content, std::map<std::string, LoggerInfo> & outInfo)
	//{

	//	std::string key;
	//	int curLine = 1;
	//	std::string line;
	//	std::string::size_type curPos = 0;
	//	if (content.empty())
	//	{
	//		return true;
	//	}
	//	do
	//	{
	//		std::string::size_type pos = std::string::npos;
	//		for (std::string::size_type i = curPos; i < content.length(); ++i)
	//		{
	//			//support linux/unix/windows LRCF
	//			if (content[i] == '\r' || content[i] == '\n')
	//			{
	//				pos = i;
	//				break;
	//			}
	//		}
	//		line = content.substr(curPos, pos - curPos);
	//		parseConfigLine(line, curLine, key, outInfo);
	//		curLine++;

	//		if (pos == std::string::npos)
	//		{
	//			break;
	//		}
	//		else
	//		{
	//			curPos = pos + 1;
	//		}
	//	} while (1);
	//	return true;
	//}

	bool isDirectory(std::string path)
	{
#ifdef WIN32
		return PathIsDirectoryA(path.c_str()) ? true : false;
#else
		DIR * pdir = opendir(path.c_str());
		if (pdir == NULL)
		{
			return false;
		}
		else
		{
			closedir(pdir);
			pdir = NULL;
			return true;
		}
#endif
	}

	bool createRecursionDir(std::string path)
	{
		if (path.length() == 0) return true;
		std::string sub;
		fixPath(path);

		std::string::size_type pos = path.find('/');
		while (pos != std::string::npos)
		{
			std::string cur = path.substr(0, pos - 0);
			if (cur.length() > 0 && !isDirectory(cur))
			{
				bool ret = false;
#ifdef WIN32
				ret = CreateDirectoryA(cur.c_str(), NULL) ? true : false;
#else
				ret = (mkdir(cur.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
				if (!ret)
				{
					return false;
				}
			}
			pos = path.find('/', pos + 1);
		}

		return true;
	}

	std::string getProcessID()
	{
		std::string pid = "0";
		char buf[260] = { 0 };
#ifdef WIN32
		DWORD winPID = GetCurrentProcessId();
		sprintf(buf, "%06d", winPID);
		pid = buf;
#else
		sprintf(buf, "%06d", getpid());
		pid = buf;
#endif
		return pid;
	}

	std::string getProcessName()
	{
		std::string name = "MainLog";
		char buf[260] = { 0 };
#ifdef WIN32
		if (GetModuleFileNameA(NULL, buf, 259) > 0)
		{
			name = buf;
		}
		std::string::size_type pos = name.rfind("\\");
		if (pos != std::string::npos)
		{
			name = name.substr(pos + 1, std::string::npos);
		}
		pos = name.rfind(".");
		if (pos != std::string::npos)
		{
			name = name.substr(0, pos - 0);
		}

#elif defined(__APPLE__)

		proc_name(getpid(), buf, 260);
		name = buf;
		return name;;
#else
		sprintf(buf, "/proc/%d/cmdline", (int)getpid());
		LogFileHandler i;
		i.open(buf, "r");
		if (!i.isOpen())
		{
			return name;
		}
		name = i.readLine();
		i.close();

		std::string::size_type pos = name.rfind("/");
		if (pos != std::string::npos)
		{
			name = name.substr(pos + 1, std::string::npos);
		}
#endif

		return name;
	}
}

#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unique_ptr>

#define LOG_API

// 枚举
// 日志等级划分
enum LOG_LEVEL {
     LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,    //用于业务错误
    LOG_LEVEL_SYSERROR, //用于技术框架本身的错误
    LOG_LEVEL_FATAL,    //FATAL 级别的日志会让在程序输出日志后退出
    LOG_LEVEL_CRITICAL  //CRITICAL 日志不受日志级别控制，总是输出
};

//TODO: 多增加几个策略
//注意：如果打印的日志信息中有中文，则格式化字符串要用_T()宏包裹起来，
//e.g. LOGI(_T("GroupID=%u, GroupName=%s, GroupName=%s."), 
//   lpGroupInfo->m_nGroupCode, lpGroupInfo->m_strAccount.c_str(), 
//   lpGroupInfo->m_strName.c_str());
#define LOGT(...)    CAsyncLog::output(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOGD(...)    CAsyncLog::output(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOGI(...)    CAsyncLog::output(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(...)    CAsyncLog::output(LOG_LEVEL_WARNING, __FILE__, __LINE__,__VA_ARGS__)
#define LOGE(...)    CAsyncLog::output(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOGSYSE(...) CAsyncLog::output(LOG_LEVEL_SYSERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOGF(...)    CAsyncLog::output(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)        //为了让FATAL级别的日志能立即crash程序，采取同步写日志的方法
#define LOGC(...)    CAsyncLog::output(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __VA_ARGS__)     //关键信息，无视日志级别，总是输出

class LOG_API CAsyncLog {
  public: 
    // 初始化
    static bool init(const char* strFileName = nullptr, bool truncateLongLog = false, long long rollSize = 10 * 1024 * 1024);
    static void uninit();

    // 设置当前日志级别
    static void setLevel(LOG_LEVEL nLevel); 
    static bool isRunning();

    // 输出函数
    // 不输出线程 ID 号和所在函数签名，行号
    static bool output(long nLevel, const char* pszFmt, ...);
    // 输出线程 ID 号和所在函数签名，行号
    static bool output(long nLevel, const char* pszFileName, int lineNo, const char* pszFmt, ...);

    // 输出二进制
    static bool outputBinary(unsigned char* buffer, size_t size);

  private:
    CAsyncLog() = delete;
    ~CAsyncLog() = delete;

    CAsyncLog(const CAsyncLog& rhs) = delete;
    CAsyncLog& operator=(const CAsyncLog& rhs) = delete;

    // 设置后缀
    static void makeLinePrefix(long nLevel, std::string& strPrefix);
    // 获取时间
    static void getTime(char* pszTime, int timeStrLength);
    // 创建新的日志文件
    static bool createNewFile(const char* strFileName);
    // 写入文件
    static bool writeToFile(const std::string& data);
    // 让程序主动崩溃
    static void crash();

    static const char* ullto4Str(int n);
    static char* formLog(int& index, char* szbuf, size_t size_buf, unsigned char* buffer, size_t size);

    static void writeThreadProc();

  public: 
    static bool logToFile; // 标识当前日志写入文件还是写入控制台
    static FILE* hlogFile;
    static std::string strFileName; // 日志文件名
    static std::string strFIleNamePID; // 文件名中的进程 PID
    static bool truncateLongLog; // 长日志是否截断
    static LOG_LEVEL currentLevel; // 当前日志级别
    static long long fileRollSize; // 单日志文件最大字节数
    static long long curretWrittenSize; // 当前已经写入文件的字节数
    static std::list<std::string> listWaitToWrite; // 待写入的日志
    static std::unique_ptr<std::thread> spWriteThread; // 写入日志线程的智能指针
    static std::mutex mutexWrite; // 写入互斥锁
    static std::condition_variable cvWrite; // 写入条件变量
    static bool flagExit; // 退出标志
    static bool flagRunning; // 运行标志
};

#endif 
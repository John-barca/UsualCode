/**
 * @desc: 异步日志类
 * @author: hanhaodong
 * @date: 2021-01-24
*/
#include "AsyncLog.h"
#include <ctime>
#include <sys/timeb.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iostream>
#include <cstdarg>
#include "platform.h"

#define MAX_LINE_LENGTH 256
#define DEFAULT_ROLL_SIZE 10 * 1024 * 1024

bool CAsyncLog::truncateLongLog = false; // 当前长日志不截断
FILE* CAsyncLog::plogFile = NULL;   // 日志文件指针
std::string CAsyncLog::strFileName = "default";     // 日志文件名称
std::string CAsyncLog::strFIleNamePID = "";     // 进程 pid
LOG_LEVEL CAsyncLog::currentLevel = LOG_LEVEL_INFO;     // 日志级别设置为 info 级别
int64_t CAsyncLog::fileRollSize = DEFAULT_ROLL_SIZE;    // 设置日志文件最大字节数
int64_t CAsyncLog::curretWrittenSize = 0;   // 当前写入日志文件的字节数
std::list<std::string> CAsyncLog::listWaitToWrite;  // 待写入日志
std::unique_ptr<std::thread> CAsyncLog::spWriteThread;  // 指向写线程的智能指针
std::mutex CAsyncLog::mutexWrite;   // 写入互斥锁
std::condition_variable CAsyncLog::cvWrite;     // 写入条件变量
bool CAsyncLog::flagExit = false;    // 退出标志
bool CAsyncLog::flagRunning = false;    // 运行标志

// 初始化
bool CAsyncLog::init(const char* strLogFileName /*= nullptr*/, bool truncateLongLine /*= false*/, int64_t rollSize /*= 10 * 1024 * 1024*/) {
  truncateLongLog = truncateLongLine;
  fileRollSize = rollSize;
  if (strLogFileName == nullptr || strLogFileName[0] == 0) {
    strFileName.clear();
  } else {
    strFileName = strLogFileName;
  }
  // 获取进程 pid
  char szPID[8];
#ifdef WIN32    
  snprintf(szPID, sizeof(szPID), "%05d", (int)::GetCurrentProcessId());
#else
  snprintf(szPID, sizeof(szPID), "%05d", (int)::getpid());
#endif    
  strFIleNamePID = szPID;
  // todo: 创建文件夹      
  spWriteThread.reset(new std::thread(writeThreadProc));
  return true;
}

void CAsyncLog::uninit() {    
  flagExit = true;
  cvWrite.notify_one();
  if (spWriteThread->joinable()) {
    spWriteThread->join();
  }

  if (plogFile != nullptr) {
    fclose(plogFile);
    plogFile = nullptr;
  }
}

// 设置当前日志级别
void CAsyncLog::setLevel(LOG_LEVEL nLevel) {     
  if (nLevel < LOG_LEVEL_TRACE || nLevel > LOG_LEVEL_FATAL) {
    return;
  }
  currentLevel = nLevel;
}
bool CAsyncLog::isRunning() {      
  return flagRunning;
}

// 输出函数
// 不输出线程 ID 号和所在函数签名，行号
bool CAsyncLog::output(long nLevel, const char* pszFmt, ...) {
  if (nLevel != LOG_LEVEL_CRITICAL) {
    if (nLevel < currentLevel) {
      return false;
    }
  }

  std::string strLine;
  makeLinePrefix(nLevel, strLine);

  // log 正文    
  std::string strLogContentMsg;

  // 计算不定参数的长度，用于分配空间
  va_list ap;
  va_start(ap, pszFmt);
  // vsnprintf 函数: 将可变参数格式化输出到一个字符数组
  int logContentMsgLen = vsnprintf(NULL, 0, pszFmt, ap);
  va_end(ap);
        
  // 大小加上'\0';
  if ((int)strLogContentMsg.capacity() < logContentMsgLen + 1) {
    strLogContentMsg.resize(logContentMsgLen + 1);
  }    
  va_list aq;
  va_start(aq, pszFmt);
  vsnprintf((char*)strLogContentMsg.data(), strLogContentMsg.capacity(), pszFmt, aq);
  va_end(aq);

  // string 内容正确但 length 不对，恢复一下其 length
  std::string strMsgFormat;
  strMsgFormat.append(strLogContentMsg.c_str(), logContentMsgLen);

  // 如果日志开启截断，长日志只取前 MAX_LINE_LENGTH 个字符
  if (truncateLongLog) {
    strMsgFormat = strMsgFormat.substr(0, MAX_LINE_LENGTH);
  }
  strLine += strMsgFormat;

  // 不是输出到控制台才会在每一行末尾加一个换行符
  if (!strFileName.empty()) {
    strLine += "\n";
  }

  if (nLevel != )
}
// 输出线程 ID 号和所在函数签名，行号
bool output(long nLevel, const char* pszFileName, int lineNo, const char* pszFmt, ...) {

}

// 输出二进制
bool outputBinary(unsigned char* buffer, size_t size) {

}
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
int64_t CAsyncLog::currentWrittenSize = 0;   // 当前写入日志文件的字节数
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

  if (nLevel != LOG_LEVEL_FATAL) {
    // RAII 锁，生命周期结束自动释放锁
    std::lock_guard<std::mutex> lock_guard(mutexWrite);
    listWaitToWrite.push_back(strLine);
    cvWrite.notify_one();
  } else {
    // 为了让 FATAL 级别的日志能立即 crash 程序，采取同步写日志的方法
    std::cout << strLine << std::endl;
  #ifdef _WIN32
    OutputDebugStringA(strLine.c_str());
    OutputDebugStringA("\n");
  #endif
    if (!strFileName.empty()) {
      if (plogFile == nullptr) {
        // 新建日志文件
        char szNow[64];
        time_t now = time(NULL);
        tm time;
#ifdef _WIN32
        localtime_s(&time, &now);
#else
        localtime_r(&now, &time);
#endif
        // 将日期格式化输出到 szNow 字符数组当中
        strftime(szNow, sizeof(szNow), "%Y%m%d%H%M%S", &time);

        // 拼接新的日志文件名
        std::string strNewFileName(strFileName);
        strNewFileName += ".";
        strNewFileName += szNow;
        strNewFileName += ".";
        strNewFileName += strFIleNamePID;
        strNewFileName += ".log";

        // 创建新的日志文件
        if (!createNewFile(strNewFileName.c_str()))
          return false;
      }
      writeToFile(strLine);
    }

    // 程序主动crash 
    crash();
  }
  return true;
}
// 输出线程 ID 号和所在函数签名，行号
bool CAsyncLog::output(long nLevel, const char* pszFileName, int lineNo, const char* pszFmt, ...) {
  if (nLevel != LOG_LEVEL_CRITICAL) {
    if (nLevel < currentLevel) {
      return false;
    }
  }

  std::string strLine;
  makeLinePrefix(nLevel, strLine);
  // 函数签名
  char szFileName[512] = { 0 };
  snprintf(szFileName, sizeof(szFileName), "[%s:%d]", pszFileName, lineNo);
  strLine += szFileName;

  // 日志正文
  std::string strLogContentMsg;
  // 先计算一下不定参数的长度，以便于分配空间
  va_list ap;
  va_start(ap, pszFmt);
  int logMsgLength = vsnprintf(NULL, 0, pszFmt, ap);
  va_end(ap);

  // 算 '\0'
  if ((int)strLogContentMsg.capacity() < logMsgLength + 1) {
    strLogContentMsg.resize(logMsgLength + 1);
  }
  va_list aq;
  va_start(aq, pszFmt);
  vsnprintf((char*)strLogContentMsg.data(), strLogContentMsg.capacity(), pszFmt, aq);
  va_end(aq);

  // string 内容正确但 length 不对，恢复一下其 length
  std::string strMsgFormal;
  strMsgFormal.append(strLogContentMsg.c_str(), logMsgLength);
  // 如果日志开启截断，长日志只取前 MAX_LINE_LENGTH 个字符
  if (truncateLongLog) 
    strMsgFormal = strMsgFormal.substr(0, MAX_LINE_LENGTH);

  strLine += strMsgFormal;
  // 不是输出到控制台才会在每一行末尾添加一个换行符
  if (!strFileName.empty()) {
    strLine += "\n";
  }

  if (nLevel != LOG_LEVEL_FATAL) {
    std::lock_guard<std::mutex> lock_guard(mutexWrite);
    listWaitToWrite.push_back(strLine);
    cvWrite.notify_one();
  } else {
    // 为了让 FATAL 级别的日志能立即 crash 程序，采取同步写日志的方法
    std::cout << strLine << std::endl;
#ifdef _WIN32
    OutputDebugStringA(strLine.c_str());
    OutputDebugStringA("\n");
#endif
    if (!strFileName.empty()) {
      if (plogFile == nullptr) {
        // 新建文件
        char szNow[64];
        time_t now = time(NULL);
        tm time;
#ifdef _WIN32
        localtime_s(&time, &now);
#else 
        localtime_r(&now, &time);
#endif
        strftime(szNow, sizeof(szNow), "%Y%m%H%M%S", &time);
        std::string strNewFileName(strFileName);
        strNewFileName += ".";
        strNewFileName += szNow;
        strNewFileName += ".";
        strNewFileName += strFIleNamePID;
        strNewFileName += ".log";
        if (!createNewFile(strNewFileName.c_str()))
          return false;
      }
      writeToFile(strLine);
    }
    crash();
  }

  return true;
}

// 输出二进制
bool CAsyncLog::outputBinary(unsigned char* buffer, size_t size) {
  std::ostringstream os;

  static const size_t PRINTSIZE = 512;
  char szbuf[PRINTSIZE * 3 + 8];

  size_t lsize = 0;
  size_t lprintbufsize = 0;
  int index = 0;
  os << "address[" << (long)buffer << "] size[" << size << "] \n";
  while (true) {
    memset(szbuf, 0, sizeof(szbuf));
    if (size > lsize) {
      lprintbufsize = (size - lsize);
      lprintbufsize = lprintbufsize > PRINTSIZE ? PRINTSIZE : lprintbufsize;
      formLog(index, szbuf, sizeof(szbuf), buffer + lsize, lprintbufsize);
      size_t len = strlen(szbuf);

      os << szbuf;
      lsize += lprintbufsize;
    } else {
      break;
    }
  }
  std::lock_guard<std::mutex> lock_guard(mutexWrite);
  listWaitToWrite.push_back(os.str());
  cvWrite.notify_one();

  return true;
}

const char* CAsyncLog::ullto4Str(int n) {
  static char buf[64 + 1];
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "%06u", n);
  return buf;
}

char* CAsyncLog::formLog(int& index, char* szbuf, size_t size_buf, unsigned char* buffer, size_t size) {
  size_t len = 0;
  size_t lsize = 0;
  int headlen = 0;
  char szhead[64 + 1] = { 0 };
  char szchar[17] = "0123456789abcdef";
  while (size > lsize && len + 10 < size_buf) {
    if (lsize % 32 == 0) {
      if (0 != headlen) {
        szbuf[len++] = '\n';
      }

      memset(szhead, 0, sizeof(szhead));
      strncpy(szhead, ullto4Str(index++), sizeof(szhead) - 1);
      headlen = strlen(szhead);
      szhead[headlen++] = ' ';

      strcat(szbuf, szhead);
      len += headlen;
    }
    if (lsize % 16 == 0 && 0 != headlen) 
      szbuf[len++] = ' ';
    szbuf[len++] = szchar[(buffer[lsize] >> 4) & 0xf];
    szbuf[len++] = szchar[(buffer[lsize]) & 0xf];
    lsize++;
  }
  szbuf[len++] = '\n';
  szbuf[len++] = '\0';
  return szbuf;
}

void CAsyncLog::makeLinePrefix(long nLevel, std::string& strPrefix) {
  // 日志不同级别，前缀
  strPrefix = "[INFO]";
  if (nLevel == LOG_LEVEL_TRACE) 
    strPrefix = "[TRACE]";
  else if (nLevel == LOG_LEVEL_DEBUG) 
    strPrefix = "[DEBUG]";
  else if (nLevel == LOG_LEVEL_WARNING) 
    strPrefix = "[WARN]";
  else if (nLevel == LOG_LEVEL_ERROR)
    strPrefix = "[ERROR]";
  else if (nLevel == LOG_LEVEL_SYSERROR)
    strPrefix = "[SYSE]";
  else if (nLevel == LOG_LEVEL_FATAL) 
    strPrefix = "[FATAL]";
  else if (nLevel == LOG_LEVEL_CRITICAL)
    strPrefix = "[CRITICAL]";
  // 时间
  char szTime[64] = { 0 };
  getTime(szTime, sizeof(szTime));
  strPrefix += "[";
  strPrefix += szTime;
  strPrefix += "]";

  // 当前线程信息
  char szThreadID[32] = { 0 };
  std::ostringstream osThreadID;
  osThreadID << std::this_thread::get_id();
  snprintf(szThreadID, sizeof(szThreadID), "[%s]", osThreadID.str().c_str());
  strPrefix += szThreadID;
}

void CAsyncLog::getTime(char* pszTime, int timeStrLength) {
  struct timeb tp;
  ftime(&tp);

  time_t now = tp.time;
  tm time;
#ifdef _WIN32
  localtime_s(&time, &now);
#else 
  localtime_r(&now, &time);
#endif
  snprintf(pszTime, timeStrLength, "[%04d-%02d-%02d %02d:%02d:%02d:%03d]", time.tm_year + 1900, 
  time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, tp.millitm);
}

bool CAsyncLog::createNewFile(const char* strFileName) {
  if (plogFile != nullptr) {
    fclose(plogFile);
  }

  // 始终新建文件
  plogFile = fopen(strFileName, "w+");
  return plogFile != nullptr;
}

bool CAsyncLog::writeToFile(const std::string& data) {
  // 为了防止长文件一次性写不完，放在一个循环里面分批写
  std::string strLocal(data);
  int ret = 0;
  while (true) {
    ret = fwrite(strLocal.c_str(), 1, strLocal.length(), plogFile);
    if (ret <= 0) 
      return false;
    else if (ret <= (int)strLocal.length()) {
      strLocal.erase(0, ret);
    }
    // 写完后跳出
    if (strLocal.empty())
      break;
  }
  fflush(plogFile);
  return true;
}

void CAsyncLog::crash() {
  char* p = nullptr;
  *p = 0;
}

void CAsyncLog::writeThreadProc() {
  flagRunning = true;
  while(true) {
    if (!strFileName.empty()) {
      if (plogFile == nullptr || currentWrittenSize >= fileRollSize) {
        // currentWrittenSize 大小
        currentWrittenSize = 0;
        // 第一次或者文件大小超过 rollSize，均新建文件
        char szNow[64];
        time_t now = time(NULL);
        tm time;
#ifdef _WIN32
        localtime_s(&time, &now);
#else   
        localtime_r(&now, &time);
#endif
        strftime(szNow, sizeof(szNow), "%Y%m%d%H%M%S", &time);

        std::string strNewFileName(strFileName);
        strNewFileName += ".";
        strNewFileName += szNow;
        strNewFileName += ".";
        strNewFileName += strFIleNamePID;
        strNewFileName += ".log";
        if (!createNewFile(strNewFileName.c_str()));
          return;
      }
    }

    std::string strLine; 
    std::unique_lock<std::mutex> guard(mutexWrite);
    while(listWaitToWrite.empty()) {
      if (flagExit) 
        return;
      cvWrite.wait(guard);
    }
    strLine = listWaitToWrite.front();
    listWaitToWrite.pop_front();
    std::cout << strLine << std::endl;
#ifdef _WIN32
    OutputDebugStringA(strLine.c_str());
    OutputDebugStringA("\n");
#endif
    if (!strFileName.empty()) {
      if (!writeToFile(strLine)) 
        return;
      
      currentWrittenSize += strLine.length();
    }
  }
  flagRunning = false;
}
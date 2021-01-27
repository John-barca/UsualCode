/**
 * 配置文件读取
 * hanhaodong   2021-01-27
*/
#include "ConfigFileReader.h"
#include <stdio.h>
#include <string.h>
#include <map>

ConfigFileReader::ConfigFileReader(const char* fileName) {
  loadFile(fileName);
}

ConfigFileReader::~ConfigFileReader() {}

char* ConfigFileReader::getConfigName(const char* name) {
  if (!flagLoadOk) 
    return NULL;

  char* value = NULL;
  std::map<std::string, std::string>::iterator it = configMap.find(name);
  if (it != configMap.end()) {
    value = (char*)it->second.c_str();
  }
  return value;
}

int ConfigFileReader::setConfigValue(const char* name, const char* value) {
  if (!flagLoadOk) 
    return -1;

  std::map<std::string, std::string>::iterator it = configMap.find(name);
  if (it != configMap.end()) {
    // 找到的话修改当前 value
    it->second = value;
  } else {
    // 否则创造该键值对
    configMap.insert(std::make_pair(name, value));
  }
  return writeFile();
}

// 加载配置文件
void ConfigFileReader::loadFile(const char* filename) {
  configFile.clear();
  configFile.append(filename);
  FILE* fp = fopen(filename, "r");
  if (!fp) 
    return;
  
  char buf[256];
  for (;;) {
    char* p = fgets(buf, 256, fp);
    if (!p) 
      break;
    size_t len = strlen(buf);
    if (buf[len - 1] == '\n') 
      buf[len - 1] = 0;

    char* ch = strchr(buf, '#');
    if (ch)
      *ch = 0;
    if (strlen(buf) == 0)
      continue;

    parseLine(buf);
  }
  fclose(fp);
  flagLoadOk = true;
} 

int ConfigFileReader::writeFile(const char* filename) {
  FILE* fp = NULL;
  if (filename == NULL) {
    fp = fopen(configFile.c_str(), "w");
  } else {
    fp = fopen(filename, "w");
  }

  if (fp == NULL) {
    return -1;
  }
  char szPaire[128];
  std::map<std::string, std::string>::iterator it = configMap.begin();
  for (; it != configMap.end(); it++) {
    memset(szPaire, 0, sizeof(szPaire));
    snprintf(szPaire, sizeof(szPaire), "%s=%s\n", it->first.c_str(), it->second.c_str());
    size_t ret = fwrite(szPaire, strlen(szPaire), 1, fp);
    if (ret != 1) {
      fclose(fp);
      return -1;
    }
  }
  fclose(fp);
  return 0;
}

void ConfigFileReader::parseLine(char* line) {
  char* p = strchr(line, '=');
  if (p == NULL)
    return;

  *p = 0;
  // 移除字符串当中的空格
  char* key = trimSpace(line);
  char* value = trimSpace(p + 1);
  if (key && value) {
    configMap.insert(std::make_pair(key, value));
  }
}

char* ConfigFileReader::trimSpace(char* name) {
  // 删除一开始的 space 或 tab 
  char* start_pos = name;
  while ((*start_pos == ' ') || (*start_pos == '\t') || (*start_pos == '\r')) {
    start_pos++;
  }
  if (strlen(start_pos) == 0) 
    return NULL;

  // 删除结尾的 space 或 tab
  char* end_pos = name + strlen(name) - 1;
  while((*end_pos == ' ') || (*end_pos == '\t') || (*end_pos == '\r')) {
    *end_pos = 0;
    end_pos--;
  }
  int len = (int)(end_pos - start_pos) + 1;
  if (len <= 0)
    return NULL;

  return start_pos;
}
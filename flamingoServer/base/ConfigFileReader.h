/**
 * 配置文件读取类. ConfigFileReader.h
 * hanhaodong 2021-1-27
 */
#ifndef __CONFIG_FILE_READER_H__
#define __CONFIG_FILE_READER_H__

#include <map>
#include <string>

class ConfigFileReader {
public:
  ConfigFileReader(const char* filename);
  ~ConfigFileReader();

  char* getConfigName(const char* name);
  int setConfigValue(const char* name, const char* value);
private:
  void loadFile(const char* filename);
  int writeFile(const char* filename = NULL);
  void parseLine(char* line);
  char* trimSpace(char* name);

  bool flagLoadOk;
  std::map<std::string, std::string> configMap;
  std::string configFile;
};
#endif
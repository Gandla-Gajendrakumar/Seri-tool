#pragma once
// microSD on VSPI. Event/sample log (/log.csv) and a file-backed upload queue (/queue.txt with
// the read position kept in /qpos.txt so it survives reboots). Card-detect switch on SD_CD.
#include <SD.h>
#include <string>
#include "seri/hal.h"

class SdStorage : public seri::hal::Storage {
 public:
  SdStorage(int cs, int cardDetect) : cs_(cs), cd_(cardDetect) {}
  void begin();
  bool ok() override;
  bool appendLog(const std::string& line) override;
  bool queuePush(const std::string& record) override;
  bool queueFront(std::string& record) override;
  bool queuePop() override;
  size_t queueSize() override;
  bool readFile(const std::string& name, std::string& out) override;
  bool removeFile(const std::string& name) override;
 private:
  bool mount();
  void unmount();
  void loadQueueState();
  void saveQueuePos();
  int cs_, cd_;
  bool mounted_ = false, failed_ = false;
  uint32_t qpos_ = 0;
  size_t qcount_ = 0;
  std::string frontCache_;
};

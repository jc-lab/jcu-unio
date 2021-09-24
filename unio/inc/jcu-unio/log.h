/**
 * @file	log.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_LOG_H_
#define JCU_UNIO_LOG_H_

#include <memory>
#include <string>
#include <cstdarg>
#include <functional>

namespace jcu {
namespace unio {

class Logger {
 public:
  enum LogLevel {
    kLogTrace,
    kLogDebug,
    kLogInfo,
    kLogWarn,
    kLogError
  };

  typedef std::function<void(LogLevel level, const std::string &log)> LogWriter_t;

  virtual void logf(LogLevel level, const char *format, ...) = 0;
};

std::shared_ptr<Logger> createDefaultLogger(const Logger::LogWriter_t &writer);

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_LOG_H_

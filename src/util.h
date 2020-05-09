#ifndef AFPACKET_UTIL_H_
#define AFPACKET_UTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <time.h>      // strftime(), gmtime(), time(),
#include <sys/time.h>  // gettimeofday()
#include <pthread.h>   // pthread_self()
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>  // backtrace(), backtrace_symbols()

#include <condition_variable>
#include <deque>
#include <iomanip>   // setw, setfill
#include <iostream>  // cerr
#include <memory>
#include <mutex>
#include <sstream>  // stringstream
#include <string>   // string
#include <thread>


namespace afpackettest {

#define DISALLOW_COPY_AND_ASSIGN(name) \
  name(const name&);                   \
  name(const name&&);                  \
  void operator=(const name&)

////////////////////////////////////////////////////////////////////////////////
//// Logging helpers
//
// Simple methods for logging intersting events.
// LOG(INFO) << "blah";  // normal logging message
// LOG(FATAL) << "blah";  // log, then crash the program.  something is wrong.
// CHECK(b) << "blah";  // if 'b' is false, crash with given message.

namespace {

const size_t kTimeBufferSize = 20;
const char* kTimeFormat = "%Y-%m-%dT%H:%M:%S";

}  // namespace

class LogLine {
 public:
  LogLine(bool crash, const char* file, int line) : crash_(crash) {
    FillTimeBuffer();
    uint32_t tid = uint32_t(pthread_self()) >> 8;  // first bits always 0.
    ss_ << std::setfill('0') << time_buffer_ << "." << std::setw(6)
        << tv_.tv_usec << "Z T:" << std::hex << std::setw(6) << tid
        << std::setw(0) << std::dec << " [" << file << ":" << line << "] ";
  }
  ~LogLine() {
    ss_ << "\n";
    std::cerr << ss_.str() << std::flush;
    if (crash_) {
      std::cerr << "ABORTABORTABORT" << std::endl;
      void* backtraces[32];
      int size = backtrace(backtraces, 32);
      char** symbols = backtrace_symbols(backtraces, size);
      for (int i = 0; i < size; i++) {
        std::cerr << symbols[i] << std::endl;
      }
      free(symbols);
      abort();
    }
  }

  template <class T>
  LogLine& operator<<(const T& data) {
    ss_ << data;
    return *this;
  }

 private:
  void FillTimeBuffer() {
    gettimeofday(&tv_, NULL);
    struct tm timeinfo;
    gmtime_r(&tv_.tv_sec, &timeinfo);
    size_t len =
        strftime(time_buffer_, kTimeBufferSize, kTimeFormat, &timeinfo);
    if (len + 1 != kTimeBufferSize) {  // returned num bytes doesn't include \0
      strcpy(time_buffer_, "STRFTIME_ERROR");
    }
  }

  std::stringstream ss_;
  struct timeval tv_;
  char time_buffer_[kTimeBufferSize];
  bool crash_;

  DISALLOW_COPY_AND_ASSIGN(LogLine);
};

extern int logging_verbose_level;

typedef std::unique_ptr<std::string> Error;

}  // namespace 
// Define macros OUTSIDE of the namespace.

#define LOGGING_FATAL_CRASH true
#define LOGGING_ERROR_CRASH false
#define LOGGING_INFO_CRASH false

#define LOGGING_FATAL_LOG true
#define LOGGING_ERROR_LOG true
#define LOGGING_INFO_LOG (logging_verbose_level > 0)

#ifndef VLOG
#define VLOG(x) if (logging_verbose_level > (x)) \
  ::afpackettest::LogLine(false, __FILE__, __LINE__)
#endif
#ifndef LOG
#define LOG(level)           \
  if (LOGGING_##level##_LOG) \
  ::afpackettest::LogLine(LOGGING_##level##_CRASH, __FILE__, __LINE__)
#endif
#ifndef CHECK
#define CHECK(expr) \
  if (!(expr)) LOG(FATAL) << "CHECK(" #expr ") "
#endif

#define SUCCEEDED(x) ((x).get() == NULL)
#define SUCCESS NULL

#define ERROR(x) ::afpackettest::Error(new std::string(x))

#define RETURN_IF_ERROR(status, msg)                   \
  do {                                                 \
    ::afpackettest::Error __return_if_error_status__ = (status); \
    if (!SUCCEEDED(__return_if_error_status__)) {      \
      __return_if_error_status__->append(" <- ");      \
      __return_if_error_status__->append(msg);         \
      return __return_if_error_status__;               \
    }                                                  \
  } while (false)

#define LOG_IF_ERROR(status, msg)                            \
  do {                                                       \
    ::afpackettest::Error __log_if_error_status__ = (status);          \
    if (!SUCCEEDED(__log_if_error_status__)) {               \
      LOG(ERROR) << msg << ": " << *__log_if_error_status__; \
    }                                                        \
  } while (false)

#define CHECK_SUCCESS(x)                                                   \
  do {                                                                     \
    ::afpackettest::Error __check_success_error__ = (x);                             \
    CHECK(SUCCEEDED(__check_success_error__)) << #x << ": "                \
                                              << *__check_success_error__; \
  } while (false)

#define REPLACE_IF_ERROR(initial, replacement)         \
  do {                                                 \
    ::st::Error __replacement_error__ = (replacement); \
    if (!SUCCEEDED(__replacement_error__)) {           \
      if (!SUCCEEDED(initial)) {                       \
        LOG(ERROR) << "replacing error: " << *initial; \
      }                                                \
      initial = std::move(__replacement_error__);      \
    }                                                  \
  } while (false)

namespace afpackettest {

inline Error Errno(int ret = -1) {
  if (ret >= 0 || errno == 0) {
    return SUCCESS;
  }
  return ERROR(strerror(errno));
}

}  // namespace afpackettest

#endif  // AFPACKET_UTIL_H_

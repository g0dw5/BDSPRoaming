#ifndef ROAMINGID_SCOPE_GUARD_H_
#define ROAMINGID_SCOPE_GUARD_H_

#include <chrono>
#include <functional>
#include <iomanip>
#include <sstream>
#include <stdio.h>

class ScopeGuard {
 public:
  explicit ScopeGuard(std::function<void()> onExitScope)
      : onExitScope_(onExitScope), dismissed_(false) {
  }

  ~ScopeGuard() {
    if (!dismissed_) {
      onExitScope_();
    }
  }

  void Dismiss() { dismissed_ = true; }

 private:
  std::function<void()> onExitScope_;
  bool dismissed_{false};

 private:
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard(ScopeGuard &&) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
  ScopeGuard &operator=(ScopeGuard &&) = delete;
};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)

#define ON_SCOPE_EXIT(callback) \
  ScopeGuard SCOPEGUARD_LINENAME(exit, __LINE__)(callback)

// FIXME:其实应该叫ScopeStopWatch
class FunctionStopWatch {
 public:
  FunctionStopWatch(
      std::string func_name,
      std::function<void(const std::string &)> fn_log =
          [](const std::string &) {},
      int time_out_in_millisecond = -1)
      : func_name_(func_name),
        fn_log_(fn_log),
        time_out_in_millisecond_(time_out_in_millisecond) {
    tb_ = std::chrono::high_resolution_clock::now();
  }
  ~FunctionStopWatch() {
    te_ = std::chrono::high_resolution_clock::now();
    int duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(te_ - tb_)
            .count();
    if (duration <= time_out_in_millisecond_)
      return;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << std::chrono::time_point_cast<std::chrono::milliseconds>(tb_)
               .time_since_epoch()
               .count()
        << "|"
        << std::chrono::time_point_cast<std::chrono::milliseconds>(te_)
               .time_since_epoch()
               .count()
        << "|" << duration / 1000.0 << "|" << func_name_;

    std::string msg = oss.str();
    fn_log_(msg);
  }

 private:
  std::string func_name_{};
  std::function<void(std::string)> fn_log_;
  int time_out_in_millisecond_{-1};
  decltype(std::chrono::high_resolution_clock::now()) tb_;
  decltype(std::chrono::high_resolution_clock::now()) te_;
};

#endif// ROAMINGID_SCOPE_GUARD_H_

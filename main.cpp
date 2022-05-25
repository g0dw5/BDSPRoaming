#include <cmath>
#include <iostream>
#include <list>
#include <vector>

#include "Xoroshiro128Plus8b.h"
#include "roaming_finder.h"

#include "scope_guard.h"
#include "thread_pool.h"

class IProgress {
 public:
  virtual void Progress(double dProgress, const char *pszInfo) = 0;

  virtual void ProgOver(bool bSuccess, const char *pszInfo) = 0;
};

class CmdProgress : public IProgress {
 public:
  CmdProgress() {
    sprintf(m_szFormat, "%%3.0%df", m_precision);
    m_pszPercent = (char *) calloc(m_szPercentLen, sizeof(char));
  }

  ~CmdProgress() {
    free(m_pszPercent);
    m_pszPercent = nullptr;
  }

 public:
  virtual void Progress(double dProgress, const char *pszInfo) {
    const int c_iBarLen(50);

    uint32_t szThisProg = uint32_t(dProgress * 100 * pow(10, m_precision));
    if (szThisProg != m_szLastProg) {
      m_szLastProg = szThisProg;

      char bar[c_iBarLen + 1] = {0};
      int nThisTick = (int) ceil(dProgress * c_iBarLen);
      nThisTick = std::min(c_iBarLen, std::max(0, nThisTick));
      for (int i = 0; i < c_iBarLen; ++i) {
        if (i < nThisTick - 1)
          bar[i] = '=';
        else if (i == nThisTick - 1)
          bar[i] = dProgress == 1 ? '=' : '>';
        else
          bar[i] = ' ';
      }

      sprintf(m_pszPercent, m_szFormat, szThisProg / pow(10.0, m_precision));
      printf("Progress:[%s]\t%s%%\r", bar, m_pszPercent);
      fflush(stdout);
    }
  }

  virtual void ProgOver(bool bSuccess, const char *pszInfo) {
    m_bSuccess = bSuccess;
    if (bSuccess) {
      printf("\n处理成功\n");
    } else {
      printf("\n处理失败：%s\n", pszInfo);
    }
    if (nullptr != pszInfo) {
      printf("%s\n", pszInfo);
    }
    c.notify_one();
  }

 public:
  void wait() {
    {
      std::unique_lock<std::mutex> lock(m);
      c.wait(lock);
    }
  }

  bool success() { return m_bSuccess; }

 private:
  std::condition_variable c;
  std::mutex m;
  uint32_t m_szLastProg = 0;
  bool m_bSuccess = false;
  const uint32_t m_precision = 2;// 1-9

  char m_szFormat[7];
  uint32_t m_szPercentLen = 4 + m_precision + 1;
  char *m_pszPercent = nullptr;
};

void FindRoamingPokemon() {
  // 这里改你的诉求
  uint G8SID = 0;
  uint G8TID = 999999;
  uint HP = 31;
  uint ATK = 0;
  uint DEF = 31;
  uint SPA = 31;
  uint SPD = 31;
  uint SPE = 0;
  uint thread_count = std::thread::hardware_concurrency();

  FunctionStopWatch stop_watch(
      __PRETTY_FUNCTION__, [](const std::string &msg) {
        std::cout << msg << std::endl;
      });

  ITrainerID trainer;
  IVs expect_ivs;

  // 内部计算用的真实ID需要转一下
  uint tidsid = G8SID * 1000000 + G8TID;
  trainer.SID = tidsid & 0xFFFF;
  trainer.TID = tidsid >> 16;

  expect_ivs.IV_HP = HP;
  expect_ivs.IV_ATK = ATK;
  expect_ivs.IV_DEF = DEF;
  expect_ivs.IV_SPA = SPA;
  expect_ivs.IV_SPD = SPD;
  expect_ivs.IV_SPE = SPE;

  uint end_seed = 0x80000000;
  uint batch_count = thread_count * 128;
  uint seed_count_in_each_batch = end_seed / batch_count;

  using PKMs = std::vector<PKM>;
  using Futures = std::list<std::future<PKMs>>;

  ThreadPool pool(thread_count);
  Futures futures;
  CmdProgress prog;

  std::thread t([&]() {
    for (uint batch = 0; batch < batch_count; ++batch) {
      uint e_beg = seed_count_in_each_batch * (batch);
      uint e_end = seed_count_in_each_batch * (batch + 1);

      if (batch == batch_count - 1)
        e_end = end_seed;

      futures.emplace_back(pool.enqueue([e_beg, e_end, &trainer, &expect_ivs] {
        PKMs pkms;

        for (uint e = e_beg; e < e_end; ++e) {
          RoamingFinder finder(trainer, expect_ivs, e);

          if (finder.Step1IsIVLegal()) {
            const auto &pkm = finder.Step2GetPokemon();
            if (Shiny::Never != pkm.shiny) {
              pkms.push_back(pkm);
            }
          }
        }

        return pkms;
      }));
    }

    PKMs shiny_pkms;

    uint32_t over_count{};
    for (auto &f: futures) {
      auto pkms = f.get();
      for (const auto &pkm: pkms) {
        shiny_pkms.push_back(pkm);
      }

      prog.Progress(double(over_count + 1) / batch_count, nullptr);
      ++over_count;
    }
    prog.ProgOver(true, nullptr);

    for (const auto &pkm: shiny_pkms) {
      std::cout << std::hex << "Encryption=" << pkm.EncryptionConstant << std::endl;
      std::cout << std::hex << "PID=" << pkm.PID << std::endl;
      std::cout << std::dec << "AbilityIndex=" << pkm.AbilityNumber << std::endl;
      std::cout << std::dec << "HeightScalar=" << pkm.HeightScalar << std::endl;
      std::cout << std::dec << "WeightScalar=" << pkm.WeightScalar << std::endl;
      std::cout << "ShinyType=" << GetShinyType(pkm.shiny) << std::endl;
      std::cout << std::endl;
    }
  });
  t.detach();

  prog.wait();
}

int main(int argc, char *argv[]) {
  FindRoamingPokemon();
  return 0;
}

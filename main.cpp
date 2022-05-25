#include <iostream>
#include <list>
#include <vector>

#include "Xoroshiro128Plus8b.h"
#include "roaming_finder.h"

#include "scope_guard.h"
#include "thread_pool.h"

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
  uint batch_count = thread_count * 4;
  uint seed_count_in_each_batch = end_seed / batch_count;

  using PKMs = std::vector<PKM>;
  using Futures = std::list<std::future<PKMs>>;
  Futures futures;

  ThreadPool pool(thread_count);

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

  for (auto &f: futures) {
    auto pkms = f.get();

    for (const auto &pkm: pkms) {
      std::cout << std::hex << "Encryption=" << pkm.EncryptionConstant << std::endl;
      std::cout << std::hex << "PID=" << pkm.PID << std::endl;
      std::cout << std::dec << "AbilityIndex=" << pkm.AbilityNumber << std::endl;
      std::cout << std::dec << "HeightScalar=" << pkm.HeightScalar << std::endl;
      std::cout << std::dec << "WeightScalar=" << pkm.WeightScalar << std::endl;
      std::cout << "ShinyType=" << GetShinyType(pkm.shiny) << std::endl;
      std::cout << std::endl;
    }
  }
}

int main() {
  FindRoamingPokemon();
  return 0;
}

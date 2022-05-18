#include <iostream>
#include <vector>

#include "Xoroshiro128Plus8b.h"
#include "roaming_finder.h"

#include "scope_guard.h"

void FindRoamingPokemon() {
  FunctionStopWatch stop_watch(
      __PRETTY_FUNCTION__, [](const std::string &msg) {
        std::cout << msg << std::endl;
      });

  ITrainerID trainer;
  trainer.SID = 2331;
  trainer.TID = 210519;
  // FIXME:通过PKHex的窗口tips读到的真的ID
  trainer.SID = 35571;
  trainer.TID = 29463;

  IVs expect_ivs;
  expect_ivs.IV_HP = 31;
  expect_ivs.IV_ATK = 0;
  expect_ivs.IV_DEF = 31;
  expect_ivs.IV_SPA = 31;
  expect_ivs.IV_SPD = 31;
  expect_ivs.IV_SPE = 31;

  for (long e = 0; e < (1L << 31); ++e) {
    RoamingFinder finder(trainer, expect_ivs, e);

    if (finder.Step1IsIVLegal()) {
      const auto &pkm = finder.Step2GetPokemon();

      if (Shiny::Never != pkm.shiny) {
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
}

int main() {
  FindRoamingPokemon();
  return 0;
}

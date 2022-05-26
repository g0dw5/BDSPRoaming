//
// Created by 王嵩 on 2022/5/26.
//

#ifndef ROAMINGID__OVERWORLD_FINDER_H_
#define ROAMINGID__OVERWORLD_FINDER_H_

#include "Xoroshiro128Plus.h"

class OverworldFinder {
 public:
  OverworldFinder(const ITrainerID &trainer, const IVs &ivs, uint seed)
      : trainer_(trainer), expect_ivs_(ivs), rnd_(seed) {
    pkm_.SID = trainer.SID;
    pkm_.TID = trainer.TID;
  }
  OverworldFinder(const OverworldFinder &) = delete;
  OverworldFinder(OverworldFinder &&) = delete;
  OverworldFinder &operator=(const OverworldFinder &) = delete;
  OverworldFinder &operator=(OverworldFinder &&) = delete;
  virtual ~OverworldFinder() = default;

 public:
  // 生成假OID,PID,IVs
  bool Step1IsIVLegal();
  // 生成特性序号,身高,体重
  const PKM &Step2GetPokemon();

 private:
  Xoroshiro128Plus rnd_;

  const ITrainerID &trainer_;
  const IVs &expect_ivs_;

  PKM pkm_;

  // FIXME:input from pokemon
  static constexpr uint kFlawlessCount = 0;
  static constexpr uint kFlawlessValue = 31;
  static constexpr int kUnsetIV = -1;

 private:
  static uint GetRevisedPID(uint pid, ITrainerID tr);
  static uint GetOID(int tid, int sid);
  static uint GetShinyXor(uint pid, uint oid);
  static Shiny GetRareType(uint _xor);
};

#endif//ROAMINGID__OVERWORLD_FINDER_H_

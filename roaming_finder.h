//
// Created by 王嵩 on 2022/5/18.
//

#ifndef ROAMINGID__ROAMING_FINDER_H_
#define ROAMINGID__ROAMING_FINDER_H_

#include "Xoroshiro128Plus8b.h"

class RoamingFinder {
 public:
  RoamingFinder(const ITrainerID &trainer, const IVs &ivs, uint seed)
      : trainer_(trainer), expect_ivs_(ivs), rnd_(seed) {
    pkm_.SID = trainer.SID;
    pkm_.TID = trainer.TID;
    pkm_.EncryptionConstant = seed;
  }
  RoamingFinder(const RoamingFinder &) = delete;
  RoamingFinder(RoamingFinder &&) = delete;
  RoamingFinder &operator=(const RoamingFinder &) = delete;
  RoamingFinder &operator=(RoamingFinder &&) = delete;
  virtual ~RoamingFinder() = default;

 public:
  // 生成假OID,PID,IVs
  bool Step1IsIVLegal();
  // 生成特性序号,身高,体重
  const PKM &Step2GetPokemon();

 private:
  Xoroshiro128Plus8b rnd_;

  const ITrainerID &trainer_;
  const IVs &expect_ivs_;

  PKM pkm_;

  static constexpr uint kFlawlessCount = 3;
  static constexpr uint kFlawlessValue = 31;
  static constexpr int kUnsetIV = -1;

 private:
  static uint GetRevisedPID(uint oid_from_rand, uint pid, ITrainerID tr);
  static uint GetOID(int tid, int sid);
  static uint GetShinyXor(uint pid, uint oid);
  static Shiny GetRareType(uint _xor);
};

#endif//ROAMINGID__ROAMING_FINDER_H_

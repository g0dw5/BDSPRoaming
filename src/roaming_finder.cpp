//
// Created by 王嵩 on 2022/5/18.
//

#include "roaming_finder.h"

#include <vector>

bool RoamingFinder::Step1IsSatisfied() {
  pkm_.EncryptionConstant = seed_;

  uint oid_from_rand = rnd_.NextUInt();
  uint pid = rnd_.NextUInt();
  // 根据闪的状态修正一下PID
  uint revised_pid = GetRevisedPID(oid_from_rand, pid, trainer_);
  pkm_.PID = revised_pid;

  uint oid = GetOID(pkm_.TID, pkm_.SID);
  pkm_.shiny = GetRareType(GetShinyXor(pkm_.PID, oid));

  if (shiny_type_you_want_ != pkm_.shiny)
    return false;

  std::vector<int> ivs{kUnsetIV, kUnsetIV, kUnsetIV, kUnsetIV, kUnsetIV, kUnsetIV};

  // 用完美个数取index,剩下的随机个体值
  int determined = 0;
  while (determined < flawless_count_) {
    int idx = (int) rnd_.NextUInt(6);
    if (ivs[idx] != kUnsetIV)
      continue;

    ivs[idx] = kFlawlessValue;

    uint expect_value = *((uint *) (&expect_ivs_) + idx);
    if (ivs[idx] != expect_value)
      return false;

    ++determined;
  }

  for (int idx = 0; idx < ivs.size(); ++idx) {
    if (ivs[idx] == kUnsetIV) {
      ivs[idx] = (int) rnd_.NextUInt(kFlawlessValue + 1);

      uint expect_value = *((uint *) (&expect_ivs_) + idx);
      if (ivs[idx] != expect_value)
        return false;
    }
  }

  // 如果要放宽一点条件,需要把这里打开,把上面的提前返回关了
  //  if (ivs[0] != expect_ivs_.IV_HP) return false;
  //  if (ivs[1] != expect_ivs_.IV_ATK) return false;
  //  if (ivs[2] != expect_ivs_.IV_DEF) return false;
  //  if (ivs[3] != expect_ivs_.IV_SPA) return false;
  //  if (ivs[4] != expect_ivs_.IV_SPD) return false;
  //  if (ivs[5] != expect_ivs_.IV_SPE) return false;

  pkm_.IV_HP = ivs[0];
  pkm_.IV_ATK = ivs[1];
  pkm_.IV_DEF = ivs[2];
  pkm_.IV_SPA = ivs[3];
  pkm_.IV_SPD = ivs[4];
  pkm_.IV_SPE = ivs[5];

  return true;
}

const PKM &RoamingFinder::Step2GetPokemon() {
  pkm_.AbilityNumber = (1 << (int) rnd_.NextUInt(2));
  pkm_.HeightScalar = (unsigned char) ((int) rnd_.NextUInt(0x81) + (int) rnd_.NextUInt(0x80));
  pkm_.WeightScalar = (unsigned char) ((int) rnd_.NextUInt(0x81) + (int) rnd_.NextUInt(0x80));

  return pkm_;
}

uint RoamingFinder::GetRevisedPID(uint oid_from_rand, uint pid, ITrainerID tr) {
  auto shiny_type_by_rand = GetRareType(GetShinyXor(pid, oid_from_rand));

  uint oid = GetOID(tr.TID, tr.SID);
  auto shiny_type_by_pid = GetRareType(GetShinyXor(pid, oid));

  // 推算跟真实一致(都闪或都不闪)
  if (shiny_type_by_rand == shiny_type_by_pid)
    return pid;

  if (Shiny::Never != shiny_type_by_rand) {
    // 推算闪,真的不闪,通过修改PID使之闪
    return (((uint) (tr.TID ^ tr.SID) ^ (pid & 0xFFFF) ^ (Shiny::AlwaysSquare == shiny_type_by_rand ? 0u : 1u)) << 16) | (pid & 0xFFFF);
  } else {
    // 推算不闪,真的闪,随便找了个方式把PID变不闪
    return pid ^ 0x10000000;
  }
}

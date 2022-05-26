//
// Created by 王嵩 on 2022/5/26.
//

#include "overworld_finder.h"

#include <vector>

bool OverworldFinder::Step1IsSatisfied()
{
  pkm_.EncryptionConstant = rnd_.NextInt();
  uint pid = rnd_.NextInt();

  std::vector<int> ivs{kUnsetIV, kUnsetIV, kUnsetIV,
                       kUnsetIV, kUnsetIV, kUnsetIV};

  // 用完美个数取index,剩下的随机个体值
  int determined = 0;
  while (determined < flawless_count_)
  {
    int idx = (int)rnd_.NextInt(6);
    if (ivs[idx] != kUnsetIV)
      continue;

    ivs[idx] = kFlawlessValue;

    uint expect_value = *((uint*)(&expect_ivs_) + idx);
    if (ivs[idx] != expect_value)
      return false;

    ++determined;
  }

  for (int idx = 0; idx < ivs.size(); ++idx)
  {
    if (ivs[idx] == kUnsetIV)
    {
      ivs[idx] = (int)rnd_.NextInt(kFlawlessValue + 1);

      uint expect_value = *((uint*)(&expect_ivs_) + idx);
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

  // 根据闪的状态修正一下PID
  uint revised_pid = GetRevisedPID(pid, trainer_);
  pkm_.PID = revised_pid;
  uint oid = GetOID(pkm_.TID, pkm_.SID);
  pkm_.shiny = GetRareType(GetShinyXor(pkm_.PID, oid));

  return true;
}

const PKM& OverworldFinder::Step2GetPokemon()
{
  pkm_.HeightScalar =
      (unsigned char)((int)rnd_.NextInt(0x81) + (int)rnd_.NextInt(0x80));
  pkm_.WeightScalar =
      (unsigned char)((int)rnd_.NextInt(0x81) + (int)rnd_.NextInt(0x80));

  return pkm_;
}

uint OverworldFinder::GetRevisedPID(uint pid, ITrainerID tr)
{
  // TODO(wang.song)
  // 跟BDSP游走不同,这里可以设置成任意的闪类型,want_shiny_type可设置为入参
  Shiny shiny_type_by_need = shiny_type_you_want_;

  uint oid = GetOID(tr.TID, tr.SID);
  auto shiny_type_by_pid = GetRareType(GetShinyXor(pid, oid));

  // 期望跟真实一致(都闪或都不闪)
  if (shiny_type_by_need == shiny_type_by_pid)
    return pid;

  if (Shiny::Never != shiny_type_by_need)
  {
    // 期望闪,真的不闪,通过修改PID使之闪
    return (((uint)(tr.TID ^ tr.SID) ^ (pid & 0xFFFF) ^
             (Shiny::AlwaysSquare == shiny_type_by_need ? 0u : 1u))
            << 16) |
           (pid & 0xFFFF);
  }
  else
  {
    // 期望不闪,真的闪,随便找了个方式把PID变不闪
    return pid ^ 0x10000000;
  }
}

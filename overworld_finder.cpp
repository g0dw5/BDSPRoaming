//
// Created by 王嵩 on 2022/5/26.
//

#include "overworld_finder.h"

#include <vector>

bool OverworldFinder::Step1IsIVLegal() {
  pkm_.EncryptionConstant = rnd_.NextInt();
  auto pid = rnd_.NextInt();

  std::vector<int> ivs{kUnsetIV, kUnsetIV, kUnsetIV, kUnsetIV, kUnsetIV, kUnsetIV};

  // 用完美个数取index,剩下的随机个体值
  auto determined = 0;
  while (determined < kFlawlessCount) {
    auto idx = (int) rnd_.NextInt(6);
    if (ivs[idx] != kUnsetIV)
      continue;

    ivs[idx] = kFlawlessValue;

    auto expect_value = *((uint *) (&expect_ivs_) + idx);
    if (ivs[idx] != expect_value)
      return false;

    ++determined;
  }

  for (int idx = 0; idx < ivs.size(); ++idx) {
    if (ivs[idx] == kUnsetIV) {
      ivs[idx] = (int) rnd_.NextInt(kFlawlessValue + 1);

      auto expect_value = *((uint *) (&expect_ivs_) + idx);
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
  auto revised_pid = GetRevisedPID(pid, trainer_);
  pkm_.PID = revised_pid;
  auto oid = GetOID(pkm_.TID, pkm_.SID);
  pkm_.shiny = GetRareType(GetShinyXor(pkm_.PID, oid));

  return true;
}

const PKM &OverworldFinder::Step2GetPokemon() {
  pkm_.HeightScalar = (unsigned char) ((int) rnd_.NextInt(0x81) + (int) rnd_.NextInt(0x80));
  pkm_.WeightScalar = (unsigned char) ((int) rnd_.NextInt(0x81) + (int) rnd_.NextInt(0x80));

  return pkm_;
}

uint OverworldFinder::GetRevisedPID(uint pid, ITrainerID tr) {
  // TODO(wang.song) 跟BDSP游走不同,这里可以设置成任意的闪类型,看看是不是通过加查询参数进行统一化的处理
  auto fake_xor = 0;
  auto oid = GetOID(tr.TID, tr.SID);
  auto real_xor = GetShinyXor(pid, oid);

  auto fake_shiny_type = GetRareType(fake_xor);
  auto real_shiny_type = GetRareType(real_xor);

  // 推算跟真实一致(都闪或都不闪)
  if (fake_shiny_type == real_shiny_type)
    return pid;

  auto is_fake_shiny = fake_xor < 16;
  if (is_fake_shiny) {
    // 推算闪,真的不闪,通过修改PID使之闪
    return (((uint) (tr.TID ^ tr.SID) ^ (pid & 0xFFFF) ^ (fake_xor == 0 ? 0u : 1u)) << 16) | (pid & 0xFFFF);
  } else {
    // 推算不闪,真的闪,随便找了个方式把PID变不闪
    return pid ^ 0x10000000;
  }
}

uint OverworldFinder::GetOID(int tid, int sid) {
  return (uint) ((sid << 16) | tid);
}

uint OverworldFinder::GetShinyXor(uint pid, uint oid) {
  auto _xor = pid ^ oid;
  return (_xor ^ (_xor >> 16)) & 0xFFFF;
}

Shiny OverworldFinder::GetRareType(uint _xor) {
  if (_xor == 0) {
    return Shiny::AlwaysSquare;
  } else if (_xor < 16) {
    return Shiny::AlwaysStar;
  } else {
    return Shiny::Never;
  }
}

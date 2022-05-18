#include <iostream>
#include <vector>

#include "Xoroshiro128Plus8b.h"

enum class Shiny {
  //  Random = 0,
  Never = 1,
  //  Always = 2,
  AlwaysStar = 3,
  AlwaysSquare = 4,
  //  FixedValue = 5,
};

enum class Nature : uint8_t {
  Hardy = 0,
  Lonely = 1,
  Brave = 2,
  Adamant = 3,
  Naughty = 4,
  Bold = 5,
  Docile = 6,
  Relaxed = 7,
  Impish = 8,
  Lax = 9,
  Timid = 10,
  Hasty = 11,
  Serious = 12,
  Jolly = 13,
  Naive = 14,
  Modest = 15,
  Mild = 16,
  Quiet = 17,
  Bashful = 18,
  Rash = 19,
  Calm = 20,
  Gentle = 21,
  Sassy = 22,
  Careful = 23,
  Quirky = 24,

  Random = 25,
};

struct ITrainerID {
  int SID{};
  int TID{};
};

static constexpr uint kFlawlessValue = 31;
class PKM : public ITrainerID {
 public:
  uint IV_HP{};
  uint IV_ATK{};
  uint IV_DEF{};
  uint IV_SPA{};
  uint IV_SPD{};
  uint IV_SPE{};

  uint EncryptionConstant{};

  uint PID{};
  // 特性序号(1,2)(1起始)
  uint AbilityNumber{};
  Nature nature{Nature::Random};
  uint HeightScalar{};
  uint WeightScalar{};
};

class ShinyUtil {
 public:
  static uint GetTidSid(int tid, int sid) {
    return (sid << 16) | tid;
  }

  static uint GetShinyXor(uint val) { return (val >> 16) ^ (val & 0xFFFF); }

  static uint GetShinyValue(uint num) { return GetShinyXor(num) >> 4; }

  static Shiny GetShinyType(uint pid, uint tidsid) {
    uint p = GetShinyXor(pid);
    uint t = GetShinyXor(tidsid);
    if (p == t)
      return Shiny::AlwaysSquare;
    if ((p ^ t) < 0x10)
      return Shiny::AlwaysStar;
    return Shiny::Never;
  }
};

class Roaming8bRNG {
 public:
  bool IsIVOK(const PKM &pk) {
    int flawless = 3;

    Xoroshiro128Plus8b xoro(pk.EncryptionConstant);

    // 要先生成2个数
    auto fakeOID = xoro.NextUInt();
    (void) fakeOID;
    auto pid = xoro.NextUInt();
    (void) pid;

    // Check IVs: Create flawless IVs at random indexes, then the random IVs for not flawless.
    std::vector<int> ivs = {UNSET, UNSET, UNSET, UNSET, UNSET, UNSET};

    auto determined = 0;
    while (determined < flawless) {
      auto idx = (int) xoro.NextUInt(6);
      if (ivs[idx] != UNSET)
        continue;

      ivs[idx] = kFlawlessValue;
      ++determined;
    }

    for (int i = 0; i < ivs.size(); ++i) {
      if (ivs[i] == UNSET)
        ivs[i] = (int) xoro.NextUInt(kFlawlessValue + 1);
    }

    if (ivs[0] != pk.IV_HP) return false;
    if (ivs[1] != pk.IV_ATK) return false;
    if (ivs[2] != pk.IV_DEF) return false;
    if (ivs[3] != pk.IV_SPA) return false;
    if (ivs[4] != pk.IV_SPD) return false;
    if (ivs[5] != pk.IV_SPE) return false;

    return true;
  }

  void GeneratePKM(PKM &pk) {
    int flawless = 3;

    Xoroshiro128Plus8b xoro(pk.EncryptionConstant);

    // Check PID
    auto fakeOID = xoro.NextUInt();
    auto pid_bak = xoro.NextUInt();
    auto pid = GetRevisedPID(fakeOID, pid_bak, pk);

    // Check IVs: Create flawless IVs at random indexes, then the random IVs for not flawless.
    std::vector<int> ivs = {UNSET, UNSET, UNSET, UNSET, UNSET, UNSET};

    auto determined = 0;
    while (determined < flawless) {
      auto idx = (int) xoro.NextUInt(6);
      if (ivs[idx] != UNSET)
        continue;

      ivs[idx] = kFlawlessValue;
      ++determined;
    }

    for (int i = 0; i < ivs.size(); ++i) {
      if (ivs[i] == UNSET)
        ivs[i] = (int) xoro.NextUInt(kFlawlessValue + 1);
    }

    pk.IV_HP = ivs[0];
    pk.IV_ATK = ivs[1];
    pk.IV_DEF = ivs[2];
    pk.IV_SPA = ivs[3];
    pk.IV_SPD = ivs[4];
    pk.IV_SPE = ivs[5];

    pk.PID = pid;
    pk.AbilityNumber = (1 << (int) xoro.NextUInt(2));
    pk.HeightScalar = (unsigned char) ((int) xoro.NextUInt(0x81) + (int) xoro.NextUInt(0x80));
    pk.WeightScalar = (unsigned char) ((int) xoro.NextUInt(0x81) + (int) xoro.NextUInt(0x80));
  }

  bool ValidateRoamingEncounter(const PKM &pk) {
    int flawless = 3;

    auto seed = pk.EncryptionConstant;
    if (seed == std::numeric_limits<int>::max())
      return false;// Unity's Rand is [int.MinValue, int.MaxValue)
    Xoroshiro128Plus8b xoro(seed);

    // Check PID
    auto fakeOID = xoro.NextUInt();
    auto pid = xoro.NextUInt();
    pid = GetRevisedPID(fakeOID, pid, pk);
    if (pk.PID != pid)
      return false;

    // Check IVs: Create flawless IVs at random indexes, then the random IVs for not flawless.
    std::vector<int> ivs{UNSET, UNSET, UNSET, UNSET, UNSET, UNSET};

    auto determined = 0;
    while (determined < flawless) {
      auto idx = (int) xoro.NextUInt(6);
      if (ivs[idx] != UNSET)
        continue;
      ivs[idx] = kFlawlessValue;
      determined++;
    }

    for (auto i = 0; i < ivs.size(); i++) {
      if (ivs[i] == UNSET)
        ivs[i] = (int) xoro.NextUInt(kFlawlessValue + 1);
    }

    if (ivs[0] != pk.IV_HP) return false;
    if (ivs[1] != pk.IV_ATK) return false;
    if (ivs[2] != pk.IV_DEF) return false;
    if (ivs[3] != pk.IV_SPA) return false;
    if (ivs[4] != pk.IV_SPD) return false;
    if (ivs[5] != pk.IV_SPE) return false;

    // Don't check Hidden ability, as roaming encounters are 1/2 only.
    if (pk.AbilityNumber != (1 << (int) xoro.NextUInt(2)))
      return false;

    return true;
  }

 private:
  uint GetRevisedPID(uint fakeOID, uint pid, ITrainerID tr) {
    auto _xor = GetShinyXor(pid, fakeOID);
    auto oid = GetOID(tr.TID, tr.SID);
    auto newXor = GetShinyXor(pid, oid);

    auto fakeRare = GetRareType(_xor);
    auto newRare = GetRareType(newXor);

    if (fakeRare == newRare)
      return pid;

    auto isShiny = _xor < 16;
    if (isShiny) {
      // force same shiny star type
      return (((uint) (tr.TID ^ tr.SID) ^ (pid & 0xFFFF) ^ (_xor == 0 ? 0u : 1u)) << 16) | (pid & 0xFFFF);
    }
    return pid ^ 0x10000000;
  }

  uint GetOID(int tid, int sid) {
    return (uint) ((sid << 16) | tid);
  }

  uint GetShinyXor(uint pid, uint oid) {
    auto _xor = pid ^ oid;
    return (_xor ^ (_xor >> 16)) & 0xFFFF;
  }

  Shiny GetRareType(uint _xor) {
    if (_xor == 0) {
      return Shiny::AlwaysSquare;
    } else if (_xor < 16) {
      return Shiny::AlwaysStar;
    } else {
      return Shiny::Never;
    }
  }

 private:
  static const int UNSET = -1;
};

void verify() {
  PKM cresselia;
  // 先定死的
  cresselia.SID = 651;
  cresselia.TID = 147661;
  cresselia.SID = 2331;
  cresselia.TID = 210519;
  cresselia.IV_HP = 2;
  cresselia.IV_ATK = 31;
  cresselia.IV_DEF = 31;
  cresselia.IV_SPA = 31;
  cresselia.IV_SPD = 25;
  cresselia.IV_SPE = 8;

  uint tidsid = ShinyUtil::GetTidSid(cresselia.TID, cresselia.SID);
  uint tsv = ShinyUtil::GetShinyValue(tidsid);

  cresselia.EncryptionConstant = 0x5684E8EA;

  {
    // 代码推断的
    cresselia.PID = 0xC1738335;
    cresselia.AbilityNumber = 1;
    cresselia.HeightScalar = 151;
    cresselia.WeightScalar = 120;

    auto shinytype = ShinyUtil::GetShinyType(cresselia.PID, tidsid);
    uint psv = ShinyUtil::GetShinyValue(cresselia.PID);

    Roaming8bRNG r;
    bool valid = r.ValidateRoamingEncounter(cresselia);
  }

  {
    // PKHex算出来的
    cresselia.PID = 0x1C778335;
    cresselia.AbilityNumber = 1;
    cresselia.HeightScalar = 184;
    cresselia.WeightScalar = 88;

    auto shinytype = ShinyUtil::GetShinyType(cresselia.PID, tidsid);
    uint psv = ShinyUtil::GetShinyValue(cresselia.PID);

    Roaming8bRNG r;
    bool valid = r.ValidateRoamingEncounter(cresselia);
  }
}

void loopFindPK() {
  PKM cresselia;
  // 先定死的
  cresselia.SID = 2331;
  cresselia.TID = 210519;
  uint fake_tid = 29463;
  uint fake_sid = 35571;

  cresselia.IV_HP = 31;
  cresselia.IV_ATK = 0;
  cresselia.IV_DEF = 31;
  cresselia.IV_SPA = 31;
  cresselia.IV_SPD = 31;
  cresselia.IV_SPE = 0;

  static const uint tidsid = ShinyUtil::GetTidSid(fake_tid, fake_sid);

  for (long e = 0; e < (1L << 31); ++e) {
    //    if (e != 0x321ae2b4 && e != 0x3d77798f && e != 0x5e7082da && e != 0x62e47e43)
    //      continue;
    // 随机变量
    cresselia.EncryptionConstant = e;

    Roaming8bRNG v1;
    if (v1.IsIVOK(cresselia)) {

      PKM pk = cresselia;

      Roaming8bRNG v2;
      v2.GeneratePKM(pk);

      auto shiny_type = ShinyUtil::GetShinyType(pk.PID, tidsid);

      if (Shiny::Never != shiny_type) {

        Roaming8bRNG v3;
        bool b1 = v3.ValidateRoamingEncounter(pk);

        std::cout << std::hex << "Encryption=" << pk.EncryptionConstant << std::endl;
        std::cout << std::hex << "PID=" << pk.PID << std::endl;
        std::cout << std::dec << "AbilityIndex=" << pk.AbilityNumber << std::endl;
        std::cout << std::dec << "HeightScalar=" << pk.HeightScalar << std::endl;
        std::cout << std::dec << "WeightScalar=" << pk.WeightScalar << std::endl;
        std::cout << std::endl;
      }
    }
  }
}

int main() {
  loopFindPK();
  return 0;

  PKM cresselia;
  // 先定死的
  cresselia.SID = 651;
  cresselia.TID = 147661;
  cresselia.IV_HP = 2;
  cresselia.IV_ATK = 31;
  cresselia.IV_DEF = 31;
  cresselia.IV_SPA = 31;
  cresselia.IV_SPD = 25;
  cresselia.IV_SPE = 8;

  for (long e = 0; e < (1L << 31); ++e) {
    // 方块闪
    if (e != 0x5684E8EA)
      continue;

    // 随机变量
    cresselia.EncryptionConstant = e;

    //    Roaming8bRNG v1;
    //    if (v1.IsIVOK(cresselia)) {

    PKM pk = cresselia;

    Roaming8bRNG v2;
    v2.GeneratePKM(pk);

    //    if (Shiny::AlwaysSquare == pk.shiny) {
    std::cout << std::hex << "Encryption=" << pk.EncryptionConstant << std::endl;
    std::cout << std::hex << "PID=" << pk.PID << std::endl;
    std::cout << std::dec << "AbilityIndex=" << pk.AbilityNumber << std::endl;
    std::cout << std::dec << "HeightScalar=" << pk.HeightScalar << std::endl;
    std::cout << std::dec << "WeightScalar=" << pk.WeightScalar << std::endl;
    std::cout << std::endl;
    //    }
  }
  //  }

  return 0;
}

#include <iostream>
#include <vector>

using uint = uint32_t;
using ulong = unsigned long;

enum class Shiny {
  Random = 0,
  Never,
  Always,
  AlwaysStar,
  AlwaysSquare,
  FixedValue,
};

struct ITrainerID {
  int SID{};
  int TID{};
};

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
  uint HeightScalar{};
  uint WeightScalar{};
  Shiny shiny{Shiny::Never};

 public:
  uint ShinyXor() {
    auto pid = PID;
    auto upper = (pid >> 16) ^ (uint) SID;
    return (pid & 0xFFFF) ^ (uint) TID ^ upper;
  }

  bool IsShiny() {
    uint PSV = ((PID >> 16) ^ (PID & 0xFFFF)) >> 4;
    uint TSV = ((uint16_t) TID ^ (uint16_t) SID) >> 4;
    return PSV == TSV;
  }
};

struct Xoroshiro128Plus8b {
 public:
  Xoroshiro128Plus8b(ulong seed) {
    s0 = SplitMix64(seed + 0x9E3779B97F4A7C15);
    s1 = SplitMix64(seed + 0x3C6EF372FE94F82A);
  }

  uint NextUInt() {
    return (uint) (Next() >> 32);
  }

  uint NextUInt(uint max) {
    auto rnd = NextUInt();
    return rnd - ((rnd / max) * max);
  }

 private:
  ulong SplitMix64(ulong seed) {
    seed = 0xBF58476D1CE4E5B9 * (seed ^ (seed >> 30));
    seed = 0x94D049BB133111EB * (seed ^ (seed >> 27));
    return seed ^ (seed >> 31);
  }

  ulong Next() {
    auto _s0 = s0;
    auto _s1 = s1;
    ulong result = _s0 + _s1;
    _s1 ^= s0;

    // Final calculations and store back to fields
    s0 = RotateLeft(_s0, 24) ^ _s1 ^ (_s1 << 16);
    s1 = RotateLeft(_s1, 37);

    return result;
  }

  ulong RotateLeft(ulong x, int k) {
    return (x << k) | (x >> (64 - k));
  }

 private:
  ulong s0, s1;
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
  bool IsPKMMatchIV(PKM pk) {
    static const int flawless = 3;

    Xoroshiro128Plus8b xoro(pk.EncryptionConstant);

    // 要先生成2个数
    auto fakeTID = xoro.NextUInt();
    (void) fakeTID;
    auto pid = xoro.NextUInt();
    (void) pid;

    // Check IVs: Create flawless IVs at random indexes, then the random IVs for not flawless.
    std::vector<int> ivs = {UNSET, UNSET, UNSET, UNSET, UNSET, UNSET};

    auto determined = 0;
    while (determined < flawless) {
      auto idx = (int) xoro.NextUInt(6);
      if (ivs[idx] != UNSET)
        continue;

      ivs[idx] = 31;
      ++determined;
    }

    for (int i = 0; i < ivs.size(); ++i) {
      if (ivs[i] == UNSET)
        ivs[i] = (int) xoro.NextUInt(31 + 1);
    }

    if (ivs[0] != pk.IV_HP) return false;
    if (ivs[1] != pk.IV_ATK) return false;
    if (ivs[2] != pk.IV_DEF) return false;
    if (ivs[3] != pk.IV_SPA) return false;
    if (ivs[4] != pk.IV_SPD) return false;
    if (ivs[5] != pk.IV_SPE) return false;

    return true;
  }

  void RewritePKM(PKM &pk) {
    static const int flawless = 3;

    Xoroshiro128Plus8b xoro(pk.EncryptionConstant);

    // Check PID
    auto fakeOID = xoro.NextUInt();
    auto pid_bak = xoro.NextUInt();
    auto pid = GetRevisedPID(fakeOID, pid_bak, pk);
    bool is_shiny = GetIsShiny(pk.TID, pk.SID, pid);

    // Check IVs: Create flawless IVs at random indexes, then the random IVs for not flawless.
    std::vector<int> ivs = {UNSET, UNSET, UNSET, UNSET, UNSET, UNSET};

    auto determined = 0;
    while (determined < flawless) {
      auto idx = (int) xoro.NextUInt(6);
      if (ivs[idx] != UNSET)
        continue;

      ivs[idx] = 31;
      ++determined;
    }

    for (int i = 0; i < ivs.size(); ++i) {
      if (ivs[i] == UNSET)
        ivs[i] = (int) xoro.NextUInt(31 + 1);
    }

    pk.PID = pid;
    pk.AbilityNumber = (1 << (int) xoro.NextUInt(2));
    pk.HeightScalar = (unsigned char) ((int) xoro.NextUInt(0x81) + (int) xoro.NextUInt(0x80));
    pk.WeightScalar = (unsigned char) ((int) xoro.NextUInt(0x81) + (int) xoro.NextUInt(0x80));

    auto pkShiny = pk.ShinyXor();
    auto oid = GetOID(pk.TID, pk.SID);
    pk.shiny = GetRareType(GetShinyXor(pk.PID, oid));
  }

  bool ValidateRoamingEncounter(PKM pk, int flawless = 3) {
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
      ivs[idx] = 31;
      determined++;
    }

    for (auto i = 0; i < ivs.size(); i++) {
      if (ivs[i] == UNSET)
        ivs[i] = (int) xoro.NextUInt(31 + 1);
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

  uint GetRevisedPID2(uint fakeOID, uint pid, ITrainerID tr) {
    uint tid = tr.TID;
    uint sid = tr.SID;

    uint tidsid = ShinyUtil::GetTidSid(tr.TID, tr.SID);
    auto shinytype = ShinyUtil::GetShinyType(pid, tidsid);

    uint tsv = ShinyUtil::GetShinyValue(tidsid);
    uint psv = ShinyUtil::GetShinyValue(pid);

    // TODO(wang.song) implement
    //    if (fixedShiny == 2 && shinytype == 0) {
    //      shinytype = 2;
    //    }
    //    if (fixedShiny == 1) {
    //      shinytype = 0;
    //    }

    if (shinytype == Shiny::Never) {
      if (psv == tsv)
        return pid ^ 0x10000000;// ensure not shiny
      return pid;
    }

    if (psv == tsv)
      return pid;// already shiny
    return (pid & 0xFFFF) | (tid ^ sid ^ pid ^ (2 - static_cast<uint>(shinytype))) << 16;
  }

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
      uint a = tr.TID ^ tr.SID;
      uint b = pid & 0xFFFF;
      uint c = _xor == 0 ? 0u : 1u;

      uint r = ((a ^ b ^ c) << 16) | b;
      return r;
      return (((uint) (tr.TID ^ tr.SID) ^ (pid & 0xFFFF) ^ (_xor == 0 ? 0u : 1u)) << 16) | (pid & 0xFFFF);
    }
    return pid ^ 0x10000000;
  }

  uint GetOID(int tid, int sid) {
    return (uint) ((sid << 16) | tid);
  }

  bool GetIsShiny(int tid, int sid, uint pid) {
    return GetIsShiny(pid, GetOID(tid, sid));
  }

  bool GetIsShiny(uint pid, uint oid) { return GetShinyXor(pid, oid) < 16; }

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

    bool is_shiny = cresselia.IsShiny();
    auto shinytype = ShinyUtil::GetShinyType(cresselia.PID, tidsid);
    uint psv = ShinyUtil::GetShinyValue(cresselia.PID);

    Roaming8bRNG r;
    bool valid = r.ValidateRoamingEncounter(cresselia);
    is_shiny = r.GetIsShiny(cresselia.TID, cresselia.SID, cresselia.PID);
    random();
  }

  {
    // PKHex算出来的
    cresselia.PID = 0x1C778335;
    cresselia.AbilityNumber = 1;
    cresselia.HeightScalar = 184;
    cresselia.WeightScalar = 88;

    bool is_shiny = cresselia.IsShiny();
    auto shinytype = ShinyUtil::GetShinyType(cresselia.PID, tidsid);
    uint psv = ShinyUtil::GetShinyValue(cresselia.PID);

    Roaming8bRNG r;
    bool valid = r.ValidateRoamingEncounter(cresselia);
    is_shiny = r.GetIsShiny(cresselia.TID, cresselia.SID, cresselia.PID);
    random();
  }
}

void loopFindPK() {
  PKM cresselia;
  // 先定死的
  cresselia.SID = 2331;
  cresselia.TID = 210519;
  cresselia.IV_HP = 31;
  cresselia.IV_ATK = 0;
  cresselia.IV_DEF = 31;
  cresselia.IV_SPA = 31;
  cresselia.IV_SPD = 31;
  cresselia.IV_SPE = 0;

  uint fake_tid = 29463;
  uint fake_sid = 35571;
  static const uint tidsid = ShinyUtil::GetTidSid(fake_tid, fake_sid);

  for (long e = 0; e < (1L << 31); ++e) {
    if (e != 0x321ae2b4 && e != 0x3d77798f && e != 0x5e7082da && e != 0x62e47e43)
      continue;
    // 随机变量
    cresselia.EncryptionConstant = e;

    Roaming8bRNG v1;
    if (v1.IsPKMMatchIV(cresselia)) {

      PKM pk = cresselia;

      Roaming8bRNG v2;
      v2.RewritePKM(pk);

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
    //    if (v1.IsPKMMatchIV(cresselia)) {

    PKM pk = cresselia;

    Roaming8bRNG v2;
    v2.RewritePKM(pk);

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

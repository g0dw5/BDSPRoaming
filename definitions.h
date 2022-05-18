//
// Created by 王嵩 on 2022/5/18.
//

#ifndef ROAMINGID__DEFINITIONS_H_
#define ROAMINGID__DEFINITIONS_H_

#include <cstdint>
#include <string>

using uint = uint32_t;
using ulong = unsigned long;

enum class Shiny {
  //  Random = 0,
  Never = 1,
  //  Always = 2,
  AlwaysStar = 3,
  AlwaysSquare = 4,
  //  FixedValue = 5,
};

static std::string GetShinyType(Shiny shiny) {
  switch (shiny) {
    case Shiny::Never: return "None";
    case Shiny::AlwaysStar: return "Star";
    case Shiny::AlwaysSquare: return "Square";
  }
}

struct ITrainerID {
  int SID{};
  int TID{};
};

struct IVs {
  uint IV_HP{};
  uint IV_ATK{};
  uint IV_DEF{};
  uint IV_SPA{};
  uint IV_SPD{};
  uint IV_SPE{};
};

class PKM : public ITrainerID, public IVs {
 public:
  uint EncryptionConstant{};

  uint PID{};
  // 特性序号(游走类都没有梦特,2个特性)(1起始)
  uint AbilityNumber{};
  uint HeightScalar{};
  uint WeightScalar{};

  Shiny shiny{Shiny::Never};
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

#endif//ROAMINGID__DEFINITIONS_H_

//
// Created by 王嵩 on 2022/5/18.
//

#ifndef ROAMINGID__DEFINITIONS_H_
#define ROAMINGID__DEFINITIONS_H_

#include <cstdint>
#include <string>

using byte = uint8_t;
using ushort = uint16_t;
using uint = uint32_t;
using ulong = unsigned long;

enum class RNDType
{
  kSWSHOverworld,
  kBDSPRoaming,
};

enum class Shiny
{
  kNone,
  kStar,
  kSquare,
};

enum class Nature
{
  Hardy = 0,     ///< 勤奋
  Lonely = 1,    ///< 怕寂寞
  Brave = 2,     ///< 勇敢
  Adamant = 3,   ///< 固执
  Naughty = 4,   ///< 顽皮
  Bold = 5,      ///< 大胆
  Docile = 6,    ///< 坦率
  Relaxed = 7,   ///< 悠闲
  Impish = 8,    ///< 淘气
  Lax = 9,       ///< 乐天
  Timid = 10,    ///< 胆小
  Hasty = 11,    ///< 急躁
  Serious = 12,  ///< 认真
  Jolly = 13,    ///< 爽朗
  Naive = 14,    ///< 天真
  Modest = 15,   ///< 内敛
  Mild = 16,     ///< 慢吞吞
  Quiet = 17,    ///< 冷静
  Bashful = 18,  ///< 害羞
  Rash = 19,     ///< 马虎
  Calm = 20,     ///< 温和
  Gentle = 21,   ///< 温顺
  Sassy = 22,    ///< 自大
  Careful = 23,  ///< 慎重
  Quirky = 24,   ///< 浮躁

  Random = 25,
};

static std::string GetNatureStr(Nature n)
{
  switch (n)
  {
    case Nature::Hardy:
      return "勤奋";
    case Nature::Lonely:
      return "怕寂寞";
    case Nature::Brave:
      return "勇敢";
    case Nature::Adamant:
      return "固执";
    case Nature::Naughty:
      return "顽皮";
    case Nature::Bold:
      return "大胆";
    case Nature::Docile:
      return "坦率";
    case Nature::Relaxed:
      return "悠闲";
    case Nature::Impish:
      return "淘气";
    case Nature::Lax:
      return "乐天";
    case Nature::Timid:
      return "胆小";
    case Nature::Hasty:
      return "急躁";
    case Nature::Serious:
      return "认真";
    case Nature::Jolly:
      return "爽朗";
    case Nature::Naive:
      return "天真";
    case Nature::Modest:
      return "内敛";
    case Nature::Mild:
      return "慢吞吞";
    case Nature::Quiet:
      return "冷静";
    case Nature::Bashful:
      return "害羞";
    case Nature::Rash:
      return "马虎";
    case Nature::Calm:
      return "温和";
    case Nature::Gentle:
      return "温顺";
    case Nature::Sassy:
      return "自大";
    case Nature::Careful:
      return "慎重";
    case Nature::Quirky:
      return "浮躁";
    default:
      return "";
  }
}

static std::string GetShinyType(Shiny shiny)
{
  switch (shiny)
  {
    case Shiny::kNone:
      return "None";
    case Shiny::kStar:
      return "Star";
    case Shiny::kSquare:
      return "Square";
  }
}

struct ITrainerID
{
  int SID{};
  int TID{};
};

struct IVs
{
  union
  {
    uint data{};
    struct
    {
      uint IV_HP : 5;
      uint IV_ATK : 5;
      uint IV_DEF : 5;
      uint IV_SPA : 5;
      uint IV_SPD : 5;
      uint IV_SPE : 5;
    };
  };

  IVs() { data = 0; }
  IVs(uint d) { data = d; }
  IVs(uint hp, uint atk, uint def, uint spa, uint spd, uint spe)
  {
    IV_HP = hp;
    IV_ATK = atk;
    IV_DEF = def;
    IV_SPA = spa;
    IV_SPD = spd;
    IV_SPE = spe;
  }

  uint operator[](int index) const
  {
    switch (index)
    {
      case 0:
        return IV_HP;
      case 1:
        return IV_ATK;
      case 2:
        return IV_DEF;
      case 3:
        return IV_SPA;
      case 4:
        return IV_SPD;
      case 5:
        return IV_SPE;
      default:
        __builtin_trap();
    }
  }

  void Set(int index, uint value)
  {
    switch (index)
    {
      case 0:
        IV_HP = value;
        break;
      case 1:
        IV_ATK = value;
        break;
      case 2:
        IV_DEF = value;
        break;
      case 3:
        IV_SPA = value;
        break;
      case 4:
        IV_SPD = value;
        break;
      case 5:
        IV_SPE = value;
        break;
      default:
        __builtin_trap();
    }
  }
};

class PKM : public ITrainerID, public IVs
{
 public:
  uint seed{};
  uint EncryptionConstant{};

  uint PID{};
  // 特性序号(游走类都没有梦特,2个特性)(1起始)
  uint AbilityNumber{};
  uint HeightScalar{};
  uint WeightScalar{};

  Shiny shiny{Shiny::kNone};
};

class ShinyUtil
{
 public:
  static uint GetTidSid(int tid, int sid) { return (sid << 16) | tid; }

  static uint GetShinyXor(uint val) { return (val >> 16) ^ (val & 0xFFFF); }

  static uint GetShinyValue(uint num) { return GetShinyXor(num) >> 4; }

  static Shiny GetShinyType(uint pid, uint tidsid)
  {
    uint p = GetShinyXor(pid);
    uint t = GetShinyXor(tidsid);
    if (p == t)
      return Shiny::kSquare;
    if ((p ^ t) < 0x10)
      return Shiny::kStar;
    return Shiny::kNone;
  }
};

#endif  // ROAMINGID__DEFINITIONS_H_

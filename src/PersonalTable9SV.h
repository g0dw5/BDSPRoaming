//
// Created by 王嵩 on 2022/12/15.
//

#ifndef ROAMINGID_PERSONALTABLE9SV_H
#define ROAMINGID_PERSONALTABLE9SV_H

#include <cstdint>
#include <functional>

#pragma pack(1)

struct PersonalInfo9SV
{
 public:
  static const int SIZE = 0x44;
  static const int CountTM = 172;
  static const int RatioMagicGenderless = 255;
  static const int RatioMagicFemale = 254;
  static const int RatioMagicMale = 0;
  // 0公1母2无性别

 public:
  uint16_t GetSpeciesFormIndex(uint16_t species, uint8_t form) const
  {
    if (!HasForm(form))
      return species;
    return FormStatsIndex + form - 1;
  }

  bool HasForm(uint8_t form) const
  {
    if (form == 0)  // no form requested
      return false;
    if (FormStatsIndex <= 0)  // no forms present
      return false;
    if (form >= FormCount)  // beyond range of encounter_index' forms
      return false;
    return true;
  }

  uint8_t HP;
  uint8_t ATK;
  uint8_t DEF;
  uint8_t SPE;
  uint8_t SPA;
  uint8_t SPD;
  uint8_t Type1;
  uint8_t Type2;
  uint8_t CatchRate;
  uint8_t EvoStage;  ///< 处在第几段进化态
  union
  {
    uint16_t EVYield;
    struct
    {
      uint16_t EV_HP : 2;
      uint16_t EV_ATK : 2;
      uint16_t EV_DEF : 2;
      uint16_t EV_SPE : 2;
      uint16_t EV_SPA : 2;
      uint16_t EV_SPD : 2;
      uint16_t reserved : 4;
    };
  };
  uint8_t Gender;
  uint8_t HatchCycles;
  uint8_t BaseFriendship;
  uint8_t EXPGrowth;
  uint8_t EggGroup1;
  uint8_t EggGroup2;
  uint16_t Ability1;
  uint16_t Ability2;
  uint16_t AbilityH;
  uint16_t FormStatsIndex;  ///< 不同形态的编号起始位置
  uint8_t FormCount;        ///< 有几种形态
  uint8_t Color;
  uint8_t IsPresentInGame;
  uint8_t DexGroup;
  uint16_t DexIndex;
  uint16_t Height;
  uint16_t Weight;
  uint16_t HatchSpecies;
  uint16_t LocalFormIndex;
  uint16_t RegionalFlags;
  uint16_t RegionalFormIndex;
  // 该读0x2C了,以下是能学的技能机标记位,总共172种
  // 21.5个8bit就够了,用了24个8bit存
  uint32_t reserved_for_tmhm_1;
  uint32_t reserved_for_tmhm_2;
  uint32_t reserved_for_tmhm_3;
  uint32_t reserved_for_tmhm_4;
  uint32_t reserved_for_tmhm_5;
  uint32_t reserved_for_tmhm_6;
};

#pragma pack()

class PersonalTable9SV
{
 public:
  static PersonalTable9SV& GetInstance()
  {
    static PersonalTable9SV table;
    return table;
  }

 public:
  const PersonalInfo9SV& GetSpeciesWithForm(uint16_t species,
                                            uint8_t form) const
  {
    int index = array_[species].GetSpeciesFormIndex(species, form);
    return array_[index];
  }

 private:
  static const int SIZE = PersonalInfo9SV::SIZE;
  static const int MaxSpecies = 1010;
  PersonalInfo9SV* array_{};

 public:
  static const int MaxSpeciesID = MaxSpecies;

 private:
  PersonalTable9SV();
  ~PersonalTable9SV();
  PersonalTable9SV(const PersonalTable9SV&) = delete;
  PersonalTable9SV(PersonalTable9SV&&) = delete;
  PersonalTable9SV& operator=(const PersonalTable9SV&) = delete;
  PersonalTable9SV& operator=(PersonalTable9SV&&) = delete;
};

enum class AbilityPermission : int8_t
{
  Any12H = -1,
  Any12 = 0,
  OnlyFirst = 1,
  OnlySecond = 2,
  OnlyHidden = 4,
};

struct EncounterTera9
{
  static const int SIZE = 0x18;

  uint16_t Species;
  uint8_t Form;
  uint8_t Gender;  ///< 注意有个减1的操作
  int8_t Ability;
  uint8_t FlawlessIVCount;
  uint8_t Shiny;
  uint8_t Level;
  uint16_t Move1;
  uint16_t Move2;
  uint16_t Move3;
  uint16_t Move4;
  uint8_t TeraType;
  uint8_t Index;
  uint8_t Stars;
  uint8_t RandRate;
  uint16_t RandRateMinScarlet;
  uint16_t RandRateMinViolet;
};

class EncounterTera9Table
{
 public:
  static EncounterTera9Table& GetInstance()
  {
    static EncounterTera9Table table;
    return table;
  }

 public:
  using FnOnRaidVisitd =
      std::function<void(int index, const EncounterTera9& raid)>;
  void IterateRaid(FnOnRaidVisitd on_raid_visited) const;

  const EncounterTera9& GetEncounter(int index) const { return array_[index]; }

 private:
  int count_{};
  EncounterTera9* array_{};

 private:
  EncounterTera9Table();
  ~EncounterTera9Table();
  EncounterTera9Table(const EncounterTera9Table&) = delete;
  EncounterTera9Table(EncounterTera9Table&&) = delete;
  EncounterTera9Table& operator=(const EncounterTera9Table&) = delete;
  EncounterTera9Table& operator=(EncounterTera9Table&&) = delete;
};

#endif  // ROAMINGID_PERSONALTABLE9SV_H

//
// Created by 王嵩 on 2022/12/15.
//

#ifndef ROAMINGID_READABLECHSDICT_H
#define ROAMINGID_READABLECHSDICT_H

#include <string>
#include <vector>

class ReadableCHSDict
{
 public:
  static ReadableCHSDict& GetInstance()
  {
    static ReadableCHSDict dict;
    return dict;
  }

 public:
  const std::string& GetSpecies(uint16_t index) const
  {
    return species_dict_[index];
  }
  std::string GetSpecies(uint16_t species, uint8_t form) const;
  const std::string& GetType(uint8_t index) const { return type_dict_[index]; }
  const std::string& GetGender(uint8_t index) const
  {
    return gender_dict_[index];
  }
  const std::string& GetNature(uint8_t index) const
  {
    return nature_dict_[index];
  }
  const std::string& GetAbility(uint8_t index) const
  {
    return ability_dict_[index];
  }
  const std::string& GetMove(uint16_t index) const { return move_dict_[index]; }

 private:
  std::vector<std::string> species_dict_;
  std::vector<std::string> type_dict_;
  std::vector<std::string> gender_dict_;
  std::vector<std::string> nature_dict_;
  std::vector<std::string> ability_dict_;
  std::vector<std::string> move_dict_;

 private:
  ReadableCHSDict();
  ~ReadableCHSDict() = default;
  ReadableCHSDict(const ReadableCHSDict&) = delete;
  ReadableCHSDict(ReadableCHSDict&&) = delete;
  ReadableCHSDict& operator=(const ReadableCHSDict&) = delete;
  ReadableCHSDict& operator=(ReadableCHSDict&&) = delete;
};

#endif  // ROAMINGID_READABLECHSDICT_H

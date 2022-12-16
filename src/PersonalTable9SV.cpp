//
// Created by 王嵩 on 2022/12/15.
//

#include "PersonalTable9SV.h"

#include <cstring>
#include <fstream>
#include <string>

#include "ReadableCHSDict.h"

#define OUTPUT_CSV

void* read_binary_data(const std::string& file_path, uint64_t& length)
{
  FILE* fp = fopen(file_path.c_str(), "rb");
  if (nullptr == fp)
  {
    return nullptr;
  }

  fseek(fp, 0, SEEK_END);
  auto file_length = static_cast<size_t>(ftell(fp));
  if (0 == file_length)
  {
    return nullptr;
  }

  length = file_length;
  void* mem = calloc(length, sizeof(uint8_t));
  if (mem == nullptr)
  {
    return nullptr;
  }

  fseek(fp, 0, SEEK_SET);
  fread(mem, sizeof(char), file_length, fp);
  return mem;
}

PersonalTable9SV::PersonalTable9SV()
{
  std::string path = "../conf/personal_sv";

  uint64_t length{};
  void* buffer = read_binary_data(path, length);

  int count = length / PersonalTable9SV::SIZE;

  array_ = new PersonalInfo9SV[count];
  memcpy(array_, buffer, length);

  free(buffer);
}

PersonalTable9SV::~PersonalTable9SV() { delete[] array_; }

EncounterTera9Table::EncounterTera9Table()
{
  std::string path = "../conf/encounter_gem_paldea.pkl";

  uint64_t length{};
  void* buffer = read_binary_data(path, length);

  count_ = length / EncounterTera9::SIZE;

  array_ = new EncounterTera9[count_];
  memcpy(array_, buffer, length);

#ifdef OUTPUT_CSV
  std::ofstream ofstr("/Users/wang.song/sv_raid.csv");
  ofstr << "name,star,scarlet_index,violet_index,level,";
  ofstr << "move1,move2,move3,move4" << std::endl;

  const auto& chs_dict = ReadableCHSDict::GetInstance();
  for (int i = 0; i < count_; ++i)
  {
    const auto& raid = array_[i];

    ofstr << chs_dict.GetSpecies(raid.Species, raid.Form) << ",";
    ofstr << int(raid.Stars) << ",";
    ofstr << int(raid.RandRateMinScarlet) << ",";
    ofstr << int(raid.RandRateMinViolet) << ",";
    ofstr << int(raid.Level) << ",";
    ofstr << chs_dict.GetMove(raid.Move1) << ",";
    ofstr << chs_dict.GetMove(raid.Move2) << ",";
    ofstr << chs_dict.GetMove(raid.Move3) << ",";
    ofstr << chs_dict.GetMove(raid.Move4) << std::endl;
  }
  ofstr.close();
#endif

  free(buffer);
}

EncounterTera9Table::~EncounterTera9Table() { delete[] array_; }

void EncounterTera9Table::IterateRaid(FnOnRaidVisitd on_raid_visited) const
{
  for (int i = 0; i < count_; ++i)
  {
    const auto& raid = array_[i];
    on_raid_visited(i, raid);
  }
}

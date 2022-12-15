//
// Created by 王嵩 on 2022/11/27.
//

#include "teran_finder.h"

#include <fstream>
#include <iostream>
#include <list>

#include "PersonalTable9SV.h"
#include "ReadableCHSDict.h"

#include "thread_pool.h"

TeranFinder::TeranFinder()
{
  scarlet_species_.resize(7);
  violet_species_.resize(7);

  const auto& encounter_table = EncounterTera9Table::GetInstance();

  std::vector<uint16_t> s_max_index_of_each_star_level(7, 0);
  std::vector<uint16_t> v_max_index_of_each_star_level(7, 0);

  encounter_table.IterateRaid(
      [&](int index, const EncounterTera9& encounter)
      {
        uint8_t star = encounter.Stars;
        uint16_t old_s_data = s_max_index_of_each_star_level[star];
        if (encounter.RandRateMinScarlet != 65535)
          s_max_index_of_each_star_level[star] =
              std::max(old_s_data, encounter.RandRateMinScarlet);
        uint16_t old_v_data = v_max_index_of_each_star_level[star];
        if (encounter.RandRateMinViolet != 65535)
          v_max_index_of_each_star_level[star] =
              std::max(old_v_data, encounter.RandRateMinViolet);
      });

  // 这么多个坑位
  for (int i = 1; i <= 6; ++i)
  {
    scarlet_species_[i].resize(s_max_index_of_each_star_level[i] / 100 + 1);
    violet_species_[i].resize(v_max_index_of_each_star_level[i] / 100 + 1);
  }

  encounter_table.IterateRaid(
      [&](int index, const EncounterTera9& encounter)
      {
        uint8_t star = encounter.Stars;
        if (encounter.RandRateMinScarlet != 65535)
        {
          scarlet_species_[star][encounter.RandRateMinScarlet / 100] = index;
        }
        if (encounter.RandRateMinViolet != 65535)
        {
          violet_species_[star][encounter.RandRateMinViolet / 100] = index;
        }
      });
}

std::string GetTeranString(uint32_t teran_type)
{
  switch (teran_type)
  {
    case 0:
      return "一般";
    case 1:
      return "格斗";
    case 2:
      return "飞行";
    case 3:
      return "毒";
    case 4:
      return "地面";
    case 5:
      return "岩石";
    case 6:
      return "虫";
    case 7:
      return "幽灵";
    case 8:
      return "钢";
    case 9:
      return "火";
    case 10:
      return "水";
    case 11:
      return "草";
    case 12:
      return "电";
    case 13:
      return "超能力";
    case 14:
      return "冰";
    case 15:
      return "龙";
    case 16:
      return "恶";
    case 17:
      return "妖精";
    default:
      __builtin_trap();
  }
}

void TeranFinder::FindAllResult()
{
  // 1.循环所有seed
  //   1.1 确定ec/pid/shiny/ivs(直接跟过滤条件作用)
  //   1.2 循环朱紫版本,是否黑坑

  ThreadPool thread_pool(std::thread::hardware_concurrency());
  std::list<std::future<std::vector<Result>>> futures;

  uint32_t seed_count_in_one_batch{8192};
  uint64_t batch_count = 0x100000000L / seed_count_in_one_batch;

  for (uint64_t batch = 0; batch < batch_count; ++batch)
  {
    uint32_t seed_beg = seed_count_in_one_batch * batch;
    uint32_t seed_end = seed_count_in_one_batch * (batch + 1);

    futures.emplace_back(thread_pool.enqueue(
        [this, seed_beg, seed_end]
        {
          std::vector<Result> result_array;

          for (ulong seed = seed_beg; seed < seed_end; ++seed)
          {
            std::vector<Result> result_for_one_seed;
            generate_info(seed, result_for_one_seed);

            for (const auto& result : result_for_one_seed)
            {
              bool is_shiny = result.shiny_type;

              bool is_6v =
                  (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 31 &&
                   result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                   result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 31);
              bool is_5v0a =
                  (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 0 &&
                   result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                   result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 31);
              bool is_5v0e =
                  (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 31 &&
                   result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                   result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 0);
              bool is_4v0a0e =
                  (result.ivs.IV_HP == 31 && result.ivs.IV_ATK == 0 &&
                   result.ivs.IV_DEF == 31 && result.ivs.IV_SPA == 31 &&
                   result.ivs.IV_SPD == 31 && result.ivs.IV_SPE == 0);

              // 一样都不沾就算了吧
              if (!is_shiny && !is_6v && !is_5v0a && !is_5v0e && !is_4v0a0e)
                continue;

              result_array.emplace_back(result);
            }
          }
          return result_array;
        }));
  }

  for (auto& f : futures)
  {
    const auto& result_array = f.get();
    for (const auto& result : result_array)
      result_array_.push_back(result);
  }

  std::cout << "总结" << result_array_.size() << "种" << std::endl;
  /*
    std::ofstream ofstr("result_only_shiny.txt");
    ofstr << "版本,seed,星级,太晶,图鉴编号,ec,pid,是否闪光,";
    ofstr << "hp,atk,def,spa,spd,spe" << std::endl;
    for (const auto& result : result_array_)
    {
      if (result.encounter_index[0] == result.encounter_index[1])
      {
        ofstr << "朱/紫"
              << ",";
        ofstr << std::hex << "0x" << result.seed << ",";
        ofstr << std::dec << result.star_count << ","
              << GetTeranString(result.teran_type) << ","
              << result.encounter_index[0] << ",";
        ofstr << std::hex << "0x" << result.ec << ",0x" << result.pid << ",";
        ofstr << std::dec << result.shiny_type << ",";
        ofstr << std::dec << result.ivs.IV_HP << "," << result.ivs.IV_ATK << ","
              << result.ivs.IV_DEF << "," << result.ivs.IV_SPA << ","
              << result.ivs.IV_SPD << "," << result.ivs.IV_SPE << std::endl;
      }
      else
      {
        ofstr << "朱"
              << ",";
        ofstr << std::hex << "0x" << result.seed << ",";
        ofstr << std::dec << result.star_count << ","
              << GetTeranString(result.teran_type) << ","
              << result.encounter_index[0] << ",";
        ofstr << std::hex << "0x" << result.ec << ",0x" << result.pid << ",";
        ofstr << std::dec << result.shiny_type << ",";
        ofstr << std::dec << result.ivs.IV_HP << "," << result.ivs.IV_ATK << ","
              << result.ivs.IV_DEF << "," << result.ivs.IV_SPA << ","
              << result.ivs.IV_SPD << "," << result.ivs.IV_SPE << std::endl;

        ofstr << "紫"
              << ",";
        ofstr << std::hex << "0x" << result.seed << ",";
        ofstr << std::dec << result.star_count << ","
              << GetTeranString(result.teran_type) << ","
              << result.encounter_index[1] << ",";
        ofstr << std::hex << "0x" << result.ec << ",0x" << result.pid << ",";
        ofstr << std::dec << result.shiny_type << ",";
        ofstr << std::dec << result.ivs.IV_HP << "," << result.ivs.IV_ATK << ","
              << result.ivs.IV_DEF << "," << result.ivs.IV_SPA << ","
              << result.ivs.IV_SPD << "," << result.ivs.IV_SPE << std::endl;
      }
    }
    */
}

int GetStarCount(uint Difficulty, int Progress, bool IsBlack)
{
  if (IsBlack)
    return 6;

  switch (Progress)
  {
    case 0:
    {
      if (Difficulty > 80)
        return 2;
      else
        return 1;
    }
    case 1:
    {
      if (Difficulty > 70)
        return 3;
      else if (Difficulty > 30)
        return 2;
      else
        return 1;
    }
    case 2:
    {
      if (Difficulty > 70)
        return 4;
      else if (Difficulty > 40)
        return 3;
      else if (Difficulty > 20)
        return 2;
      else
        return 1;
    }
    case 3:
    {
      if (Difficulty > 75)
        return 5;
      else if (Difficulty > 40)
        return 4;
      else
        return 3;
    }
    case 4:
    {
      if (Difficulty > 70)
        return 5;
      else if (Difficulty > 30)
        return 4;
      else
        return 3;
    }
    default:
      return 1;
  }
}

void TeranFinder::generate_info(uint32_t seed,
                                std::vector<Result>& result_array)
{
  const auto& encounter_table = EncounterTera9Table::GetInstance();

  for (int i = 0; i < 2; ++i)
  {
    bool is_black = i;

    for (int j = 0; j < 2; ++j)
    {
      Xoroshiro128Plus rng(seed);
      // 游戏分了5个阶段，暂时只输出最后一个阶段的
      uint32_t star_count =
          is_black ? 6 : GetStarCount(rng.NextInt(100), 4, is_black);

      Result result;
      result.seed = seed;
      result.star_count = star_count;

      bool is_scarlet = j;

      result.is_scarlet = is_scarlet;

      const auto& species_of_star = is_scarlet ? scarlet_species_[star_count]
                                               : violet_species_[star_count];

      int species_roll = rng.NextInt(species_of_star.size() * 100);
      int encounter_index = species_of_star[species_roll / 100];

      result.encounter_index = encounter_index;

      const auto& encounter = encounter_table.GetEncounter(encounter_index);

      generate_pkm_info(seed, encounter, result);
      result_array.emplace_back(result);
    }
  }
}

void TeranFinder::generate_pkm_info(uint32_t seed,
                                    const EncounterTera9& encounter,
                                    Result& result)
{
  // ec/tidsid/pid/shiny
  // ivs
  // ability(species)
  // gender(species)
  // nature(species)
  // height/weight/scale
  // tera_type

  const auto& species_table = PersonalTable9SV::GetInstance();
  const auto& species =
      species_table.GetSpeciesWithForm(encounter.Species, encounter.Form);

  result.teran_type = Xoroshiro128Plus(seed).NextInt(18);

  Xoroshiro128Plus rng(seed);

  result.ec = (uint)rng.NextInt();
  result.tidsid = (uint)rng.NextInt();
  result.pid = (uint)rng.NextInt();
  result.shiny_type =
      (((result.pid >> 16) ^ (result.pid & 0xFFFF)) >> 4) ==
              (((result.tidsid >> 16) ^ (result.tidsid & 0xFFFF)) >> 4)
          ? 1
          : 0;

  static constexpr uint kFlawlessValue = 31;
  static constexpr int kUnsetIV = -1;
  std::vector<int> ivs{kUnsetIV, kUnsetIV, kUnsetIV,
                       kUnsetIV, kUnsetIV, kUnsetIV};

  // 用完美个数取index,剩下的随机个体值
  int determined = 0;
  while (determined < encounter.FlawlessIVCount)
  {
    int idx = (int)rng.NextInt(6);
    if (ivs[idx] != kUnsetIV)
      continue;

    ivs[idx] = kFlawlessValue;
    ++determined;
  }

  for (int idx = 0; idx < ivs.size(); ++idx)
  {
    if (ivs[idx] == kUnsetIV)
    {
      ivs[idx] = (int)rng.NextInt(kFlawlessValue + 1);
    }
  }

  result.ivs.IV_HP = ivs[0];
  result.ivs.IV_ATK = ivs[1];
  result.ivs.IV_DEF = ivs[2];
  result.ivs.IV_SPA = ivs[3];
  result.ivs.IV_SPD = ivs[4];
  result.ivs.IV_SPE = ivs[5];

  switch (encounter.Ability)
  {
    case 0:  // Any12
    {
      int ability = rng.NextInt(2);
      switch (ability)
      {
        case 0:
          result.ability = species.Ability1;
          break;
        case 1:
          result.ability = species.Ability2;
          break;
        default:
          __builtin_trap();
      }
      break;
    }
    case 1:  // Any12H
    {
      int ability = rng.NextInt(3);
      switch (ability)
      {
        case 0:
          result.ability = species.Ability1;
          break;
        case 1:
          result.ability = species.Ability2;
          break;
        case 2:
          result.ability = species.AbilityH;
          break;
        default:
          __builtin_trap();
      }
      break;
    }
    case 2:  // OnlyFirst
      result.ability = species.Ability1;
      break;
    case 3:  // OnlySecond
      result.ability = species.Ability2;
      break;
    case 4:  // OnlyHidden
      result.ability = species.AbilityH;
      break;
  }

  switch (species.Gender)
  {
    case PersonalInfo9SV::RatioMagicGenderless:
      result.gender = 2;
      break;
    case PersonalInfo9SV::RatioMagicFemale:
      result.gender = 1;
      break;
    case PersonalInfo9SV::RatioMagicMale:
      result.gender = 0;
      break;
    default:
    {
      int rate = rng.NextInt(100);
      switch (species.Gender)
      {
        case 0x1F:
          result.gender = rate < 12 ? 1 : 0;
          break;
        case 0x3F:
          result.gender = rate < 25 ? 1 : 0;
          break;
        case 0x7F:
          result.gender = rate < 50 ? 1 : 0;
          break;
        case 0xBF:
          result.gender = rate < 75 ? 1 : 0;
          break;
        case 0xE1:
          result.gender = rate < 89 ? 1 : 0;
          break;
        default:
          __builtin_trap();
      }
    }
  }

  // nature一定是random
  if (encounter.Species == 849)
  {
    // 颤弦蝾螈
    static std::vector<uint8_t> nature0{3,  4,  2,  8, 9, 19, 22,
                                        11, 13, 14, 0, 6, 24};
    static std::vector<uint8_t> nature1{1,  5,  7,  10, 12, 15,
                                        16, 17, 18, 20, 21, 23};

    if (encounter.Form == 0)
    {
      result.nature = nature0[rng.NextInt(nature0.size())];
    }
    else
    {
      result.nature = nature1[rng.NextInt(nature1.size())];
    }
  }
  else
  {
    result.nature = rng.NextInt(25);
  }

  result.height = rng.NextInt(0x81) + rng.NextInt(0x80);
  result.weight = rng.NextInt(0x81) + rng.NextInt(0x80);
  result.scale = rng.NextInt(0x81) + rng.NextInt(0x80);
}

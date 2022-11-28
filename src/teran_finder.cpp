//
// Created by 王嵩 on 2022/11/27.
//

#include "teran_finder.h"

#include <fstream>
#include <iostream>
#include <list>

#include "thread_pool.h"

TeranFinder::TeranFinder()
{
  scarlet_species_.resize(7);
  scarlet_species_[1] = {172, 174, 48,  298,  187, 191,  206, 280, 278, 283,
                         285, 339, 333, 396,  401, 415,  438, 456, 548, 585,
                         667, 669, 672, 734,  739, 744,  753, 761, 833, 819,
                         821, 848, 872, 856,  915, 918,  920, 194, 922, 957,
                         968, 929, 935, 963,  972, 1006, 924, 926, 954, 602,
                         840, 940, 90,  1003, 843, 96,   970, 296};
  scarlet_species_[2] = {25,   39,  50,  52,  54,  56,  58,  79,  92,  100, 129,
                         179,  198, 204, 216, 228, 231, 287, 307, 325, 331, 353,
                         361,  370, 403, 418, 422, 425, 453, 456, 551, 574, 590,
                         624,  661, 690, 749, 757, 878, 837, 859, 942, 938, 945,
                         1000, 769, 846, 81,  88,  90,  854, 974, 434};
  scarlet_species_[3] = {
      25,  49,  93,  132, 129, 133, 113, 147, 180, 183,  185,  188, 610,
      203, 206, 211, 215, 234, 246, 281, 279, 296, 302,  322,  335, 336,
      402, 404, 417, 436, 443, 447, 449, 459, 550, 570,  613,  627, 633,
      704, 712, 714, 735, 747, 919, 921, 933, 947, 949,  876,  876, 871,
      860, 820, 838, 397, 779, 857, 936, 955, 964, 1003, 1001, 636, 662,
      192, 916, 916, 762, 575, 702, 765, 822, 966};
  scarlet_species_[4] = {
      26,  40,  51,  53,  55,  57,  82,  89,  91,  101,  123, 148, 1009,
      205, 214, 217, 232, 247, 284, 288, 184, 308, 324,  326, 354, 357,
      419, 435, 444, 457, 479, 549, 552, 586, 603, 611,  614, 615, 625,
      634, 668, 673, 701, 705, 740, 741, 745, 745, 754,  775, 874, 847,
      855, 930, 932, 939, 941, 946, 960, 961, 128, 1007, 97,  132, 423,
      426, 454, 594, 834, 870, 844, 873, 917, 336, 923,  925, 927, 335,
      953, 928, 958, 969, 971, 702, 416, 181, 332, 340};
  scarlet_species_[5] = {
      26,  59,  80,   91,   123,  130,  133, 94,  132, 149, 199, 212, 225,
      229, 242, 248,  282,  289,  302,  297, 323, 820, 334, 362, 398, 405,
      426, 430, 437,  445,  450,  460,  461, 462, 475, 478, 479, 553, 571,
      576, 591, 612,  604,  628,  635,  637, 663, 671, 691, 706, 713, 715,
      763, 778, 823,  849,  870,  879,  839, 871, 861, 855, 841, 842, 858,
      931, 934, 937,  943,  944,  948,  951, 952, 952, 952, 956, 959, 965,
      128, 975, 1002, 1004, 1008, 1010, 973, 967, 286, 765, 750, 876, 876};
  scarlet_species_[6] = {
      199,  136, 135, 134, 197, 196, 470,  471,  700,  130,  149, 242,  248,
      324,  279, 445, 462, 635, 637, 663,  706,  591,  612,  691, 713,  778,
      823,  871, 931, 956, 944, 948, 1002, 1004, 1008, 1010, 128, 128,  132,
      946,  962, 967, 958, 953, 928, 745,  94,   398,  282,  475, 1007, 286,
      1009, 748, 951, 873, 214, 965, 969,  971,  450,  212,  959, 973,  943};

  violet_species_.resize(7);
  violet_species_[1] = {172, 174, 48,  298,  187, 191,  206, 280, 278, 283,
                        285, 339, 333, 396,  401, 415,  438, 456, 548, 585,
                        667, 669, 672, 734,  739, 744,  753, 761, 833, 819,
                        821, 848, 872, 856,  915, 918,  920, 194, 922, 957,
                        968, 929, 935, 963,  972, 1006, 924, 926, 954, 602,
                        840, 940, 90,  1003, 843, 96,   970, 296};
  violet_species_[2] = {25,   39,  50,  52,  54,  56,  58,  79,  92,  100, 129,
                        179,  198, 204, 216, 228, 231, 287, 307, 325, 331, 353,
                        361,  370, 403, 418, 422, 453, 456, 551, 574, 590, 200,
                        624,  661, 692, 749, 757, 878, 837, 859, 942, 938, 945,
                        1000, 769, 316, 846, 81,  88,  90,  854, 974};
  violet_species_[3] = {25,  49,  93,  132, 129, 133, 113,  147,  180, 183, 185,
                        188, 610, 203, 206, 211, 215, 234,  281,  279, 296, 302,
                        322, 335, 336, 371, 402, 404, 417,  436,  443, 447, 449,
                        459, 550, 570, 613, 627, 704, 712,  714,  735, 747, 885,
                        919, 921, 933, 947, 949, 876, 876,  871,  860, 820, 838,
                        397, 779, 857, 936, 955, 964, 1003, 1001, 636, 662, 192,
                        916, 916, 762, 575, 702, 766, 822,  966};
  violet_species_[4] = {26,  40,   51,  53,  55,  57,  82,   89,  91,  101, 123,
                        148, 1009, 205, 214, 217, 232, 284,  288, 184, 308, 317,
                        324, 326,  354, 357, 372, 419, 444,  457, 479, 549, 552,
                        586, 603,  611, 614, 615, 625, 668,  673, 701, 705, 740,
                        741, 745,  745, 754, 775, 875, 847,  855, 886, 930, 932,
                        939, 941,  946, 960, 961, 128, 1007, 97,  132, 423, 454,
                        594, 834,  870, 844, 873, 917, 336,  923, 925, 927, 335,
                        953, 928,  958, 969, 971, 702, 416,  181, 332, 340};
  violet_species_[5] = {
      26,  59,  80,   91,   123,  130,  133, 94,  132, 149, 199, 212, 225,
      229, 242, 282,  289,  302,  297,  323, 820, 334, 362, 373, 398, 405,
      430, 429, 437,  445,  450,  460,  461, 462, 475, 478, 479, 553, 571,
      576, 591, 612,  604,  628,  637,  663, 671, 693, 706, 713, 715, 763,
      778, 823, 849,  870,  879,  839,  871, 861, 855, 841, 842, 858, 887,
      931, 934, 937,  943,  944,  948,  951, 952, 952, 952, 956, 959, 965,
      128, 975, 1002, 1005, 1008, 1010, 973, 967, 286, 766, 750, 876, 876};
  violet_species_[6] = {
      199, 136, 135, 134, 197, 196,  470,  471,  700,  130, 149,  242, 324,
      279, 373, 445, 462, 637, 663,  706,  591,  612,  693, 713,  778, 823,
      871, 931, 956, 944, 948, 1002, 1005, 1008, 1010, 128, 128,  132, 946,
      962, 967, 958, 953, 928, 745,  94,   398,  282,  475, 1007, 286, 1009,
      748, 951, 873, 214, 965, 969,  971,  450,  212,  959, 973,  943, 887};
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
              if (result.shiny_type != 1)
                continue;

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
              if (!is_6v && !is_5v0a && !is_5v0e && !is_4v0a0e)
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

  std::ofstream ofstr("result_only_shiny.txt");
  ofstr << "版本,seed,星级,太晶,图鉴编号,ec,pid,是否闪光,";
  ofstr << "hp,atk,def,spa,spd,spe" << std::endl;
  for (const auto& result : result_array_)
  {
    if (result.scarlet_species == result.violet_species)
    {
      ofstr << "朱/紫"
            << ",";
      ofstr << std::hex << "0x" << result.seed << ",";
      ofstr << std::dec << result.star_count << ","
            << GetTeranString(result.teran_type) << ","
            << result.scarlet_species << ",";
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
            << result.scarlet_species << ",";
      ofstr << std::hex << "0x" << result.ec << ",0x" << result.pid << ",";
      ofstr << std::dec << result.shiny_type << ",";
      ofstr << std::dec << result.ivs.IV_HP << "," << result.ivs.IV_ATK << ","
            << result.ivs.IV_DEF << "," << result.ivs.IV_SPA << ","
            << result.ivs.IV_SPD << "," << result.ivs.IV_SPE << std::endl;

      ofstr << "紫"
            << ",";
      ofstr << std::hex << "0x" << result.seed << ",";
      ofstr << std::dec << result.star_count << ","
            << GetTeranString(result.teran_type) << "," << result.violet_species
            << ",";
      ofstr << std::hex << "0x" << result.ec << ",0x" << result.pid << ",";
      ofstr << std::dec << result.shiny_type << ",";
      ofstr << std::dec << result.ivs.IV_HP << "," << result.ivs.IV_ATK << ","
            << result.ivs.IV_DEF << "," << result.ivs.IV_SPA << ","
            << result.ivs.IV_SPD << "," << result.ivs.IV_SPE << std::endl;
    }
  }
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
  uint32_t teran_type = Xoroshiro128Plus(seed).NextInt(18);

  for (int i = 0; i < 2; ++i)
  {
    bool is_black = i;

    Xoroshiro128Plus rng(seed);
    // 注意这里会跳过一次rng的调用
    uint32_t star_count =
        is_black ? 6 : GetStarCount(rng.NextInt(100), 4, is_black);

    Result result;
    result.seed = seed;
    result.teran_type = teran_type;
    result.star_count = star_count;

    {
      // 朱
      const auto& species_table = scarlet_species_[star_count];
      int species_roll = rng.NextInt(species_table.size() * 100);
      result.scarlet_species = species_table[species_roll / 100];
    }

    {
      // 紫
      const auto& species_table = violet_species_[star_count];
      int species_roll = rng.NextInt(species_table.size() * 100);
      result.violet_species = species_table[species_roll / 100];
    }

    generate_pkm_info(seed, star_count - 1, result);

    result_array.emplace_back(result);
  }
}

void TeranFinder::generate_pkm_info(uint32_t seed, int miniv, Result& result)
{
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
  while (determined < miniv)
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
}

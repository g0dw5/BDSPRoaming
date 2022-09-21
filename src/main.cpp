#include <cmath>
#include <iostream>
#include <list>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rng_pokemon_finder.h"

#include "G3RNG.h"
#include "arg_op.h"
#include "thread_pool.h"

using namespace OpUtils;

class IProgress
{
 public:
  virtual void Progress(double dProgress, const char* pszInfo) = 0;

  virtual void ProgOver(bool bSuccess, const char* pszInfo) = 0;
};

class CmdProgress : public IProgress
{
 public:
  CmdProgress()
  {
    sprintf(m_szFormat, "%%3.0%df", m_precision);
    m_pszPercent = (char*)calloc(m_szPercentLen, sizeof(char));
    tb_ = std::chrono::high_resolution_clock::now();
  }

  ~CmdProgress()
  {
    free(m_pszPercent);
    m_pszPercent = nullptr;
  }

 public:
  virtual void Progress(double dProgress, const char* pszInfo)
  {
    const int c_iBarLen(50);

    uint32_t szThisProg = uint32_t(dProgress * 100 * pow(10, m_precision));
    if (szThisProg != m_szLastProg)
    {
      m_szLastProg = szThisProg;

      char bar[c_iBarLen + 1] = {0};
      int nThisTick = (int)ceil(dProgress * c_iBarLen);
      nThisTick = std::min(c_iBarLen, std::max(0, nThisTick));
      for (int i = 0; i < c_iBarLen; ++i)
      {
        if (i < nThisTick - 1)
          bar[i] = '=';
        else if (i == nThisTick - 1)
          bar[i] = dProgress == 1 ? '=' : '>';
        else
          bar[i] = ' ';
      }

      sprintf(m_pszPercent, m_szFormat, szThisProg / pow(10.0, m_precision));
      printf("Progress:[%s]\t%s%%\r", bar, m_pszPercent);
      fflush(stdout);
    }
  }

  virtual void ProgOver(bool bSuccess, const char* pszInfo)
  {
    auto te = std::chrono::high_resolution_clock::now();
    int duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(te - tb_).count();
    m_bSuccess = bSuccess;
    if (bSuccess)
    {
      printf("\n处理成功,耗时%.03f秒.\n", duration / 1000.0);
    }
    else
    {
      printf("\n处理失败,耗时%.03f秒.(%s)\n", duration / 1000.0, pszInfo);
    }
    if (nullptr != pszInfo)
    {
      printf("%s\n", pszInfo);
    }
    c.notify_one();
  }

 public:
  void wait()
  {
    {
      std::unique_lock<std::mutex> lock(m);
      c.wait(lock);
    }
  }

  bool success() { return m_bSuccess; }

 private:
  std::condition_variable c;
  std::mutex m;
  uint32_t m_szLastProg = 0;
  bool m_bSuccess = false;
  const uint32_t m_precision = 2;  // 1-9

  char m_szFormat[7];
  uint32_t m_szPercentLen = 4 + m_precision + 1;
  char* m_pszPercent = nullptr;

  decltype(std::chrono::high_resolution_clock::now()) tb_;
};

void FindPokemon(RNDType mode, int sid, int tid, int hp, int atk, int def,
                 int spa, int spd, int spe, Shiny shiny_type_you_want,
                 int flawless_count,
                 const std::unordered_set<uint>& seed_white_list)
{
  // 这里改你的诉求
  uint G8SID = sid;
  uint G8TID = tid;
  uint HP = hp;
  uint ATK = atk;
  uint DEF = def;
  uint SPA = spa;
  uint SPD = spd;
  uint SPE = spe;
  uint thread_count = std::thread::hardware_concurrency();

  ITrainerID trainer;
  IVs expect_ivs;

  // 内部计算用的真实ID需要转一下
  uint tidsid = G8SID * 1000000 + G8TID;
  trainer.SID = tidsid & 0xFFFF;
  trainer.TID = tidsid >> 16;

  expect_ivs.IV_HP = HP;
  expect_ivs.IV_ATK = ATK;
  expect_ivs.IV_DEF = DEF;
  expect_ivs.IV_SPA = SPA;
  expect_ivs.IV_SPD = SPD;
  expect_ivs.IV_SPE = SPE;

  ulong end_seed = 0x100000000L;
  uint batch_count = thread_count * 256;
  ulong seed_count_in_each_batch = end_seed / batch_count;

  using PKMs = std::vector<PKM>;
  using Futures = std::list<std::future<PKMs>>;

  ThreadPool pool(thread_count);
  Futures futures;
  CmdProgress prog;

  PKMs result_pkms;

  std::thread t(
      [&]()
      {
        for (uint batch = 0; batch < batch_count; ++batch)
        {
          ulong e_beg = seed_count_in_each_batch * (batch);
          ulong e_end = seed_count_in_each_batch * (batch + 1);

          if (batch == batch_count - 1)
            e_end = end_seed;

          futures.emplace_back(pool.enqueue(
              [e_beg, e_end, mode, shiny_type_you_want, flawless_count,
               &trainer, &expect_ivs, &seed_white_list]
              {
                PKMs pkms;

                for (ulong e = e_beg; e < e_end; ++e)
                {
                  if (!seed_white_list.empty() && !seed_white_list.count(e))
                    continue;

                  auto finder = RNGPokemonFinderFactory::CreateFinder(
                      mode, trainer, expect_ivs, e, shiny_type_you_want,
                      flawless_count);
                  if (finder->Step1IsSatisfied())
                  {
                    const auto& pkm = finder->Step2GetPokemon();
                    pkms.push_back(pkm);
                  }
                }

                return pkms;
              }));
        }

        uint32_t over_count{};
        for (auto& f : futures)
        {
          auto pkms = f.get();
          for (const auto& pkm : pkms)
          {
            result_pkms.push_back(pkm);
          }

          prog.Progress(double(over_count + 1) / batch_count, nullptr);
          ++over_count;
        }

        prog.ProgOver(true, nullptr);
      });
  t.detach();

  prog.wait();

  static constexpr uint32_t kMaxOutput = 32;
  uint32_t output_count{};
  for (const auto& pkm : result_pkms)
  {
    std::cout << std::hex << "Seed=" << pkm.seed << std::endl;
    std::cout << std::hex << "Encryption=" << pkm.EncryptionConstant
              << std::endl;
    std::cout << std::hex << "PID=" << pkm.PID << std::endl;
    std::cout << std::dec << "AbilityIndex=" << pkm.AbilityNumber << std::endl;
    std::cout << std::dec << "HeightScalar=" << pkm.HeightScalar << std::endl;
    std::cout << std::dec << "WeightScalar=" << pkm.WeightScalar << std::endl;
    std::cout << "ShinyType=" << GetShinyType(pkm.shiny) << std::endl;
    std::cout << std::endl;

    ++output_count;
    // 防止打印太多(FIXME:有空加点随机性)
    if (output_count > kMaxOutput)
    {
      std::cout << "总共有" << result_pkms.size() << "个结果,只打印了"
                << kMaxOutput << "条" << std::endl;
      break;
    }
  }
}

struct sArgs
{
  RNDType mode{RNDType::kBDSPRoaming};
  int sid{};
  int tid{999999};
  std::string ivs{"31,0,31,31,31,0"};
  Shiny shiny_type_you_want{Shiny::kSquare};
  int flawless_count{3};
};
sArgs g_args;

// 数字正则
std::regex reg("^(0|[1-9][0-9]*|-[1-9][0-9]*)$");

sArgDef g_opt_def[] = {
    {'m', "mode", must_have::must_n, arg_required,
     "计算任务模式,有两种取值:bdsp(找游走怪),swsh(找野生怪).默认bdsp",
     [](char* optarg, void* data, std::string& strErr)
     {
       if (nullptr == optarg)
       {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       RNDType* type = static_cast<RNDType*>(data);
       if ("bdsp" == str)
       {
         *type = RNDType::kBDSPRoaming;
         return true;
       }
       else if ("swsh" == str)
       {
         *type = RNDType::kSWSHOverworld;
         return true;
       }
       else
       {
         strErr = "不支持的模式";
         return false;
       }
     },
     &(g_args.mode)},
    {'s', "sid", must_have::must_y, arg_required, "训练家里ID",
     [](char* optarg, void* data, std::string& strErr)
     {
       if (nullptr == optarg)
       {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       if (!std::regex_match(str.begin(), str.end(), reg))
       {
         strErr = "SID输入非数字";
         return false;
       }

       int sid = std::stoi(optarg);
       if (sid < 0 || sid > 4294)
       {
         strErr = "SID需要在[0,4294]之间";
         return false;
       }

       int* psid = static_cast<int*>(data);
       *psid = sid;
       return true;
     },
     &(g_args.sid)},
    {'t', "tid", must_have::must_y, arg_required, "训练家表ID",
     [](char* optarg, void* data, std::string& strErr)
     {
       if (nullptr == optarg)
       {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       if (!std::regex_match(str.begin(), str.end(), reg))
       {
         strErr = "TID输入非数字";
         return false;
       }

       int tid = std::stoi(optarg);
       if (tid < 0 || tid > 999999)
       {
         strErr = "TID需要在[0,999999]之间";
         return false;
       }

       int* ptid = static_cast<int*>(data);
       *ptid = tid;
       return true;
     },
     &(g_args.tid)},
    {'i', "ivs", must_have::must_y, arg_required,
     "个体值(HP,物攻,物防,特攻,特防,速度.例:31,0,31,31,31,0)",
     [](char* optarg, void* data, std::string& strErr)
     {
       if (nullptr == optarg)
       {
         strErr = "缺失参数值";
         return false;
       }
       std::string* pivs = static_cast<std::string*>(data);
       *pivs = optarg;
       return true;
     },
     &(g_args.ivs)},
    {'h', "shiny", must_have::must_n, arg_required,
     "闪光类型:none/star/square(默认square)",
     [](char* optarg, void* data, std::string& strErr)
     {
       if (nullptr == optarg)
       {
         strErr = "缺失参数值";
         return false;
       }
       std::string str(optarg);
       Shiny* pshiny = static_cast<Shiny*>(data);
       if ("none" == str)
       {
         *pshiny = Shiny::kNone;
         return true;
       }
       else if ("star" == str)
       {
         *pshiny = Shiny::kStar;
         return true;
       }
       else if ("square" == str)
       {
         *pshiny = Shiny::kSquare;
         return true;
       }
       else
       {
         strErr = "不支持的闪光类型";
         return false;
       }
     },
     &(g_args.shiny_type_you_want)},
    {'f', "flawless", must_have::must_n, arg_required,
     "游戏中抓到时至少的完美数(bdsp游走一定是3,swsh好像有0有3)",
     [](char* optarg, void* data, std::string& strErr)
     {
       if (nullptr == optarg)
       {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       if (!std::regex_match(str.begin(), str.end(), reg))
       {
         strErr = "flawless输入非数字";
         return false;
       }

       int flawless_count = std::stoi(optarg);
       if (flawless_count < 0 || flawless_count > 6)
       {
         strErr = "flawless需要在[0,6]之间";
         return false;
       }

       int* pflawless = static_cast<int*>(data);
       *pflawless = flawless_count;
       return true;
     },
     &(g_args.flawless_count)},
};

void FindTrainersByOriginalPID()
{
  // 如果你知道了通过RNG算出来的PID，可以通过这个PID找与它天作之合的训练家
  uint32_t pid = 0xA1AD9F75;
  uint32_t count{};
  for (uint64_t long_id = 0; long_id < 4295000000; ++long_id)
  {
    uint32_t sid = long_id / 1000000;
    uint32_t tid = long_id % 1000000;

    Shiny type = ShinyUtil::GetShinyType(pid, ShinyUtil::GetTidSid(tid, sid));
    if (Shiny::kStar == type)
    {
      ++count;
      if (tid == 210519)
      {
        std::cout << "sid=" << sid << std::endl;
        std::cout << "tid=" << tid << std::endl;
      }
    }
  }
  std::cout << "找到" << count << "位幸运训练家" << std::endl;
}

std::vector<uint> GetSeedsFromPID(uint a, uint b)
{
  uint second = a << 16;
  uint first = b << 16;
  return lcrng.RecoverLower16Bits(first, second);
}
std::vector<uint> GetSeedsFromPIDSkip(uint a, uint b)
{
  uint third = a << 16;
  uint first = b << 16;
  return lcrng.RecoverLower16BitsGap(first, third);
}

uint GetIVChunk(IVs ivs, int start)
{
  uint val = 0;
  for (int i = 0; i < 3; i++)
    val |= ivs[i + start] << (5 * i);
  return val;
}

void GetIVsInt32(uint r1, uint r2, IVs& ivs)
{
  // IV的排序
  //  IV_HP = value[0];
  //  IV_ATK = value[1];
  //  IV_DEF = value[2];
  //  IV_SPE = value[3];
  //  IV_SPA = value[4];
  //  IV_SPD = value[5];

  // SPD
  ivs.IV_SPD = (int)r2 >> 10 & 31;
  // SPA
  ivs.IV_SPA = (int)r2 >> 5 & 31;
  // SPE
  ivs.IV_SPE = (int)r2 & 31;
  // DEF
  ivs.IV_DEF = (int)r1 >> 10 & 31;
  // ATK
  ivs.IV_ATK = (int)r1 >> 5 & 31;
  // HP
  ivs.IV_HP = (int)r1 & 31;
}

void GeneratePIDIVs(PIDType type, uint seed, uint& pid, IVs& ivs,
                    Nature& nature)
{
  const auto& rng = lcrng;

  auto A = rng.Next(seed);
  auto B = rng.Next(A);

  bool skipBetweenPID =
      type == PIDType::Method_3 or type == PIDType::Method_3_Unown;
  if (skipBetweenPID)  // VBlank skip between PID rand() [RARE]
    B = rng.Next(B);

  bool swappedPIDHalves =
      type >= PIDType::Method_1_Unown and type <= PIDType::Method_4_Unown;
  if (swappedPIDHalves)  // switched order of PID halves, "BA.."
    pid = (A & 0xFFFF0000) | B >> 16;
  else
    pid = (B & 0xFFFF0000) | A >> 16;
  nature = static_cast<Nature>(pid % 25);

  auto C = rng.Next(B);
  bool skipIV1Frame =
      type == PIDType::Method_2 or type == PIDType::Method_2_Unown;
  if (skipIV1Frame)  // VBlank skip after PID
    C = rng.Next(C);

  auto D = rng.Next(C);
  bool skipIV2Frame =
      type == PIDType::Method_4 or type == PIDType::Method_4_Unown;
  if (skipIV2Frame)  // VBlank skip between IVs
    D = rng.Next(D);

  GetIVsInt32(C >> 16, D >> 16, ivs);
  if (type == PIDType::Method_1_Roamer)
  {
    // Only store lowest 8 bits of IV data; zero out the other bits.
    ivs.Set(1, ivs[1] & 7);
    for (int i = 2; i < 6; i++)
      ivs.Set(i, 0);
  }
}

bool GetLCRNGMatch(uint top, uint bot, IVs ivs, IVs& pidiv)
{
  // TODO(wang.song) return?
  uint new_pid;
  Nature nature;

  auto reg = GetSeedsFromPID(top, bot);
  auto iv1 = GetIVChunk(ivs, 0);
  auto iv2 = GetIVChunk(ivs, 3);
  for (auto seed : reg)
  {
    // A and B are already used by PID
    auto B = lcrng.Advance(seed, 2);

    // Method 1/2/4 can use 3 different RNG frames
    auto C = lcrng.Next(B);
    auto ivC = C >> 16 & 0x7FFF;
    if (iv1 == ivC)
    {
      auto D = lcrng.Next(C);
      auto ivD = D >> 16 & 0x7FFF;
      if (iv2 == ivD)  // ABCD
      {
        GeneratePIDIVs(PIDType::Method_1, seed, new_pid, pidiv, nature);
        return true;
      }

      auto E = lcrng.Next(D);
      auto ivE = E >> 16 & 0x7FFF;
      if (iv2 == ivE)  // ABCE
      {
        GeneratePIDIVs(PIDType::Method_4, seed, new_pid, pidiv, nature);
        return true;
      }
    }
    else
    {
      auto D = lcrng.Next(C);
      auto ivD = D >> 16 & 0x7FFF;
      if (iv1 != ivD)
        continue;

      auto E = lcrng.Next(D);
      auto ivE = E >> 16 & 0x7FFF;
      if (iv2 == ivE)  // ABDE
      {
        GeneratePIDIVs(PIDType::Method_2, seed, new_pid, pidiv, nature);
        return true;
      }
    }
  }
  reg = GetSeedsFromPIDSkip(top, bot);
  for (auto seed : reg)
  {
    // A and B are already used by PID
    auto C = lcrng.Advance(seed, 3);

    // Method 3
    auto D = lcrng.Next(C);
    auto ivD = D >> 16 & 0x7FFF;
    if (iv1 != ivD)
      continue;
    auto E = lcrng.Next(D);
    auto ivE = E >> 16 & 0x7FFF;
    if (iv2 != ivE)
      continue;
    GeneratePIDIVs(PIDType::Method_3, seed, new_pid, pidiv, nature);
    return true;
  }

  return false;
}

void find_pid(uint tid, uint sid, uint& pid)
{
  uint tsv = (sid ^ tid) >> 3;

  ulong PID = 0;
  uint count{};
  for (; PID < 0x100000000; ++PID)
  {
    uint psv = (int)((pid >> 16 ^ (pid & 0xFFFF)) >> 3);
    if (psv == tsv)
    {
      //      if (count % 1024 == 0)
      //      {
      random();
      //      }
    }
  }
  fprintf(stderr, "遍历%lu个宝可梦,找到%u个有缘怪...\n", PID, count);
}

void find_lucky_trainer(uint pid, uint& tid, uint& sid)
{
  uint psv = (int)((pid >> 16 ^ (pid & 0xFFFF)) >> 3);

  ulong tidsid = 0;
  uint count{};
  for (; tidsid < 0x100000000; ++tidsid)
  {
    uint TID = tidsid >> 16;
    uint SID = tidsid & 0xFFFF;

    uint tsv = (SID ^ TID) >> 3;

    if (psv == tsv)
    {
      if (SID == 0)
      {
        random();
      }
      ++count;
    }
  }
  fprintf(stderr, "遍历%lu个训练家,找到%u个有缘人...\n", tidsid, count);
}

void test_for_g3_ivs()
{
  IVs _6v(31, 31, 31, 31, 31, 31);
  IVs _5v0a(31, 0, 31, 31, 31, 31);
  IVs _5v0e(31, 31, 31, 31, 31, 0);
  IVs _4v0a0e(31, 0, 31, 31, 31, 0);

  {
    uint pid = 0x6d243843;

    uint top = pid >> 16;
    uint bot = pid & 0xFFFF;

    IVs ivs;
    bool success = GetLCRNGMatch(top, bot, _5v0e, ivs);
  }
  //  return;

  struct OneTry
  {
    uint seed;
    uint pid;
    Nature nature;
  };

  std::unordered_map<uint, std::vector<OneTry>> result_hash;
  result_hash.emplace(_6v.data, std::vector<OneTry>());
  result_hash.emplace(_5v0a.data, std::vector<OneTry>());
  result_hash.emplace(_5v0e.data, std::vector<OneTry>());
  result_hash.emplace(_4v0a0e.data, std::vector<OneTry>());

  ulong count{};
  ulong found_count{};
  for (ulong e = 1; e < 0x100000000L; ++e)
  {
    OneTry one_try;
    one_try.seed = e;

    IVs ivs;
    GeneratePIDIVs(PIDType::Method_1, one_try.seed, one_try.pid, ivs,
                   one_try.nature);

    auto iter = result_hash.find(ivs.data);
    if (result_hash.end() != iter)
    {
      ++found_count;
      iter->second.push_back(one_try);
    }
    ++count;
  }

  for (const auto& item : result_hash)
  {
    IVs ivs(item.first);
    std::cout << std::dec << ivs[0] << "," << ivs[1] << "," << ivs[2] << ","
              << ivs[3] << "," << ivs[4] << "," << ivs[5] << std::endl;
    for (const auto& one_try : item.second)
    {
      std::cout << std::hex << "  0x" << one_try.seed << ",0x" << one_try.pid
                << ",";
      std::cout << std::dec << GetNatureStr(one_try.nature) << std::endl;
    }
    std::cout << std::endl;
  }
  fprintf(stderr, "查找%lu次,保留结果%lu个\n", count, found_count);
}

void FindStarShinyTrainer()
{
  std::vector<uint> candidate_seed{0x44e87f35, 0x4dd86a64, 0x54676db9,
                                   0xe7cf1d3c, 0xf7400fb0, 0xfe701ae1};
  uint G8TID = 871229;
  IVs expect_ivs;
  expect_ivs.IV_HP = 31;
  expect_ivs.IV_ATK = 0;
  expect_ivs.IV_DEF = 31;
  expect_ivs.IV_SPA = 31;
  expect_ivs.IV_SPD = 31;
  expect_ivs.IV_SPE = 31;

  for (uint G8SID = 0; G8SID <= 4294; ++G8SID)
  {
    ITrainerID trainer;

    // 内部计算用的真实ID需要转一下
    uint tidsid = G8SID * 1000000 + G8TID;
    trainer.SID = tidsid & 0xFFFF;
    trainer.TID = tidsid >> 16;

    for (auto e : candidate_seed)
    {
      auto finder = RNGPokemonFinderFactory::CreateFinder(
          RNDType::kSWSHOverworld, trainer, expect_ivs, e, Shiny::kStar, 0);
      if (finder->Step1IsSatisfied())
      {
        const auto& pkm = finder->Step2GetPokemon();
        std::cout << "tid=" << G8TID << std::endl;
        std::cout << "sid=" << G8SID << std::endl;
        std::cout << std::hex << "e=0x" << pkm.EncryptionConstant << std::endl;
        std::cout << std::hex << "pid=0x" << pkm.PID << std::endl;
        std::cout << "ability=" << pkm.AbilityNumber << std::endl;
        std::cout << std::dec << "Height=" << pkm.HeightScalar << std::endl;
        std::cout << std::dec << "Weight=" << pkm.WeightScalar << std::endl;
        std::cout << std::endl;
      }
    }
  }
}

int main(int argc, char* argv[])
{
  //    uint sid, tid;
  //    find_lucky_trainer(0xcfbeaf43, tid, sid);

  //  uint pid;
  //  find_pid(24818, 0, pid);
//  test_for_g3_ivs();
//  return 0;

  if (!parse_cmd_args(argc, argv, g_opt_def,
                      sizeof(g_opt_def) / sizeof(sArgDef)))
    return 1;

  std::vector<int> ivs;

  std::istringstream iss(g_args.ivs);
  std::string elem;
  while (std::getline(iss, elem, ','))
  {
    if (!std::regex_match(elem.begin(), elem.end(), reg))
    {
      printf("存在个体值非数字\n");
      return 2;
    }

    ivs.push_back(std::stoi(elem));
  }

  if (ivs.size() != 6)
  {
    printf("个体值不是6个\n");
    return 2;
  }

  if (RNDType::kSWSHOverworld == g_args.mode &&
      Shiny::kStar == g_args.shiny_type_you_want)
  {
    // clang-format off
    fprintf(stderr, "剑盾星闪有bug,高IV自ID很看脸(1/4369),原因见:https://twitter.com/SciresM/status/1197039032112304128\n");
    // clang-format on
  }

  if (RNDType::kBDSPRoaming == g_args.mode && 3 != g_args.flawless_count)
  {
    // clang-format off
    fprintf(stderr, "珍钻复刻游走怪就2只,都是至少3V,flawless建议重填下\n");
    // clang-format on
  }

  // 其实seed确定了，IV就确定了，是不是闪，是什么闪就确定了
  // 换句话说，所有玩这个游戏的，只要是同样的IV，seed100%一样
  // 就可以缓存所有高需求的seed，只对这些seed重新算下pid就行了
  // key的末尾再下划线连上一个flawless_count
  std::unordered_map<std::string, std::unordered_set<uint>>
      precalculated_swsh_ivs_to_seed{
          {"31,31,31,31,31,0_0", {0x857fa070}},
          {"31,0,31,31,31,31_0",
           {0x44e87f35, 0x4dd86a64, 0x54676db9, 0xe7cf1d3c, 0xf7400fb0,
            0xfe701ae1}},
          {"31,0,31,31,31,0_0",
           {0xddf01ac6, 0xd2b74a56, 0xc44f1d1b, 0x7ed778cf, 0x200da840}},
      };
  std::unordered_map<std::string, std::unordered_set<uint>>
      precalculated_bdsp_ivs_to_seed{
          {"31,31,31,31,31,31_3",
           {0x60d489e,  0xf962809,  0x1707a1bc, 0x256512ac, 0x2b2e0bc5,
            0x3e9e0489, 0x429f7bdd, 0x4fcd2b6c, 0x5ce9aa13, 0x65aded09,
            0x6707f338, 0x6b72fe4d, 0x7171a30b, 0x733dd448, 0x7a928de5,
            0x7ef34d82, 0x874a8cd4, 0x90383581, 0x9bd65fdb, 0x9f2f499e,
            0xa01f78d8, 0xafecaffc, 0xb2b8db34, 0xc057809e, 0xc21a8fb4,
            0xcab3bc4d, 0xd970b3b2, 0xe6da6290, 0xebf047c3, 0xf489a30c,
            0xf6cf4c3b, 0xff2be493}},
          {"31,0,31,31,31,0_3",
           {0x21bbd6a, 0x630fbe9f, 0x6a84b6ed, 0x6c6d3a44, 0x745e444f,
            0xc9080389, 0xc937601d, 0xce11bee0}},
          {"31,0,31,31,31,31_3",
           {0xbd5a6b6,  0x1434d7a3, 0x14a4e19d, 0x16356604, 0x258ffeb3,
            0x2811e041, 0x2fbfee90, 0x4318207a, 0x57003093, 0x6ea9022c,
            0x7d5b6f50, 0x86e81468, 0x8b2bc094, 0x91c150eb, 0x93c2fef8,
            0x96e4f3e9, 0xa3fde2ad, 0xdc52ec02, 0xdd32dc34, 0xfaea4706}},
          {"31,31,31,31,31,0_3",
           {0x6f865f2, 0x1eeb4ab9, 0x22c848cd, 0x2fbdd191, 0x3d0b17be,
            0x51097301, 0x558e91a4, 0x5f051ba8, 0x61afeb5f, 0x6be08d41,
            0x6e87f93c, 0x77252c8c, 0x86138402, 0x9aeeee9b, 0xac21a413,
            0xca9ded5d, 0xd013ae00, 0xd6fc7ddd}},
      };

  std::string query_key =
      g_args.ivs + "_" + std::to_string(g_args.flawless_count);
  std::unordered_set<uint> seed_white_list;
  switch (g_args.mode)
  {
    case RNDType::kSWSHOverworld:
    {
      // 找不到加一个就加一个了
      seed_white_list = precalculated_swsh_ivs_to_seed[query_key];
      break;
    }
    case RNDType::kBDSPRoaming:
    {
      // 找不到加一个就加一个了
      seed_white_list = precalculated_bdsp_ivs_to_seed[query_key];
      break;
    }
  }

  FindPokemon(g_args.mode, g_args.sid, g_args.tid, ivs[0], ivs[1], ivs[2],
              ivs[3], ivs[4], ivs[5], g_args.shiny_type_you_want,
              g_args.flawless_count, seed_white_list);
  return 0;
}

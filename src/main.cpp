#include <cmath>
#include <iostream>
#include <list>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rng_pokemon_finder.h"

#include "arg_op.h"
#include "thread_pool.h"

using namespace OpUtils;

class IProgress {
 public:
  virtual void Progress(double dProgress, const char *pszInfo) = 0;

  virtual void ProgOver(bool bSuccess, const char *pszInfo) = 0;
};

class CmdProgress : public IProgress {
 public:
  CmdProgress() {
    sprintf(m_szFormat, "%%3.0%df", m_precision);
    m_pszPercent = (char *) calloc(m_szPercentLen, sizeof(char));
  }

  ~CmdProgress() {
    free(m_pszPercent);
    m_pszPercent = nullptr;
  }

 public:
  virtual void Progress(double dProgress, const char *pszInfo) {
    const int c_iBarLen(50);

    uint32_t szThisProg = uint32_t(dProgress * 100 * pow(10, m_precision));
    if (szThisProg != m_szLastProg) {
      m_szLastProg = szThisProg;

      char bar[c_iBarLen + 1] = {0};
      int nThisTick = (int) ceil(dProgress * c_iBarLen);
      nThisTick = std::min(c_iBarLen, std::max(0, nThisTick));
      for (int i = 0; i < c_iBarLen; ++i) {
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

  virtual void ProgOver(bool bSuccess, const char *pszInfo) {
    m_bSuccess = bSuccess;
    if (bSuccess) {
      printf("\n处理成功\n");
    } else {
      printf("\n处理失败：%s\n", pszInfo);
    }
    if (nullptr != pszInfo) {
      printf("%s\n", pszInfo);
    }
    c.notify_one();
  }

 public:
  void wait() {
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
  const uint32_t m_precision = 2;// 1-9

  char m_szFormat[7];
  uint32_t m_szPercentLen = 4 + m_precision + 1;
  char *m_pszPercent = nullptr;
};

void FindPokemon(RNDType mode,
                 int sid, int tid,
                 int hp, int atk, int def, int spa, int spd, int spe,
                 Shiny shiny_type_you_want, int flawless_count,
                 const std::unordered_set<uint> &seed_white_list) {
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

  std::thread t([&]() {
    for (uint batch = 0; batch < batch_count; ++batch) {
      ulong e_beg = seed_count_in_each_batch * (batch);
      ulong e_end = seed_count_in_each_batch * (batch + 1);

      if (batch == batch_count - 1)
        e_end = end_seed;

      futures.emplace_back(pool.enqueue([e_beg, e_end, mode, shiny_type_you_want, flawless_count, &trainer, &expect_ivs, &seed_white_list] {
        PKMs pkms;

        for (ulong e = e_beg; e < e_end; ++e) {
          if (!seed_white_list.empty() && !seed_white_list.count(e))
            continue;

          auto finder = RNGPokemonFinderFactory::CreateFinder(mode, trainer, expect_ivs, e, shiny_type_you_want, flawless_count);
          if (finder->Step1IsSatisfied()) {
            const auto &pkm = finder->Step2GetPokemon();
            pkms.push_back(pkm);
          }
        }

        return pkms;
      }));
    }

    PKMs shiny_pkms;

    uint32_t over_count{};
    for (auto &f: futures) {
      auto pkms = f.get();
      for (const auto &pkm: pkms) {
        shiny_pkms.push_back(pkm);
      }

      prog.Progress(double(over_count + 1) / batch_count, nullptr);
      ++over_count;
    }

    static constexpr uint32_t kMaxOutput = 32;
    uint32_t output_count{};
    for (const auto &pkm: shiny_pkms) {
      std::cout << std::hex << "Encryption=" << pkm.EncryptionConstant << std::endl;
      std::cout << std::hex << "PID=" << pkm.PID << std::endl;
      std::cout << std::dec << "AbilityIndex=" << pkm.AbilityNumber << std::endl;
      std::cout << std::dec << "HeightScalar=" << pkm.HeightScalar << std::endl;
      std::cout << std::dec << "WeightScalar=" << pkm.WeightScalar << std::endl;
      std::cout << "ShinyType=" << GetShinyType(pkm.shiny) << std::endl;
      std::cout << std::endl;

      ++output_count;
      // 防止打印太多(FIXME:有空加点随机性)
      if (output_count > kMaxOutput) {
        std::cout << "总共有" << shiny_pkms.size() << "个结果,只打印了" << kMaxOutput << "条" << std::endl;
        break;
      }
    }

    prog.ProgOver(true, nullptr);
  });
  t.detach();

  prog.wait();
}

struct sArgs {
  RNDType mode{RNDType::kBDSPRoaming};
  int sid{};
  int tid{999999};
  std::string ivs{"31,0,31,31,31,0"};
  Shiny shiny_type_you_want{Shiny::AlwaysSquare};
  int flawless_count{3};
};
sArgs g_args;

// 数字正则
std::regex reg("^(0|[1-9][0-9]*|-[1-9][0-9]*)$");

sArgDef g_opt_def[] = {
    {'m', "mode", must_have::must_n, arg_required,
     "计算任务模式,有两种取值:bdsp(找游走怪),swsh(找野生怪).默认bdsp",
     [](char *optarg, void *data, std::string &strErr) {
       if (nullptr == optarg) {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       RNDType *type = static_cast<RNDType *>(data);
       if ("bdsp" == str) {
         *type = RNDType::kBDSPRoaming;
         return true;
       } else if ("swsh" == str) {
         *type = RNDType::kSWSHOverworld;
         return true;
       } else {
         strErr = "不支持的模式";
         return false;
       }
     },
     &(g_args.mode)},
    {'s', "sid", must_have::must_y, arg_required,
     "训练家里ID",
     [](char *optarg, void *data, std::string &strErr) {
       if (nullptr == optarg) {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       if (!std::regex_match(str.begin(), str.end(), reg)) {
         strErr = "SID输入非数字";
         return false;
       }

       int sid = std::stoi(optarg);
       if (sid < 0 || sid > 4294) {
         strErr = "SID需要在[0,4294]之间";
         return false;
       }

       int *psid = static_cast<int *>(data);
       *psid = sid;
       return true;
     },
     &(g_args.sid)},
    {'t', "tid", must_have::must_y, arg_required,
     "训练家表ID",
     [](char *optarg, void *data, std::string &strErr) {
       if (nullptr == optarg) {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       if (!std::regex_match(str.begin(), str.end(), reg)) {
         strErr = "TID输入非数字";
         return false;
       }

       int tid = std::stoi(optarg);
       if (tid < 0 || tid > 999999) {
         strErr = "TID需要在[0,999999]之间";
         return false;
       }

       int *ptid = static_cast<int *>(data);
       *ptid = tid;
       return true;
     },
     &(g_args.tid)},
    {'i', "ivs", must_have::must_y, arg_required,
     "个体值(HP,物攻,物防,特攻,特防,速度.例:31,0,31,31,31,0)",
     [](char *optarg, void *data, std::string &strErr) {
       if (nullptr == optarg) {
         strErr = "缺失参数值";
         return false;
       }
       std::string *pivs = static_cast<std::string *>(data);
       *pivs = optarg;
       return true;
     },
     &(g_args.ivs)},
    {'h', "shiny", must_have::must_n, arg_required,
     "闪光类型:none/star/square(默认square)",
     [](char *optarg, void *data, std::string &strErr) {
       if (nullptr == optarg) {
         strErr = "缺失参数值";
         return false;
       }
       std::string str(optarg);
       Shiny *pshiny = static_cast<Shiny *>(data);
       if ("none" == str) {
         *pshiny = Shiny::Never;
         return true;
       } else if ("star" == str) {
         *pshiny = Shiny::AlwaysStar;
         return true;
       } else if ("square" == str) {
         *pshiny = Shiny::AlwaysSquare;
         return true;
       } else {
         strErr = "不支持的闪光类型";
         return false;
       }
     },
     &(g_args.shiny_type_you_want)},
    {'f', "flawless", must_have::must_n, arg_required,
     "游戏中抓到时至少的完美数(bdsp游走一定是3,swsh好像有0有3)",
     [](char *optarg, void *data, std::string &strErr) {
       if (nullptr == optarg) {
         strErr = "缺失参数值";
         return false;
       }

       std::string str(optarg);
       if (!std::regex_match(str.begin(), str.end(), reg)) {
         strErr = "flawless输入非数字";
         return false;
       }

       int flawless_count = std::stoi(optarg);
       if (flawless_count < 0 || flawless_count > 6) {
         strErr = "flawless需要在[0,6]之间";
         return false;
       }

       int *pflawless = static_cast<int *>(data);
       *pflawless = flawless_count;
       return true;
     },
     &(g_args.flawless_count)},
};

int main(int argc, char *argv[]) {
  if (!parse_cmd_args(argc, argv, g_opt_def,
                      sizeof(g_opt_def) / sizeof(sArgDef)))
    return 1;

  std::vector<int> ivs;

  std::istringstream iss(g_args.ivs);
  std::string elem;
  while (std::getline(iss, elem, ',')) {
    if (!std::regex_match(elem.begin(), elem.end(), reg)) {
      printf("存在个体值非数字\n");
      return 2;
    }

    ivs.push_back(std::stoi(elem));
  }

  if (ivs.size() != 6) {
    printf("个体值不是6个\n");
    return 2;
  }

  // 其实seed确定了，IV就确定了，是不是闪，是什么闪就确定了
  // 换句话说，所有玩这个游戏的，只要是同样的IV，seed100%一样
  // 就可以缓存所有高需求的seed，只对这些seed重新算下pid就行了
  // key的末尾再下划线连上一个flawless_count
  std::unordered_map<std::string, std::unordered_set<uint>> precalculated_swsh_ivs_to_seed{
      {"31,31,31,31,31,0_0",
       {0x857fa070}},
      {"31,0,31,31,31,31_0",
       {0x44e87f35, 0x4dd86a64, 0x54676db9, 0xe7cf1d3c,
        0xf7400fb0, 0xfe701ae1}},
      {"31,0,31,31,31,0_0",
       {0xddf01ac6, 0xd2b74a56, 0xc44f1d1b, 0x7ed778cf,
        0x200da840}},
  };
  std::unordered_map<std::string, std::unordered_set<uint>> precalculated_bdsp_ivs_to_seed{
      {"31,31,31,31,31,31_3",
       {0x60d489e, 0xf962809, 0x1707a1bc, 0x256512ac,
        0x2b2e0bc5, 0x3e9e0489, 0x429f7bdd, 0x4fcd2b6c,
        0x5ce9aa13, 0x65aded09, 0x6707f338, 0x6b72fe4d,
        0x7171a30b, 0x733dd448, 0x7a928de5, 0x7ef34d82,
        0x874a8cd4, 0x90383581, 0x9bd65fdb, 0x9f2f499e,
        0xa01f78d8, 0xafecaffc, 0xb2b8db34, 0xc057809e,
        0xc21a8fb4, 0xcab3bc4d, 0xd970b3b2, 0xe6da6290,
        0xebf047c3, 0xf489a30c, 0xf6cf4c3b, 0xff2be493}},
      {"31,0,31,31,31,0_3",
       {0x21bbd6a, 0x630fbe9f, 0x6a84b6ed, 0x6c6d3a44,
        0x745e444f, 0xc9080389, 0xc937601d, 0xce11bee0}},
      {"31,0,31,31,31,31_3",
       {0xbd5a6b6, 0x1434d7a3, 0x14a4e19d, 0x16356604,
        0x258ffeb3, 0x2811e041, 0x2fbfee90, 0x4318207a,
        0x57003093, 0x6ea9022c, 0x7d5b6f50, 0x86e81468,
        0x8b2bc094, 0x91c150eb, 0x93c2fef8, 0x96e4f3e9,
        0xa3fde2ad, 0xdc52ec02, 0xdd32dc34, 0xfaea4706}},
      {"31,31,31,31,31,0_3",
       {0x6f865f2, 0x1eeb4ab9, 0x22c848cd, 0x2fbdd191,
        0x3d0b17be, 0x51097301, 0x558e91a4, 0x5f051ba8,
        0x61afeb5f, 0x6be08d41, 0x6e87f93c, 0x77252c8c,
        0x86138402, 0x9aeeee9b, 0xac21a413, 0xca9ded5d,
        0xd013ae00, 0xd6fc7ddd}},
  };

  std::string query_key = g_args.ivs + "_" + std::to_string(g_args.flawless_count);
  std::unordered_set<uint> seed_white_list;
  switch (g_args.mode) {
    case RNDType::kSWSHOverworld: {
      // 找不到加一个就加一个了
      seed_white_list = precalculated_bdsp_ivs_to_seed[query_key];
      break;
    }
    case RNDType::kBDSPRoaming: {
      // 找不到加一个就加一个了
      seed_white_list = precalculated_bdsp_ivs_to_seed[query_key];
      break;
    }
  }

  FindPokemon(g_args.mode, g_args.sid, g_args.tid, ivs[0], ivs[1], ivs[2], ivs[3], ivs[4], ivs[5], g_args.shiny_type_you_want, g_args.flawless_count, seed_white_list);
  return 0;
}

#ifndef __OPUTILS_ARG_H__
#define __OPUTILS_ARG_H__

#include <functional>
#include <string>

namespace OpUtils {

typedef enum { arg_none = 0,
               arg_required = 1,
               arg_optional = 2 } arg_status;

enum class must_have { must_y,
                       must_n };

/** \brief          parse one arg
 * \param first     arg value in c-style string
 * \param second    a handle casted to void*(you should put your parsed result
 * in this)
 * \param third     error message if parse faild,will print out
 * \return          true:ok false:ng
 */
typedef std::function<bool(char *, void *, std::string &strErr)> FnArgParse;

struct sArgDef {
  char name_short;
  const char *name_long;
  must_have must;
  arg_status has_arg;
  const char *msg_help;
  FnArgParse fn_parse;
  void *data;
};

bool parse_cmd_args(int argc, char *argv[], sArgDef *pDef, int size);
}// namespace OpUtils

#endif//__OPUTILS_ARG_H__

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 268 "xbt/xbt_str.cpp" 
#include <xbt/ex.hpp>
#include "xbt/str.h"


#define mytest(name, input, expected)                                                                                  \
  xbt_test_add(name);                                                                                                  \
  a = static_cast<char**>(xbt_dynar_to_array(xbt_str_split_quoted(input)));                                            \
  s = xbt_str_join_array(a, "XXX");                                                                                    \
  xbt_test_assert(not strcmp(s, expected), "Input (%s) leads to (%s) instead of (%s)", input, s, expected);            \
  xbt_free(s);                                                                                                         \
  for (int i = 0; a[i] != nullptr; i++)                                                                                \
    xbt_free(a[i]);                                                                                                    \
  xbt_free(a);
XBT_TEST_UNIT("xbt_str_split_quoted", test_split_quoted, "Test the function xbt_str_split_quoted")
{
  char** a;
  char *s;

  mytest("Empty", "", "");
  mytest("Basic test", "toto tutu", "totoXXXtutu");
  mytest("Useless backslashes", "\\t\\o\\t\\o \\t\\u\\t\\u", "totoXXXtutu");
  mytest("Protected space", "toto\\ tutu", "toto tutu");
  mytest("Several spaces", "toto   tutu", "totoXXXtutu");
  mytest("LTriming", "  toto tatu", "totoXXXtatu");
  mytest("Triming", "  toto   tutu  ", "totoXXXtutu");
  mytest("Single quotes", "'toto tutu' tata", "toto tutuXXXtata");
  mytest("Double quotes", "\"toto tutu\" tata", "toto tutuXXXtata");
  mytest("Mixed quotes", "\"toto' 'tutu\" tata", "toto' 'tutuXXXtata");
  mytest("Backslashed quotes", "\\'toto tutu\\' tata", "'totoXXXtutu'XXXtata");
  mytest("Backslashed quotes + quotes", "'toto \\'tutu' tata", "toto 'tutuXXXtata");
}

#define test_parse_error(function, name, variable, str)                 \
  do {                                                                  \
    xbt_test_add(name);                                                 \
    try {                                                               \
      variable = function(str, "Parse error");                          \
      xbt_test_fail("The test '%s' did not detect the problem",name );  \
    } catch(xbt_ex& e) {                                                \
      if (e.category != arg_error) {                                    \
        xbt_test_exception(e);                                          \
      }                                                                 \
    }                                                                   \
  } while (0)
#define test_parse_ok(function, name, variable, str, value)             \
  do {                                                                  \
    xbt_test_add(name);                                                 \
    try {                                                               \
      variable = function(str, "Parse error");                          \
    } catch(xbt_ex& e) {                                                \
      xbt_test_exception(e);                                            \
    }                                                                   \
    xbt_test_assert(variable == value, "Fail to parse '%s'", str);      \
  } while (0)

XBT_TEST_UNIT("xbt_str_parse", test_parse, "Test the parsing functions")
{
  int rint = -9999;
  test_parse_ok(xbt_str_parse_int, "Parse int", rint, "42", 42);
  test_parse_ok(xbt_str_parse_int, "Parse 0 as an int", rint, "0", 0);
  test_parse_ok(xbt_str_parse_int, "Parse -1 as an int", rint, "-1", -1);

  test_parse_error(xbt_str_parse_int, "Parse int + noise", rint, "342 cruft");
  test_parse_error(xbt_str_parse_int, "Parse nullptr as an int", rint, nullptr);
  test_parse_error(xbt_str_parse_int, "Parse '' as an int", rint, "");
  test_parse_error(xbt_str_parse_int, "Parse cruft as an int", rint, "cruft");

  double rdouble = -9999;
  test_parse_ok(xbt_str_parse_double, "Parse 42 as a double", rdouble, "42", 42);
  test_parse_ok(xbt_str_parse_double, "Parse 42.5 as a double", rdouble, "42.5", 42.5);
  test_parse_ok(xbt_str_parse_double, "Parse 0 as a double", rdouble, "0", 0);
  test_parse_ok(xbt_str_parse_double, "Parse -1 as a double", rdouble, "-1", -1);

  test_parse_error(xbt_str_parse_double, "Parse double + noise", rdouble, "342 cruft");
  test_parse_error(xbt_str_parse_double, "Parse nullptr as a double", rdouble, nullptr);
  test_parse_error(xbt_str_parse_double, "Parse '' as a double", rdouble, "");
  test_parse_error(xbt_str_parse_double, "Parse cruft as a double", rdouble, "cruft");
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/


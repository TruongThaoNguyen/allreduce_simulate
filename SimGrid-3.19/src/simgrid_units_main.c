/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>

#include "xbt.h"

extern xbt_test_unit_t _xbt_current_unit;

#define STRLEN 1024
/* SGU: BEGIN PROTOTYPES */
  /* SGU: BEGIN FILE xbt/cunit.cpp */
    void  test_expected_failure(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/ex.cpp */
    void  test_controlflow(void);
    void  test_value(void);
    void  test_variables(void);
    void  test_cleanup(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/dynar.cpp */
    void  test_dynar_int(void);
    void test_dynar_insert(void);
    void  test_dynar_double(void);
    void  test_dynar_string(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/dict.cpp */
    void  test_dict_basic(void);
    void  test_dict_remove(void);
    void  test_dict_nulldata(void);
    void  test_dict_crash(void);
    void  test_dict_int(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/xbt_str.cpp */
    void  test_split_quoted(void);
    void  test_parse(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/config.cpp */
    void  test_config_memuse(void);
    void  test_config_use(void);
    void  test_config_cxx_flags(void);
  /* SGU: END FILE */

/* SGU: END PROTOTYPES */

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

int main(int argc, char *argv[]) {
  xbt_test_suite_t suite; 
  char selection[STRLEN];
  int verbosity = 0;
  int i;
  int res;

  /* SGU: BEGIN SUITES DECLARATION */
    /* SGU: BEGIN FILE xbt/cunit.cpp */
      suite = xbt_test_suite_by_name("cunit","Testsuite mechanism autotest");
      xbt_test_suite_push(suite, "expect",  test_expected_failure,  "expected failures");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/ex.cpp */
      suite = xbt_test_suite_by_name("xbt_ex","Exception Handling");
      xbt_test_suite_push(suite, "controlflow",  test_controlflow,  "basic nested control flow");
      xbt_test_suite_push(suite, "value",  test_value,  "exception value passing");
      xbt_test_suite_push(suite, "variables",  test_variables,  "variable value preservation");
      xbt_test_suite_push(suite, "cleanup",  test_cleanup,  "cleanup handling");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/dynar.cpp */
      suite = xbt_test_suite_by_name("dynar","Dynar data container");
      xbt_test_suite_push(suite, "int",  test_dynar_int,  "Dynars of integers");
      xbt_test_suite_push(suite, "insert", test_dynar_insert, "Using the xbt_dynar_insert and xbt_dynar_remove functions");
      xbt_test_suite_push(suite, "double",  test_dynar_double,  "Dynars of doubles");
      xbt_test_suite_push(suite, "string",  test_dynar_string,  "Dynars of strings");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/dict.cpp */
      suite = xbt_test_suite_by_name("dict","Dict data container");
      xbt_test_suite_push(suite, "basic",  test_dict_basic,  "Basic usage: change, retrieve and traverse homogeneous dicts");
      xbt_test_suite_push(suite, "remove_homogeneous",  test_dict_remove,  "Removing some values from homogeneous dicts");
      xbt_test_suite_push(suite, "nulldata",  test_dict_nulldata,  "nullptr data management");
      xbt_test_suite_push(suite, "crash",  test_dict_crash,  "Crash test");
      xbt_test_suite_push(suite, "ext",  test_dict_int,  "Test dictionnary with int keys");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/xbt_str.cpp */
      suite = xbt_test_suite_by_name("xbt_str","String Handling");
      xbt_test_suite_push(suite, "xbt_str_split_quoted",  test_split_quoted,  "Test the function xbt_str_split_quoted");
      xbt_test_suite_push(suite, "xbt_str_parse",  test_parse,  "Test the parsing functions");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/config.cpp */
      suite = xbt_test_suite_by_name("config","Configuration support");
      xbt_test_suite_push(suite, "memuse",  test_config_memuse,  "Alloc and free a config set");
      xbt_test_suite_push(suite, "use",  test_config_use,  "Data retrieving tests");
      xbt_test_suite_push(suite, "c++flags",  test_config_cxx_flags,  "C++ flags");
    /* SGU: END FILE */

  /* SGU: END SUITES DECLARATION */
      
  xbt_init(&argc,argv);
    
  /* Search for the tests to do */
    selection[0]='\0';
    for (i=1;i<argc;i++) {
      if (!strncmp(argv[i],"--tests=",strlen("--tests="))) {
        char *p=strchr(argv[i],'=')+1;
        if (selection[0] != '\0')
          strncat(selection, ",", STRLEN - 1 - strlen(selection));
        strncat(selection, p, STRLEN - 1 - strlen(selection));
      } else if (!strcmp(argv[i], "--verbose")) {
        verbosity++;
      } else if (!strcmp(argv[i], "--dump-only")||
                 !strcmp(argv[i], "--dump")) {
        xbt_test_dump(selection);
        return 0;
      } else if (!strcmp(argv[i], "--help")) {
	  printf(
	      "Usage: testall [--help] [--tests=selection] [--dump-only]\n\n"
	      "--help: display this help\n"
	      "--verbose: print the name for each running test\n"
	      "--dump-only: don't run the tests, but display some debuging info about the tests\n"
	      "--tests=selection: Use argument to select which suites/units/tests to run\n"
	      "                   --tests can be used more than once, and selection may be a comma\n"
	      "                   separated list of directives.\n\n"
	      "Directives are of the form:\n"
	      "   [-]suitename[:unitname]\n\n"
	      "If the first char is a '-', the directive disables its argument instead of enabling it\n"
	      "suitename/unitname is the set of tests to en/disable. If a unitname is not specified,\n"
	      "it applies on any unit.\n\n"
	      "By default, everything is enabled.\n\n"
	      "'all' as suite name apply to all suites.\n\n"
	      "Example 1: \"-toto,+toto:tutu\"\n"
	      "  disables the whole toto testsuite (any unit in it),\n"
	      "  then reenables the tutu unit of the toto test suite.\n\n"
	      "Example 2: \"-all,+toto\"\n"
	      "  Run nothing but the toto suite.\n");
	  return 0;
      } else {
        printf("testall: Unknown option: %s\n",argv[i]);
        return 1;
      }
    }
  /* Got all my tests to do */
      
  res = xbt_test_run(selection, verbosity);
  xbt_test_exit();
  return res;
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/


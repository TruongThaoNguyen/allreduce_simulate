/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 86 "xbt/ex.cpp" 
#include "xbt/ex.h"
#include <cstdio>
#include <xbt/ex.hpp>


XBT_TEST_UNIT("controlflow", test_controlflow, "basic nested control flow")
{
  xbt_ex_t ex;
  int n = 1;

  xbt_test_add("basic nested control flow");

  try {
    if (n != 1)
      xbt_test_fail("M1: n=%d (!= 1)", n);
    n++;
    try {
      if (n != 2)
        xbt_test_fail("M2: n=%d (!= 2)", n);
      n++;
      THROWF(unknown_error, 0, "something");
    }
    catch (xbt_ex& ex) {
      if (n != 3)
        xbt_test_fail("M3: n=%d (!= 3)", n);
      n++;
    }
    n++;
    try {
      if (n != 5)
        xbt_test_fail("M2: n=%d (!= 5)", n);
      n++;
      THROWF(unknown_error, 0, "something");
    }
    catch(xbt_ex& ex){
      if (n != 6)
        xbt_test_fail("M3: n=%d (!= 6)", n);
      n++;
      throw;
      n++;
    }
    xbt_test_fail("MX: n=%d (shouldn't reach this point)", n);
  }
  catch(xbt_ex& e) {
    if (n != 7)
      xbt_test_fail("M4: n=%d (!= 7)", n);
    n++;
  }
  if (n != 8)
    xbt_test_fail("M5: n=%d (!= 8)", n);
}

XBT_TEST_UNIT("value", test_value, "exception value passing")
{
  try {
    THROWF(unknown_error, 2, "toto");
  }
  catch (xbt_ex& ex) {
    xbt_test_add("exception value passing");
    if (ex.category != unknown_error)
      xbt_test_fail("category=%d (!= 1)", (int)ex.category);
    if (ex.value != 2)
      xbt_test_fail("value=%d (!= 2)", ex.value);
    if (strcmp(ex.what(), "toto"))
      xbt_test_fail("message=%s (!= toto)", ex.what());
  }
}

XBT_TEST_UNIT("variables", test_variables, "variable value preservation")
{
  xbt_ex_t ex;
  int r1;
  XBT_ATTRIB_UNUSED int r2;
  int v1;
  int v2;

  r1 = r2 = v1 = v2 = 1234;
  try {
    r2 = 5678;
    v2 = 5678;
    THROWF(unknown_error, 0, "toto");
  }
  catch(xbt_ex& e) {
    xbt_test_add("variable preservation");
    if (r1 != 1234)
      xbt_test_fail("r1=%d (!= 1234)", r1);
    if (v1 != 1234)
      xbt_test_fail("v1=%d (!= 1234)", v1);
    /* r2 is allowed to be destroyed because not volatile */
    if (v2 != 5678)
      xbt_test_fail("v2=%d (!= 5678)", v2);
  }
}

XBT_TEST_UNIT("cleanup", test_cleanup, "cleanup handling")
{
  int v1;
  int c;

  xbt_test_add("cleanup handling");

  v1 = 1234;
  c = 0;
  try {
    v1 = 5678;
    THROWF(1, 2, "blah");
  }
  catch (xbt_ex& ex) {
    if (v1 != 5678)
      xbt_test_fail("v1 = %d (!= 5678)", v1);
    c = 1;
    if (v1 != 5678)
      xbt_test_fail("v1 = %d (!= 5678)", v1);
    if (not(ex.category == 1 && ex.value == 2 && not strcmp(ex.what(), "blah")))
      xbt_test_fail("unexpected exception contents");
  }
  if (not c)
    xbt_test_fail("xbt_ex_free not executed");
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/


/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 546 "xbt/dict.cpp" 
#include "src/internal_config.h"
#include "xbt.h"
#include "xbt/ex.h"
#include <ctime>
#include <xbt/ex.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_dict);


static void debugged_add_ext(xbt_dict_t head, const char* key, const char* data_to_fill)
{
  char *data = xbt_strdup(data_to_fill);

  xbt_test_log("Add %s under %s", data_to_fill, key);

  xbt_dict_set(head, key, data, nullptr);
  if (XBT_LOG_ISENABLED(xbt_dict, xbt_log_priority_debug)) {
    xbt_dict_dump(head, (void (*)(void *)) &printf);
    fflush(stdout);
  }
}

static void debugged_add(xbt_dict_t head, const char* key)
{
  debugged_add_ext(head, key, key);
}

static xbt_dict_t new_fixture()
{
  xbt_test_add("Fill in the dictionnary");

  xbt_dict_t head = xbt_dict_new_homogeneous(&free);
  debugged_add(head, "12");
  debugged_add(head, "12a");
  debugged_add(head, "12b");
  debugged_add(head, "123");
  debugged_add(head, "123456");
  debugged_add(head, "1234");
  debugged_add(head, "123457");

  return head;
}

static void search_ext(xbt_dict_t head, const char *key, const char *data)
{
  xbt_test_add("Search %s", key);
  char *found = (char*) xbt_dict_get(head, key);
  xbt_test_log("Found %s", found);
  if (data) {
    xbt_test_assert(found, "data do not match expectations: found nullptr while searching for %s", data);
    if (found)
      xbt_test_assert(not strcmp(data, found), "data do not match expectations: found %s while searching for %s", found,
                      data);
  } else {
    xbt_test_assert(not found, "data do not match expectations: found %s while searching for nullptr", found);
  }
}

static void search(xbt_dict_t head, const char *key)
{
  search_ext(head, key, key);
}

static void debugged_remove(xbt_dict_t head, const char* key)
{
  xbt_test_add("Remove '%s'", key);
  xbt_dict_remove(head, key);
}

static void traverse(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor = nullptr;
  char *key;
  char *data;
  int i = 0;

  xbt_dict_foreach(head, cursor, key, data) {
    if (not key || not data || strcmp(key, data)) {
      xbt_test_log("Seen #%d:  %s->%s", ++i, key, data);
    } else {
      xbt_test_log("Seen #%d:  %s", ++i, key);
    }
    xbt_test_assert(key && data && strcmp(key, data) == 0, "Key(%s) != value(%s). Aborting", key, data);
  }
}

static void search_not_found(xbt_dict_t head, const char *data)
{
  int ok = 0;
  xbt_test_add("Search %s (expected not to be found)", data);

  try {
    data = (const char*) xbt_dict_get(head, data);
    THROWF(unknown_error, 0, "Found something which shouldn't be there (%s)", data);
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    ok = 1;
  }
  xbt_test_assert(ok, "Exception not raised");
}

static void count(xbt_dict_t dict, int length)
{
  xbt_test_add("Count elements (expecting %d)", length);
  xbt_test_assert(xbt_dict_length(dict) == length, "Announced length(%d) != %d.", xbt_dict_length(dict), length);

  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int effective = 0;
  xbt_dict_foreach(dict, cursor, key, data)
      effective++;

  xbt_test_assert(effective == length, "Effective length(%d) != %d.", effective, length);
}

static void count_check_get_key(xbt_dict_t dict, int length)
{
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int effective = 0;

  xbt_test_add("Count elements (expecting %d), and test the getkey function", length);
  xbt_test_assert(xbt_dict_length(dict) == length, "Announced length(%d) != %d.", xbt_dict_length(dict), length);

  xbt_dict_foreach(dict, cursor, key, data) {
    effective++;
    char* key2 = xbt_dict_get_key(dict, data);
    xbt_assert(not strcmp(key, key2), "The data was registered under %s instead of %s as expected", key2, key);
  }

  xbt_test_assert(effective == length, "Effective length(%d) != %d.", effective, length);
}

XBT_TEST_UNIT("basic", test_dict_basic, "Basic usage: change, retrieve and traverse homogeneous dicts")
{
  xbt_test_add("Traversal the null dictionary");
  traverse(nullptr);

  xbt_test_add("Traversal and search the empty dictionary");
  xbt_dict_t head = xbt_dict_new_homogeneous(&free);
  traverse(head);
  try {
    debugged_remove(head, "12346");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
  }
  xbt_dict_free(&head);

  xbt_test_add("Traverse the full dictionary");
  head = new_fixture();
  count_check_get_key(head, 7);

  debugged_add_ext(head, "toto", "tutu");
  search_ext(head, "toto", "tutu");
  debugged_remove(head, "toto");

  search(head, "12a");
  traverse(head);

  xbt_test_add("Free the dictionary (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  /* CHANGING */
  head = new_fixture();
  count_check_get_key(head, 7);
  xbt_test_add("Change 123 to 'Changed 123'");
  xbt_dict_set(head, "123", xbt_strdup("Changed 123"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Change 123 back to '123'");
  xbt_dict_set(head, "123", xbt_strdup("123"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Change 12a to 'Dummy 12a'");
  xbt_dict_set(head, "12a", xbt_strdup("Dummy 12a"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Change 12a to '12a'");
  xbt_dict_set(head, "12a", xbt_strdup("12a"), nullptr);
  count_check_get_key(head, 7);

  xbt_test_add("Traverse the resulting dictionary");
  traverse(head);

  /* RETRIEVE */
  xbt_test_add("Search 123");
  char* data = (char*)xbt_dict_get(head, "123");
  xbt_test_assert(data && strcmp("123", data) == 0);

  search_not_found(head, "Can't be found");
  search_not_found(head, "123 Can't be found");
  search_not_found(head, "12345678 NOT");

  search(head, "12a");
  search(head, "12b");
  search(head, "12");
  search(head, "123456");
  search(head, "1234");
  search(head, "123457");

  xbt_test_add("Traverse the resulting dictionary");
  traverse(head);

  xbt_test_add("Free the dictionary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);

  xbt_test_add("Traverse the resulting dictionary");
  traverse(head);
}

XBT_TEST_UNIT("remove_homogeneous", test_dict_remove, "Removing some values from homogeneous dicts")
{
  xbt_dict_t head = new_fixture();
  count(head, 7);
  xbt_test_add("Remove non existing data");
  try {
    debugged_remove(head, "Does not exist");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
  }
  traverse(head);

  xbt_dict_free(&head);

  xbt_test_add("Remove each data manually (traversing the resulting dictionary each time)");
  head = new_fixture();
  debugged_remove(head, "12a");
  traverse(head);
  count(head, 6);
  debugged_remove(head, "12b");
  traverse(head);
  count(head, 5);
  debugged_remove(head, "12");
  traverse(head);
  count(head, 4);
  debugged_remove(head, "123456");
  traverse(head);
  count(head, 3);
  try {
    debugged_remove(head, "12346");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
    traverse(head);
  }
  debugged_remove(head, "1234");
  traverse(head);
  debugged_remove(head, "123457");
  traverse(head);
  debugged_remove(head, "123");
  traverse(head);
  try {
    debugged_remove(head, "12346");
  }
  catch(xbt_ex& e) {
    if (e.category != not_found_error)
      xbt_test_exception(e);
  }
  traverse(head);

  xbt_test_add("Free dict, create new fresh one, and then reset the dict");
  xbt_dict_free(&head);
  head = new_fixture();
  xbt_dict_reset(head);
  count(head, 0);
  traverse(head);

  xbt_test_add("Free the dictionary twice");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

XBT_TEST_UNIT("nulldata", test_dict_nulldata, "nullptr data management")
{
  xbt_dict_t head = new_fixture();

  xbt_test_add("Store nullptr under 'null'");
  xbt_dict_set(head, "null", nullptr, nullptr);
  search_ext(head, "null", nullptr);

  xbt_test_add("Check whether I see it while traversing...");
  {
    xbt_dict_cursor_t cursor = nullptr;
    char *key;
    int found = 0;
    char* data;

    xbt_dict_foreach(head, cursor, key, data) {
      if (not key || not data || strcmp(key, data)) {
        xbt_test_log("Seen:  %s->%s", key, data);
      } else {
        xbt_test_log("Seen:  %s", key);
      }

      if (key && strcmp(key, "null") == 0)
        found = 1;
    }
    xbt_test_assert(found, "the key 'null', associated to nullptr is not found");
  }
  xbt_dict_free(&head);
}

#define NB_ELM 20000
#define SIZEOFKEY 1024
static int countelems(xbt_dict_t head)
{
  xbt_dict_cursor_t cursor;
  char *key;
  void *data;
  int res = 0;

  xbt_dict_foreach(head, cursor, key, data) {
    res++;
  }
  return res;
}

XBT_TEST_UNIT("crash", test_dict_crash, "Crash test")
{
  srand((unsigned int) time(nullptr));

  for (int i = 0; i < 10; i++) {
    xbt_test_add("CRASH test number %d (%d to go)", i + 1, 10 - i - 1);
    xbt_test_log("Fill the struct, count its elems and frees the structure");
    xbt_test_log("using 1000 elements with %d chars long randomized keys.", SIZEOFKEY);
    xbt_dict_t head = xbt_dict_new_homogeneous(free);
    for (int j = 0; j < 1000; j++) {
      char* data = nullptr;
      char* key  = (char*)xbt_malloc(SIZEOFKEY);

      do {
        for (int k         = 0; k < SIZEOFKEY - 1; k++)
          key[k] = rand() % ('z' - 'a') + 'a';
        key[SIZEOFKEY - 1] = '\0';
        data = (char*) xbt_dict_get_or_null(head, key);
      } while (data != nullptr);

      xbt_dict_set(head, key, key, nullptr);
      data = (char*) xbt_dict_get(head, key);
      xbt_test_assert(not strcmp(key, data), "Retrieved value (%s) != Injected value (%s)", key, data);

      count(head, j + 1);
    }
    traverse(head);
    xbt_dict_free(&head);
    xbt_dict_free(&head);
  }

  xbt_dict_t head = xbt_dict_new_homogeneous(&free);
  xbt_test_add("Fill %d elements, with keys being the number of element", NB_ELM);
  for (int j = 0; j < NB_ELM; j++) {
    char* key = (char*)xbt_malloc(10);

    snprintf(key,10, "%d", j);
    xbt_dict_set(head, key, key, nullptr);
  }

  xbt_test_add("Count the elements (retrieving the key and data for each)");
  xbt_test_log("There is %d elements", countelems(head));

  xbt_test_add("Search my %d elements 20 times", NB_ELM);
  char* key = (char*)xbt_malloc(10);
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < NB_ELM; j++) {
      snprintf(key,10, "%d", j);
      void* data = xbt_dict_get(head, key);
      xbt_test_assert(not strcmp(key, (char*)data), "with get, key=%s != data=%s", key, (char*)data);
      data = xbt_dict_get_ext(head, key, strlen(key));
      xbt_test_assert(not strcmp(key, (char*)data), "with get_ext, key=%s != data=%s", key, (char*)data);
    }
  }
  free(key);

  xbt_test_add("Remove my %d elements", NB_ELM);
  key = (char*) xbt_malloc(10);
  for (int j = 0; j < NB_ELM; j++) {
    snprintf(key,10, "%d", j);
    xbt_dict_remove(head, key);
  }
  free(key);

  xbt_test_add("Free the object (twice)");
  xbt_dict_free(&head);
  xbt_dict_free(&head);
}

XBT_TEST_UNIT("ext", test_dict_int, "Test dictionnary with int keys")
{
  xbt_dict_t dict = xbt_dict_new_homogeneous(nullptr);
  int count = 500;

  xbt_test_add("Insert elements");
  for (int i = 0; i < count; ++i)
    xbt_dict_set_ext(dict, (char*) &i, sizeof(i), (void*) (intptr_t) i, nullptr);
  xbt_test_assert(xbt_dict_size(dict) == (unsigned) count, "Bad number of elements in the dictionnary");

  xbt_test_add("Check elements");
  for (int i = 0; i < count; ++i) {
    int res = (int) (intptr_t) xbt_dict_get_ext(dict, (char*) &i, sizeof(i));
    xbt_test_assert(xbt_dict_size(dict) == (unsigned) count, "Unexpected value at index %i, expected %i but was %i", i, i, res);
  }

  xbt_test_add("Free the array");
  xbt_dict_free(&dict);
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/


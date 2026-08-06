/* Copyright (c) 2026 lefred (Frédéric Descamps) */

#include "../bel_pay_ref_validation.h"

#include <cassert>
#include <cstring>
#include <string>

static bool valid(const char *value)
{
  return bel_pay_ref::validate(value, std::strlen(value));
}

int main()
{
  assert(valid("+++123/4567/89002+++"));
  assert(valid("123456789002"));
  assert(valid("+++000/0000/00097+++"));
  assert(valid("+++000/0000/09797+++"));

  const char *invalid[]=
  {
    "", "123456789003", "+++123/4567/89003+++",
    "+++123/456/789002+++", "+++123-4567-89002+++",
    "+++12A/4567/89002+++", " +++123/4567/89002+++",
    "+++123/4567/89002+++ "
  };
  for (const char *value : invalid)
    assert(!valid(value));

  unsigned int check_digits= 0;
  assert(bel_pay_ref::calculate_check_digits("1234567890", 10, &check_digits));
  assert(check_digits == 2);
  assert(bel_pay_ref::calculate_check_digits("0000000000", 10, &check_digits));
  assert(check_digits == 97);
  assert(!bel_pay_ref::calculate_check_digits("123456789", 9, &check_digits));
  assert(!bel_pay_ref::calculate_check_digits("123456789x", 10, &check_digits));

  std::string result;
  assert(bel_pay_ref::generate("1234567890", 10, &result));
  assert(result == "+++123/4567/89002+++");
  assert(bel_pay_ref::generate("0000000000", 10, &result));
  assert(result == "+++000/0000/00097+++");
  assert(bel_pay_ref::format("123456789002", 12, &result));
  assert(result == "+++123/4567/89002+++");
  assert(!bel_pay_ref::format("123456789003", 12, &result));
  assert(bel_pay_ref::compact("+++123/4567/89002+++", 20, &result));
  assert(result == "123456789002");
  assert(bel_pay_ref::compact("123456789002", 12, &result));
  assert(result == "123456789002");
}

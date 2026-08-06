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

  assert(bel_pay_ref::generate_parts("123", 3, "1", 1, &result));
  assert(result == "+++123/0000/00137+++");
  assert(bel_pay_ref::generate_parts("42", 2, nullptr, 0, &result));
  assert(result == "+++000/0000/04242+++");
  assert(!bel_pay_ref::generate_parts("123456", 6, "12345", 5, &result));
  assert(!bel_pay_ref::generate_parts("12x", 3, "1", 1, &result));

  bel_pay_ref::Validation_detail detail=
    bel_pay_ref::validate_detail("+++123/4567/89003+++", 20);
  assert(!detail.valid());
  assert(detail.reason == bel_pay_ref::CHECK_DIGIT_MISMATCH);
  assert(detail.base == "1234567890");
  assert(detail.expected == "02");
  assert(detail.received == "03");

  detail= bel_pay_ref::validate_detail("123456789002", 12);
  assert(detail.valid());
  assert(detail.reason == bel_pay_ref::VALID);
  assert(detail.expected == "02");
  assert(detail.received == "02");

  detail= bel_pay_ref::validate_detail("not-a-reference", 15);
  assert(detail.reason == bel_pay_ref::INVALID_LENGTH);
  assert(detail.base.empty());

  detail= bel_pay_ref::validate_detail("12345678900x", 12);
  assert(detail.reason == bel_pay_ref::INVALID_CHARACTERS);

  detail= bel_pay_ref::validate_detail("+++123-4567/89002+++", 20);
  assert(detail.reason == bel_pay_ref::INVALID_SEPARATORS);

  detail= bel_pay_ref::validate_detail("+++123/456x/89002+++", 20);
  assert(detail.reason == bel_pay_ref::INVALID_CHARACTERS);
}

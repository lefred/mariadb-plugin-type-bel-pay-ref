/* Copyright (c) 2019,2024,2025,2026 MariaDB Corporation
   Copyright (c) 2026 lefred (Frédéric Descamps)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

#include "bel_pay_ref_validation.h"

#include <cctype>

namespace bel_pay_ref
{
namespace
{

bool all_digits(const char *value, size_t length)
{
  if (!value)
    return false;
  for (size_t i= 0; i < length; ++i)
  {
    if (!std::isdigit(static_cast<unsigned char>(value[i])))
      return false;
  }
  return true;
}

void format_digits(const std::string &digits, std::string *result)
{
  result->clear();
  result->reserve(FORMATTED_LENGTH);
  result->append("+++");
  result->append(digits, 0, 3);
  result->append("/");
  result->append(digits, 3, 4);
  result->append("/");
  result->append(digits, 7, 5);
  result->append("+++");
}

bool digits_are_valid(const std::string &digits)
{
  unsigned int expected;
  if (digits.length() != DIGITS_LENGTH ||
      !calculate_check_digits(digits.data(), BASE_LENGTH, &expected))
    return false;
  const unsigned int actual=
    static_cast<unsigned int>((digits[10] - '0') * 10 + digits[11] - '0');
  return actual == expected;
}

} // namespace

bool extract_digits(const char *value, size_t length, std::string *digits)
{
  if (!value || !digits)
    return false;

  if (length == DIGITS_LENGTH && all_digits(value, length))
  {
    digits->assign(value, length);
    return true;
  }

  if (length != FORMATTED_LENGTH ||
      value[0] != '+' || value[1] != '+' || value[2] != '+' ||
      value[6] != '/' || value[11] != '/' ||
      value[17] != '+' || value[18] != '+' || value[19] != '+')
    return false;

  digits->clear();
  digits->reserve(DIGITS_LENGTH);
  for (size_t i= 3; i < 17; ++i)
  {
    if (i == 6 || i == 11)
      continue;
    if (!std::isdigit(static_cast<unsigned char>(value[i])))
      return false;
    digits->push_back(value[i]);
  }
  return digits->length() == DIGITS_LENGTH;
}

bool calculate_check_digits(const char *base, size_t length,
                            unsigned int *check_digits)
{
  if (!check_digits || length != BASE_LENGTH || !all_digits(base, length))
    return false;

  unsigned int remainder= 0;
  for (size_t i= 0; i < length; ++i)
    remainder= (remainder * 10 + static_cast<unsigned int>(base[i] - '0')) % 97;
  *check_digits= remainder == 0 ? 97 : remainder;
  return true;
}

bool validate(const char *value, size_t length)
{
  std::string digits;
  return extract_digits(value, length, &digits) && digits_are_valid(digits);
}

bool format(const char *value, size_t length, std::string *result)
{
  std::string digits;
  if (!result || !extract_digits(value, length, &digits) ||
      !digits_are_valid(digits))
    return false;
  format_digits(digits, result);
  return true;
}

bool compact(const char *value, size_t length, std::string *result)
{
  std::string digits;
  if (!result || !extract_digits(value, length, &digits) ||
      !digits_are_valid(digits))
    return false;
  *result= std::move(digits);
  return true;
}

bool generate(const char *base, size_t length, std::string *result)
{
  unsigned int check_digits;
  if (!result || !calculate_check_digits(base, length, &check_digits))
    return false;

  std::string digits(base, length);
  digits.push_back(static_cast<char>('0' + check_digits / 10));
  digits.push_back(static_cast<char>('0' + check_digits % 10));
  format_digits(digits, result);
  return true;
}

bool generate_parts(const char *first, size_t first_length,
                    const char *second, size_t second_length,
                    std::string *result)
{
  if (!result || !first_length || first_length > BASE_LENGTH ||
      !all_digits(first, first_length))
    return false;

  std::string base;
  if (!second)
  {
    base.assign(BASE_LENGTH - first_length, '0');
    base.append(first, first_length);
  }
  else
  {
    if (!second_length || first_length + second_length > BASE_LENGTH ||
        !all_digits(second, second_length))
      return false;
    base.assign(first, first_length);
    base.append(BASE_LENGTH - first_length - second_length, '0');
    base.append(second, second_length);
  }
  return generate(base.data(), base.length(), result);
}

} // namespace bel_pay_ref

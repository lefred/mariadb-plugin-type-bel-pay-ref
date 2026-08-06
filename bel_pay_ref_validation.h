#ifndef BEL_PAY_REF_VALIDATION_INCLUDED
#define BEL_PAY_REF_VALIDATION_INCLUDED
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

#include <stddef.h>
#include <string>

namespace bel_pay_ref
{

static const size_t BASE_LENGTH= 10;
static const size_t DIGITS_LENGTH= 12;
static const size_t FORMATTED_LENGTH= 20;

/* Validate either canonical +++XXX/YYYY/ZZZZZ+++ or compact notation. */
bool validate(const char *value, size_t length);

/* Extract 12 digits from either canonical or compact notation. */
bool extract_digits(const char *value, size_t length, std::string *digits);

/* Calculate the two check digits for an exactly 10-digit base. */
bool calculate_check_digits(const char *base, size_t length,
                            unsigned int *check_digits);

/* Produce the canonical representation from a valid 12-digit reference. */
bool format(const char *value, size_t length, std::string *result);

/* Produce the compact 12-digit representation from a valid reference. */
bool compact(const char *value, size_t length, std::string *result);

/* Produce a canonical reference and calculate its check digits. */
bool generate(const char *base, size_t length, std::string *result);

} // namespace bel_pay_ref

#endif

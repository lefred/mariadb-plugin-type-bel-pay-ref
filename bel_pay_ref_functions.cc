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

#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "item_func.h"
#include "bel_pay_ref_functions.h"
#include "bel_pay_ref_validation.h"

class Item_func_bel_pay_ref_is_valid : public Item_bool_func
{
public:
  Item_func_bel_pay_ref_is_valid(THD *thd, Item *arg): Item_bool_func(thd, arg) {}
  bool val_bool() override
  {
    StringBuffer<64> buffer;
    String *value= args[0]->val_str(&buffer);
    if (!value)
    {
      null_value= true;
      return false;
    }
    null_value= false;
    return bel_pay_ref::validate(value->ptr(), value->length());
  }
  LEX_CSTRING func_name_cstring() const override
  { return "bel_pay_ref_is_valid"_LEX_CSTRING; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_bel_pay_ref_is_valid>(thd, this); }
};

enum Bel_pay_ref_operation { BASE, CHECK_DIGITS, FORMAT, COMPACT, GENERATE };

class Item_func_bel_pay_ref_string : public Item_str_func
{
  Bel_pay_ref_operation operation;
  LEX_CSTRING function_name;
public:
  Item_func_bel_pay_ref_string(THD *thd, Item *arg,
                               Bel_pay_ref_operation operation_arg,
                               LEX_CSTRING name_arg)
    : Item_str_func(thd, arg), operation(operation_arg), function_name(name_arg)
  {}

  bool fix_length_and_dec(THD *thd) override
  {
    collation.set(args[0]->collation);
    max_length= operation == BASE ? bel_pay_ref::BASE_LENGTH :
                operation == CHECK_DIGITS ? 2 : bel_pay_ref::FORMATTED_LENGTH;
    set_maybe_null();

    /*
      The result is always plain ASCII digits (and +/ punctuation for
      FORMAT/GENERATE). A fixed-width multi-byte charset such as
      ucs2/utf16/utf32 cannot represent that as single-byte code units,
      so mislabeling the ASCII output with such a charset (via
      value->charset() below) would produce corrupt data. Reject it
      up front, consistent with CAST(... AS BEL_PAY_REF CHARACTER SET
      ucs2/utf16/utf32) in Item_bel_pay_ref_typecast::fix_length_and_dec.
    */
    if (args[0]->collation.collation->mbminlen > 1)
    {
      my_error(ER_NOT_SUPPORTED_YET, MYF(0),
               "BEL_PAY_REF functions with CHARACTER SET ucs2/utf16/utf32");
      return true;
    }
    return false;
  }

  String *val_str(String *result) override
  {
    StringBuffer<64> buffer;
    String *value= args[0]->val_str(&buffer);
    if (!value)
    {
      null_value= true;
      return nullptr;
    }

    std::string output;
    bool ok= false;
    if (operation == GENERATE)
      ok= bel_pay_ref::generate(value->ptr(), value->length(), &output);
    else if (operation == FORMAT)
      ok= bel_pay_ref::format(value->ptr(), value->length(), &output);
    else if (operation == COMPACT)
      ok= bel_pay_ref::compact(value->ptr(), value->length(), &output);
    else
    {
      /*
        BASE and CHECK_DIGITS both need the validated 12-digit form;
        compact() extracts and validates it in a single parse instead of
        parsing the input twice (extract_digits() + validate()).
      */
      std::string digits;
      ok= bel_pay_ref::compact(value->ptr(), value->length(), &digits);
      if (ok)
        output= operation == BASE ? digits.substr(0, bel_pay_ref::BASE_LENGTH) :
                                   digits.substr(bel_pay_ref::BASE_LENGTH, 2);
    }

    if (!ok || result->copy(output.data(), output.size(), value->charset()))
    {
      null_value= true;
      return nullptr;
    }
    null_value= false;
    return result;
  }

  LEX_CSTRING func_name_cstring() const override { return function_name; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_bel_pay_ref_string>(thd, this); }
};

template <class Item_type>
class Create_bel_pay_ref_func_arg1 : public Create_func_arg1
{
public:
  Item *create_1_arg(THD *thd, Item *arg) override
  { return new (thd->mem_root) Item_type(thd, arg); }
};

#define BEL_PAY_REF_STRING_FUNCTION(CLASS_NAME, OPERATION, SQL_NAME)          \
class CLASS_NAME : public Item_func_bel_pay_ref_string                       \
{                                                                             \
public:                                                                       \
  CLASS_NAME(THD *thd, Item *arg)                                             \
    : Item_func_bel_pay_ref_string(thd, arg, OPERATION,                       \
                                    SQL_NAME) {}                               \
}

BEL_PAY_REF_STRING_FUNCTION(Item_func_bel_pay_ref_base, BASE,
                            "bel_pay_ref_base"_LEX_CSTRING);
BEL_PAY_REF_STRING_FUNCTION(Item_func_bel_pay_ref_check_digits, CHECK_DIGITS,
                            "bel_pay_ref_check_digits"_LEX_CSTRING);
BEL_PAY_REF_STRING_FUNCTION(Item_func_bel_pay_ref_format, FORMAT,
                            "bel_pay_ref_format"_LEX_CSTRING);
BEL_PAY_REF_STRING_FUNCTION(Item_func_bel_pay_ref_compact, COMPACT,
                            "bel_pay_ref_compact"_LEX_CSTRING);
BEL_PAY_REF_STRING_FUNCTION(Item_func_bel_pay_ref_generate, GENERATE,
                            "bel_pay_ref_generate"_LEX_CSTRING);

static Create_bel_pay_ref_func_arg1<Item_func_bel_pay_ref_is_valid>
  create_bel_pay_ref_is_valid;
static Create_bel_pay_ref_func_arg1<Item_func_bel_pay_ref_base>
  create_bel_pay_ref_base;
static Create_bel_pay_ref_func_arg1<Item_func_bel_pay_ref_check_digits>
  create_bel_pay_ref_check_digits;
static Create_bel_pay_ref_func_arg1<Item_func_bel_pay_ref_format>
  create_bel_pay_ref_format;
static Create_bel_pay_ref_func_arg1<Item_func_bel_pay_ref_compact>
  create_bel_pay_ref_compact;
static Create_bel_pay_ref_func_arg1<Item_func_bel_pay_ref_generate>
  create_bel_pay_ref_generate;

Plugin_function plugin_descriptor_bel_pay_ref_is_valid(
  &create_bel_pay_ref_is_valid);
Plugin_function plugin_descriptor_bel_pay_ref_base(&create_bel_pay_ref_base);
Plugin_function plugin_descriptor_bel_pay_ref_check_digits(
  &create_bel_pay_ref_check_digits);
Plugin_function plugin_descriptor_bel_pay_ref_format(
  &create_bel_pay_ref_format);
Plugin_function plugin_descriptor_bel_pay_ref_compact(
  &create_bel_pay_ref_compact);
Plugin_function plugin_descriptor_bel_pay_ref_generate(
  &create_bel_pay_ref_generate);

class Item_func_bel_pay_ref_generate_parts : public Item_str_func
{
public:
  Item_func_bel_pay_ref_generate_parts(THD *thd, Item *arg)
    : Item_str_func(thd, arg) {}
  Item_func_bel_pay_ref_generate_parts(THD *thd, Item *first, Item *second)
    : Item_str_func(thd, first, second) {}

  bool fix_length_and_dec(THD *thd) override
  {
    collation.set(args[0]->collation);
    max_length= bel_pay_ref::FORMATTED_LENGTH;
    set_maybe_null();

    /*
      Same rationale as Item_func_bel_pay_ref_string::fix_length_and_dec:
      the result is always plain ASCII, so mislabeling it with a
      fixed-width multi-byte charset such as ucs2/utf16/utf32 (via
      first->charset() below) would produce corrupt data. Reject it
      up front, consistent with the other BEL_PAY_REF functions. Check
      both arguments: with two arguments, either one could carry such a
      charset independently of the other.
    */
    if (args[0]->collation.collation->mbminlen > 1 ||
        (arg_count == 2 && args[1]->collation.collation->mbminlen > 1))
    {
      my_error(ER_NOT_SUPPORTED_YET, MYF(0),
               "BEL_PAY_REF functions with CHARACTER SET ucs2/utf16/utf32");
      return true;
    }
    return false;
  }

  String *val_str(String *result) override
  {
    StringBuffer<64> first_buffer;
    StringBuffer<64> second_buffer;
    String *first= args[0]->val_str(&first_buffer);
    String *second= arg_count == 2 ? args[1]->val_str(&second_buffer) : nullptr;
    if (!first || (arg_count == 2 && !second))
    {
      null_value= true;
      return nullptr;
    }

    std::string output;
    if (!bel_pay_ref::generate_parts(first->ptr(), first->length(),
                                     second ? second->ptr() : nullptr,
                                     second ? second->length() : 0, &output) ||
        result->copy(output.data(), output.size(), first->charset()))
    {
      null_value= true;
      return nullptr;
    }
    null_value= false;
    return result;
  }

  LEX_CSTRING func_name_cstring() const override
  { return "bel_pay_ref_generate_parts"_LEX_CSTRING; }
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_func_bel_pay_ref_generate_parts>(thd, this); }
};

class Create_bel_pay_ref_generate_parts : public Create_native_func
{
public:
  Item *create_native(THD *thd, const LEX_CSTRING *name,
                      List<Item> *item_list) override
  {
    const uint count= item_list ? item_list->elements : 0;
    if (count == 1)
      return new (thd->mem_root)
        Item_func_bel_pay_ref_generate_parts(thd, item_list->pop());
    if (count == 2)
    {
      Item *first= item_list->pop();
      Item *second= item_list->pop();
      return new (thd->mem_root)
        Item_func_bel_pay_ref_generate_parts(thd, first, second);
    }
    my_error(ER_WRONG_PARAMCOUNT_TO_NATIVE_FCT, MYF(0), name->str);
    return nullptr;
  }
};

static Create_bel_pay_ref_generate_parts create_bel_pay_ref_generate_parts;
Plugin_function plugin_descriptor_bel_pay_ref_generate_parts(
  &create_bel_pay_ref_generate_parts);

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
#include "sql_lex.h"
#include "sql_type_bel_pay_ref.h"
#include "bel_pay_ref_validation.h"


Type_handler_bel_pay_ref type_handler_bel_pay_ref;
Type_collection_bel_pay_ref type_collection_bel_pay_ref;

const Type_collection *Type_handler_bel_pay_ref::type_collection() const
{
  return &type_collection_bel_pay_ref;
}

const Type_handler *Type_collection_bel_pay_ref::aggregate_for_comparison(
                       const Type_handler *a, const Type_handler *b) const
{
  if (a->type_collection() == this)
    swap_variables(const Type_handler *, a, b);
  if (a == &type_handler_bel_pay_ref      || a == &type_handler_hex_hybrid ||
      a == &type_handler_tiny_blob   || a == &type_handler_blob       ||
      a == &type_handler_medium_blob || a == &type_handler_long_blob  ||
      a == &type_handler_varchar     || a == &type_handler_string     ||
      a == &type_handler_null)
    return b;
  return NULL;
}

const Type_handler *Type_collection_bel_pay_ref::aggregate_for_result(
                       const Type_handler *a, const Type_handler *b) const
{
  return aggregate_for_comparison(a,b);
}

const Type_handler *Type_collection_bel_pay_ref::aggregate_for_min_max(
                       const Type_handler *a, const Type_handler *b) const
{
  return aggregate_for_comparison(a,b);
}

const Type_handler *Type_collection_bel_pay_ref::aggregate_for_num_op(
                      const Type_handler *a, const Type_handler *b) const
{
  return NULL;
}


constexpr LEX_CSTRING Type_handler_bel_pay_ref::name_on_client;

const Type_handler *Type_handler_bel_pay_ref::type_handler_for_comparison() const
{
  return &type_handler_bel_pay_ref;
}


Field *Type_handler_bel_pay_ref::make_conversion_table_field(
      MEM_ROOT *root, TABLE *table, uint metadata, const Field *target) const
{
  /* Copied from Type_handler_varchar::make_conversion_table_field. */
  DBUG_ASSERT(HA_VARCHAR_PACKLENGTH(metadata) <= MAX_FIELD_VARCHARLENGTH);
  return new(root)
    Field_bel_pay_ref(NULL, metadata, (uchar *) "", 1,
                  Field::NONE, &empty_clex_str, table->s, target->charset());
}


Item *Type_handler_bel_pay_ref::create_typecast_item(THD *thd, Item *item,
        const Type_cast_attributes &attr) const
{
  CHARSET_INFO *real_cs= attr.charset() ?
                  attr.charset() : thd->variables.collation_connection;

  if (real_cs == &my_charset_bin)
  {
    my_error(ER_ILLEGAL_PARAMETER_DATA_TYPE_FOR_OPERATION, MYF(0),
             name().ptr(), "CHARACTER SET binary");
    return NULL;
  }

  return new (thd->mem_root) Item_bel_pay_ref_typecast(thd, item, real_cs);
}


bool Type_handler_bel_pay_ref::Column_definition_set_attributes(THD *thd,
       Column_definition *def, const Lex_field_type_st &attr,
       column_definition_type_t type) const
{
  if (Type_handler_longstr::Column_definition_set_attributes(thd, def, attr,
                                                              type))
    return true;
  def->length= bel_pay_ref::FORMATTED_LENGTH;
  return false;
}


bool Type_handler_bel_pay_ref:: Column_definition_prepare_stage1(THD *thd,
  MEM_ROOT *mem_root, Column_definition *def, column_definition_type_t type,
  const Column_derived_attributes *derived_attr) const
{
  /*
    BEL_PAY_REF has a fixed length (bel_pay_ref::FORMATTED_LENGTH
    characters): the length is intrinsic to the type and is never given
    by the user (the grammar only allows plain "BEL_PAY_REF", no "(N)").
    Force it here, then reuse Type_handler_varchar's stage1 preparation
    (character set resolution + byte-length calculation) to compute the
    on-disk layout, exactly as VARCHAR(N) would.
  */
  def->length= bel_pay_ref::FORMATTED_LENGTH;
  if (Type_handler_varchar::
      Column_definition_prepare_stage1(thd, mem_root, def, type, derived_attr))
    return true;
  if (def->charset == &my_charset_bin)
  {
    my_error(ER_ILLEGAL_PARAMETER_DATA_TYPE_FOR_OPERATION, MYF(0),
             name().ptr(), "CHARACTER SET binary");
    return true;
  }
  return false;
}


Field *Type_handler_bel_pay_ref::make_table_field(MEM_ROOT *root,
         const LEX_CSTRING *name, const Record_addr &addr,
         const Type_all_attributes &attr, TABLE_SHARE *share) const
{
  DBUG_ASSERT(HA_VARCHAR_PACKLENGTH(attr.max_length) <=
              MAX_FIELD_VARCHARLENGTH);
  return new (root) Field_bel_pay_ref(addr.ptr(), attr.max_length,
                                 addr.null_ptr(), addr.null_bit(),
                                 Field::NONE, name, share, attr.collation);
}


Field *Type_handler_bel_pay_ref::make_table_field_from_def(TABLE_SHARE *share,
         MEM_ROOT *root, const LEX_CSTRING *name, const Record_addr &rec,
         const Bit_addr &bit, const Column_definition_attributes *attr,
         uint32 flags) const
{
  return new (root) Field_bel_pay_ref(rec.ptr(), (uint32) attr->length,
            rec.null_ptr(), rec.null_bit(),
            attr->unireg_check, name, share, attr->charset);
}


Item *
Type_handler_bel_pay_ref::make_constructor_item(THD *thd, List<Item> *args) const
{
  if (!args || args->elements != 1)
    return NULL;
  Item_args tmp(thd, *args);
  return new (thd->mem_root)
    Item_bel_pay_ref_typecast(thd, tmp.arguments()[0],
                          thd->variables.collation_connection);
}


bool Type_handler_bel_pay_ref::
  Item_hybrid_func_fix_attributes(THD *thd, const LEX_CSTRING &func_name,
         Type_handler_hybrid_field_type *handler, Type_all_attributes *func,
         Item **items, uint nitems) const
{
  if (func->aggregate_attributes_string(func_name, items, nitems))
    return true;

  handler->set_handler(&type_handler_bel_pay_ref);
  return false;
}


const Type_handler *Type_handler_bel_pay_ref::
  type_handler_for_tmp_table(const Item *item) const
{
  return &type_handler_bel_pay_ref;
}



/*****************************************************************/
void Field_bel_pay_ref::sql_type(String &res) const
{
  res.set_ascii(STRING_WITH_LEN("bel_pay_ref"));
}


int Field_bel_pay_ref::store(const char *from, size_t length, CHARSET_INFO *cs)
{
  std::string formatted;
  if (bel_pay_ref::format(from, length, &formatted))
    return Field_varstring::store(formatted.data(), formatted.length(), cs);

  /*
    Invalid value: in strict SQL mode, abort the statement with an error,
    as any Field::store() implementation should. In non-strict mode,
    report a warning instead and fall through to store the field's
    canonical minimum value, matching the standard MariaDB convention
    (e.g. Field_longstr::report_if_important_data()) of only escalating
    to a hard error when thd->abort_on_warning is set.
  */
  THD *thd= get_thd();
  ErrConvString err(from, length, cs);
  if (thd->abort_on_warning)
    my_error(ER_WRONG_VALUE, MYF(0), "BEL_PAY_REF", err.ptr());
  else
    push_warning_printf(thd, Sql_condition::WARN_LEVEL_WARN, ER_WRONG_VALUE,
                        ER_THD(thd, ER_WRONG_VALUE), "BEL_PAY_REF", err.ptr());

  if (maybe_null())
    set_null();
  else
    Field_varstring::store(STRING_WITH_LEN("+++000/0000/00097+++"), cs);
  return -1;
}


/*
  We allow conversion from string types because their values fit without
  truncation. Field::store still validates every converted value.

  TODO: when replication starts sending UDT information,
  we should only return CONV_TYPE_PRECISE for the BEL_PAY_REF.
*/
enum_conv_type
Field_bel_pay_ref::rpl_conv_type_from(const Conv_source &source,
                                  const Relay_log_info *rli,
                                  const Conv_param &param) const
{
  const Type_handler *th= source.type_handler();
  if (th == &type_handler_tiny_blob ||
      th == &type_handler_medium_blob ||
      th == &type_handler_long_blob ||
      th == &type_handler_blob ||
      th == &type_handler_blob_compressed ||
      th == &type_handler_string ||
      th == &type_handler_var_string ||
      th == &type_handler_varchar ||
      th == &type_handler_varchar_compressed)
  {
    return CONV_TYPE_PRECISE;
  }

  return CONV_TYPE_IMPOSSIBLE;
}


class Item_bel_pay_ref_typecast_func_handler: public Item_handled_func::Handler_str
{
public:
  const Type_handler *
    return_type_handler(const Item_handled_func *item) const override
  { return &type_handler_bel_pay_ref; }

  const Type_handler *
    type_handler_for_create_select(const Item_handled_func *item) const override
  { return &type_handler_bel_pay_ref; }

  bool fix_length_and_dec(Item_handled_func *item) const override
  {
    return false;
  }
  String *val_str(Item_handled_func *item, String *to) const override
  {
    DBUG_ASSERT(dynamic_cast<const Item_bel_pay_ref_typecast*>(item));
    return static_cast<Item_bel_pay_ref_typecast*>(item)->val_str_generic(to);
  }
};


static Item_bel_pay_ref_typecast_func_handler item_bel_pay_ref_typecast_func_handler;

bool Item_bel_pay_ref_typecast::fix_length_and_dec(THD *thd)
{
  Item_char_typecast::fix_length_and_dec_str();
  set_func_handler(&item_bel_pay_ref_typecast_func_handler);

  if (cast_charset()->mbminlen > 1)
  {
    my_error(ER_NOT_SUPPORTED_YET, MYF(0),
             "CAST(AS BEL_PAY_REF CHARACTER SET ucs2/utf16/utf32)");
    return true;
  }

  return false;
}


String *Item_bel_pay_ref_typecast::val_str(String *to)
{
  String *res= Item_char_typecast::val_str(to);
  if (!res)
    return NULL;

  std::string formatted;
  if (!bel_pay_ref::format(res->ptr(), res->length(), &formatted) ||
      to->copy(formatted.data(), formatted.length(), res->charset()))
  {
    null_value= TRUE;
    return NULL;
  }

  return to;
}


void Item_bel_pay_ref_typecast::print(String *str, enum_query_type query_type)
{
  str->append(STRING_WITH_LEN("cast("));
  args[0]->print(str, query_type);
  str->append(STRING_WITH_LEN(" as bel_pay_ref"));
  print_charset(str);
  str->append(')');
}

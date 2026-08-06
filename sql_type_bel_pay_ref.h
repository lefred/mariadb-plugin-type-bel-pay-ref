#ifndef SQL_TYPE_BEL_PAY_REF_INCLUDED
#define SQL_TYPE_BEL_PAY_REF_INCLUDED
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


#include "sql_type.h"

class Type_handler_bel_pay_ref: public Type_handler_varchar
{
public:
  static constexpr LEX_CSTRING name_on_client{STRING_WITH_LEN("bel_pay_ref")};

  virtual ~Type_handler_bel_pay_ref() {}
  const Type_collection *type_collection() const override;
  uint get_column_attributes() const override { return ATTR_CHARSET; }
  const Type_handler *type_handler_for_comparison() const override;
  virtual Item *create_typecast_item(THD *thd, Item *item,
                  const Type_cast_attributes &attr) const override;

  /*
    Type_handler_varchar::Column_definition_set_attributes() requires an
    explicit user-supplied length for a table column (mirroring real
    VARCHAR(N) syntax) and otherwise rejects the column definition
    outright. BEL_PAY_REF's length is intrinsic and is never supplied by
    the user (get_column_attributes() above already disallows LENGTH),
    so that check must be bypassed here; skip straight to the shared
    Type_handler_longstr behaviour instead, as Type_handler_blob_common
    (the previous base) itself does.
  */
  bool Column_definition_set_attributes(THD *thd, Column_definition *def,
                  const Lex_field_type_st &attr,
                  column_definition_type_t type) const override;
  Field *make_conversion_table_field(MEM_ROOT *root,
                                     TABLE *table, uint metadata,
                                     const Field *target) const override;
  Log_event_data_type user_var_log_event_data_type(uint charset_nr)
                                                              const override
  {
    return Log_event_data_type(name().lex_cstring(), result_type(),
                               charset_nr, false);
  }

  bool Column_definition_prepare_stage1(THD *thd,
                                        MEM_ROOT *mem_root,
                                        Column_definition *def,
                                        column_definition_type_t type,
                                        const Column_derived_attributes
                                        *derived_attr) const override;

  Field *make_table_field(MEM_ROOT *root, const LEX_CSTRING *name,
           const Record_addr &addr, const Type_all_attributes &attr,
           TABLE_SHARE *share) const override;

  Field *make_table_field_from_def(TABLE_SHARE *share, MEM_ROOT *mem_root,
           const LEX_CSTRING *name, const Record_addr &addr,
           const Bit_addr &bit, const Column_definition_attributes *attr,
           uint32 flags) const override;

  Item *make_constructor_item(THD *thd, List<Item> *args) const override;

  bool Item_hybrid_func_fix_attributes(THD *thd, const LEX_CSTRING &func_name,
         Type_handler_hybrid_field_type *handler, Type_all_attributes *func,
         Item **items, uint nitems) const override;

  const Type_handler *type_handler_for_tmp_table(const Item *item) const
    override;

  bool Item_append_extended_type_info(Send_field_extended_metadata *to,
                                      const Item *item) const override
  {

    return to->set_data_type_name(name_on_client);
  }

  bool can_return_int() const override { return false; }
  bool can_return_decimal() const override { return false; }
  bool can_return_real() const override { return false; }
  bool can_return_date() const override { return false; }
  bool can_return_time() const override { return false; }

};

extern Type_handler_bel_pay_ref type_handler_bel_pay_ref;

class Type_collection_bel_pay_ref: public Type_collection
{
public:
  const Type_handler *aggregate_for_result(
          const Type_handler *a, const Type_handler *b) const override;
  const Type_handler *aggregate_for_comparison(
          const Type_handler *a, const Type_handler *b) const override;
  const Type_handler *aggregate_for_min_max(
          const Type_handler *a, const Type_handler *b) const override;
  const Type_handler *aggregate_for_num_op(
          const Type_handler *a, const Type_handler *b) const override;
};

#include "field.h"

/*
  A BEL_PAY_REF value is always exactly FORMATTED_LENGTH characters in its
  canonical form, so the field is a short fixed-size string, not a BLOB.
  This keeps the column directly usable in PRIMARY KEY / UNIQUE KEY /
  ordinary indexes without requiring an explicit key prefix length, unlike
  BLOB/TEXT-derived columns (BLOB's HA_BLOB_PART key flag forces callers
  to write e.g. "UNIQUE KEY (col(20))" instead of "UNIQUE KEY (col)").

  Field_string (plain CHAR) would be the closest match, but it is
  declared `final` in the server and cannot be subclassed, so this reuses
  Field_varstring (VARCHAR) instead: it carries a small 1-byte length
  prefix (values are always under 256 bytes) rather than none, but like
  CHAR it is fully indexable without a key prefix.
*/
class Field_bel_pay_ref :public Field_varstring
{
public:
  Field_bel_pay_ref(uchar *ptr_arg, uint32 len_arg, uchar *null_ptr_arg,
                uchar null_bit_arg, enum utype unireg_check_arg,
                const LEX_CSTRING *field_name_arg, TABLE_SHARE *share,
                const DTCollation &collation)
     :Field_varstring(ptr_arg, len_arg, len_arg < 256 ? 1 : 2, null_ptr_arg,
                      null_bit_arg, unireg_check_arg, field_name_arg, share,
                      collation)
  {}
  const Type_handler *type_handler() const override
  { return &type_handler_bel_pay_ref; }

  void make_send_field(Send_field *to) override
  {
    Field_longstr::make_send_field(to);
    to->set_data_type_name(Type_handler_bel_pay_ref::name_on_client);
  }

  void sql_type(String &str) const override;
  uint size_of() const  override { return sizeof(*this); }
  int  store(const char *to, size_t length, CHARSET_INFO *charset) override;
  using Field_str::store;
  enum_conv_type rpl_conv_type_from(const Conv_source &source,
                                    const Relay_log_info *rli,
                                    const Conv_param &param) const override;
};


class Item_bel_pay_ref_typecast :public Item_char_typecast
{
public:
  Item_bel_pay_ref_typecast(THD *thd, Item *a, CHARSET_INFO *cs_arg):
    Item_char_typecast(thd, a, -1, cs_arg) {}

  const Type_handler *type_handler() const override
  { return &type_handler_bel_pay_ref; }

  LEX_CSTRING func_name_cstring() const override
  {
    static LEX_CSTRING name= {STRING_WITH_LEN("cast_as_bel_pay_ref")};
    return name;
  }
  bool fix_length_and_dec(THD *thd) override;
  String *val_str(String *to) override;
  Item *shallow_copy(THD *thd) const override
  { return get_item_copy<Item_bel_pay_ref_typecast>(thd, this); }

  void print(String *str, enum_query_type query_type) override;
};

#endif // SQL_TYPE_BEL_PAY_REF_INCLUDED

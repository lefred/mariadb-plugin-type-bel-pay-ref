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


/* MariaDB BEL_PAY_REF data type and companion functions. */
#define MYSQL_SERVER
#include "mariadb.h"
#include "sql_class.h"
#include "sql_type_bel_pay_ref.h"
#include "bel_pay_ref_functions.h"
#include <mysql/plugin_data_type.h>

static struct st_mariadb_data_type plugin_descriptor_bel_pay_ref=
{
  MariaDB_DATA_TYPE_INTERFACE_VERSION,
  &type_handler_bel_pay_ref
};

#define BEL_PAY_REF_PLUGIN_ENTRY(TYPE, DESCRIPTOR, NAME, DESCRIPTION) \
{                                                               \
  TYPE, DESCRIPTOR, NAME, "lefred", DESCRIPTION,      \
  PLUGIN_LICENSE_GPL, 0, 0, 0x0100, NULL, NULL, "1.0.0",        \
  MariaDB_PLUGIN_MATURITY_BETA                                   \
}

maria_declare_plugin(type_bel_pay_ref)
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_DATA_TYPE_PLUGIN,
                     &plugin_descriptor_bel_pay_ref,
                     "bel_pay_ref",
                     "Belgian structured payment reference data type"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_is_valid,
                     "bel_pay_ref_is_valid",
                     "Validate a Belgian structured payment reference"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_base,
                     "bel_pay_ref_base",
                     "Return the 10-digit payment reference base"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_check_digits,
                     "bel_pay_ref_check_digits",
                     "Return the payment reference check digits"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_format,
                     "bel_pay_ref_format",
                     "Format a valid 12-digit payment reference"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_compact,
                     "bel_pay_ref_compact",
                     "Return a payment reference as 12 digits"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_generate,
                     "bel_pay_ref_generate",
                     "Generate a reference from a 10-digit base"),
  BEL_PAY_REF_PLUGIN_ENTRY(MariaDB_FUNCTION_PLUGIN,
                     &plugin_descriptor_bel_pay_ref_generate_parts,
                     "bel_pay_ref_generate_parts",
                     "Generate a reference from one or two numeric parts")
maria_declare_plugin_end;

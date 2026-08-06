# mariadb-plugin-type-bel-pay-ref

![mariabd-plugin-type-bel-pay-ref](logo/belpayref_type.png)

This MariaDB Server plugin adds the native `BEL_PAY_REF` data type for Belgian
structured payment references. It accepts both the canonical
`+++XXX/YYYY/ZZZZZ+++` form and the compact 12-digit form. Values are
validated on every storage and cast boundary and stored in canonical form.

The first 10 digits are the base number. The final two digits equal the base
modulo 97, with a zero remainder represented by `97`.

`BEL_PAY_REF` is stored as a short fixed-size string (its canonical form is
always exactly 20 bytes), so a column can be used directly in a `PRIMARY
KEY` / `UNIQUE KEY` / ordinary index, with no key-length prefix required:

```sql
MariaDB > CREATE TABLE refs (
       payment_reference BEL_PAY_REF NOT NULL,
       UNIQUE KEY (payment_reference)
     );
Query OK, 0 rows affected (0.001 sec)
```

In non-strict SQL mode, storing an invalid value into a `BEL_PAY_REF` column
does not abort the statement: it raises a warning instead of an error, and
the offending row is silently not written (the statement itself still
reports success, with 0 rows affected). In strict mode -- the server
default -- the same condition raises an error and aborts the statement, as
shown above:

```sql
MariaDB > SET sql_mode= '';
MariaDB > INSERT INTO invoices VALUES (3, '+++123/4567/89003+++');
Query OK, 0 rows affected, 1 warning (0.001 sec)

MariaDB > SHOW WARNINGS;
+---------+------+-------------------------------------------------------+
| Level   | Code | Message                                                |
+---------+------+-------------------------------------------------------+
| Warning | 1525 | Incorrect BEL_PAY_REF value: '+++123/4567/89003+++'   |
+---------+------+-------------------------------------------------------+
```

```sql
MariaDB > INSTALL SONAME 'type_bel_pay_ref';
MariaDB > SELECT plugin_name, plugin_type, plugin_library, plugin_description, plugin_author 
          FROM information_schema.PLUGINS WHERE plugin_library LIKE 'type_bel_pay_ref.so';
+--------------------------+-------------+---------------------+-------------------------------------------------+---------------+
| plugin_name              | plugin_type | plugin_library      | plugin_description                              | plugin_author |
+--------------------------+-------------+---------------------+-------------------------------------------------+---------------+
| bel_pay_ref              | DATA TYPE   | type_bel_pay_ref.so | Belgian structured payment reference data type  | lefred        |
| bel_pay_ref_is_valid     | FUNCTION    | type_bel_pay_ref.so | Validate a Belgian structured payment reference | lefred        |
| bel_pay_ref_base         | FUNCTION    | type_bel_pay_ref.so | Return the 10-digit payment reference base      | lefred        |
| bel_pay_ref_check_digits | FUNCTION    | type_bel_pay_ref.so | Return the payment reference check digits       | lefred        |
| bel_pay_ref_format       | FUNCTION    | type_bel_pay_ref.so | Format a valid 12-digit payment reference       | lefred        |
| bel_pay_ref_compact      | FUNCTION    | type_bel_pay_ref.so | Return a payment reference as 12 digits         | lefred        |
| bel_pay_ref_generate     | FUNCTION    | type_bel_pay_ref.so | Generate a reference from a 10-digit base       | lefred        |
+--------------------------+-------------+---------------------+-------------------------------------------------+---------------+
7 rows in set (0.009 sec)
```

Example in action:

```sql
MariaDB > CREATE TABLE invoices (
       id BIGINT UNSIGNED PRIMARY KEY,
       payment_reference BEL_PAY_REF NOT NULL
     );
Query OK, 0 rows affected (0.001 sec)

MariaDB > INSERT INTO invoices VALUES (1, '+++123/4567/89002+++');
Query OK, 1 row affected (0.016 sec)

MariaDB > INSERT INTO invoices VALUES (2, '123456789002');
Query OK, 1 row affected (0.000 sec)

MariaDB > SELECT id, payment_reference FROM invoices ORDER BY id;
+----+----------------------+
| id | payment_reference    |
+----+----------------------+
|  1 | +++123/4567/89002+++ |
|  2 | +++123/4567/89002+++ |
+----+----------------------+
2 rows in set (0.001 sec)


MariaDB > SELECT BEL_PAY_REF_BASE(payment_reference),
                 BEL_PAY_REF_CHECK_DIGITS(payment_reference)
          FROM invoices;
+-------------------------------------+---------------------------------------------+
| BEL_PAY_REF_BASE(payment_reference) | BEL_PAY_REF_CHECK_DIGITS(payment_reference) |
+-------------------------------------+---------------------------------------------+
| 1234567890                          | 02                                          |
| 1234567890                          | 02                                          |
+-------------------------------------+---------------------------------------------+
2 rows in set (0.001 sec)
```

Invalid values are rejected:

```sql
MariaDB > INSERT INTO invoices VALUES (3, '+++123/4567/89003+++');
ERROR 1525 (HY000): Incorrect BEL_PAY_REF value: '+++123/4567/89003+++'

MariaDB > set SQL_MODE='';
Query OK, 0 rows affected (0.000 sec)

MariaDB > INSERT INTO invoices VALUES (4, '+++123/4567/89003+++');
Query OK, 0 rows affected (0.001 sec)
```

## Functions

- `BEL_PAY_REF_IS_VALID(value)` returns `1` for a valid canonical or compact
  reference, `0` for an invalid value, and `NULL` for SQL `NULL`.
- `BEL_PAY_REF_BASE(value)` returns the 10-digit base of a valid reference, or
  `NULL` when invalid.
- `BEL_PAY_REF_CHECK_DIGITS(value)` returns its two check digits, preserving a
  leading zero, or `NULL` when invalid.
- `BEL_PAY_REF_FORMAT(value)` converts a valid compact 12-digit reference (or
  an already canonical one) to canonical form; it returns `NULL` if invalid.
- `BEL_PAY_REF_COMPACT(value)` converts a valid canonical reference (or an
  already compact one) to its 12-digit form; it returns `NULL` if invalid.
- `BEL_PAY_REF_GENERATE(base)` calculates the check digits for an exactly
  10-digit string and returns the canonical reference; it returns `NULL` for
  an invalid base.

### `BEL_PAY_REF_IS_VALID()`

```sql
SELECT BEL_PAY_REF_IS_VALID('+++123/4567/89002+++');
-- 1

SELECT BEL_PAY_REF_IS_VALID('123456789002');
-- 1

SELECT BEL_PAY_REF_IS_VALID('+++123/4567/89003+++');
-- 0

SELECT BEL_PAY_REF_IS_VALID(NULL);
-- NULL
```

### `BEL_PAY_REF_BASE()`

```sql
SELECT BEL_PAY_REF_BASE('+++123/4567/89002+++');
-- 1234567890
```

### `BEL_PAY_REF_CHECK_DIGITS()`

```sql
SELECT BEL_PAY_REF_CHECK_DIGITS('+++123/4567/89002+++');
-- 02
```

### `BEL_PAY_REF_FORMAT()`

```sql
SELECT BEL_PAY_REF_FORMAT('123456789002');
-- +++123/4567/89002+++
```

### `BEL_PAY_REF_COMPACT()`

```sql
SELECT BEL_PAY_REF_COMPACT('+++123/4567/89002+++');
-- 123456789002
```

### `BEL_PAY_REF_GENERATE()`

```sql
SELECT BEL_PAY_REF_GENERATE('1234567890');
-- +++123/4567/89002+++

SELECT BEL_PAY_REF_GENERATE('0000000000');
-- +++000/0000/00097+++
```

Pass bases as strings when leading zeroes matter. The functions that return a
string return `NULL` when their input is invalid or SQL `NULL`.

## Build

The plugin uses MariaDB's internal data-type API and must be compiled as part
of a MariaDB Server source tree (tested with MariaDB 13.1):

```sh
ln -s /path/to/mariadb-plugin-type-bel-pay-ref \
  /path/to/MariaDB-server/plugin/type_bel_pay_ref
cmake -S /path/to/MariaDB-server -B build
cmake --build build --target type_bel_pay_ref
```

Run the standalone validation tests with:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic \
  bel_pay_ref_validation.cc tests/bel_pay_ref_validation_test.cc \
  -o /tmp/bel-pay-ref-test
/tmp/bel-pay-ref-test
```

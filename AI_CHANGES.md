# AI-Assisted Changes

This file tracks changes made to this repository with AI assistance, per
change, so the origin of non-human-authored work stays traceable.

## 2026-08-06 — Claude Sonnet 5 (claude-sonnet-5)

**Prompted by:** a code review against MariaDB Server coding standards and a
security review, followed by a request to fix the indexing issue, add an
`mysql-test` suite, and fix the other findings from that review.

### Review findings and fixes

1. **`BEL_PAY_REF` could not be indexed without an explicit key-length
   prefix (design/high).**
   `Field_bel_pay_ref` was `Field_blob`-derived (`Type_handler_bel_pay_ref`
   was `Type_handler_long_blob`-derived), which carries BLOB's
   `HA_BLOB_PART` key flag. That forces `UNIQUE KEY (col)` / `PRIMARY KEY
   (col)` to fail with "BLOB/TEXT column used in key specification without
   a key length", even though every `BEL_PAY_REF` value is a fixed
   20-byte canonical string. **Fix:** `Field_bel_pay_ref` now derives from
   `Field_varstring` (`Type_handler_bel_pay_ref` from
   `Type_handler_varchar`), and `Column_definition_prepare_stage1` forces
   the intrinsic fixed length (`bel_pay_ref::FORMATTED_LENGTH`) instead of
   taking a user-supplied one. `Field_string` (plain `CHAR`) would have
   been the closest match but is declared `final` in the server and can't
   be subclassed; `Field_varstring` gives up a 1-byte length prefix
   (values are always under 256 bytes) but is fully indexable without a
   key prefix, like `VARCHAR`. Affected: `sql_type_bel_pay_ref.h`,
   `sql_type_bel_pay_ref.cc`.

2. **Switching to `Type_handler_varchar` broke plain `CREATE TABLE ...
   (col BEL_PAY_REF)` entirely (regression introduced by fix 1, caught by
   live testing, not by the build).**
   `Type_handler_varchar::Column_definition_set_attributes()` requires an
   explicit user-supplied length for a table column (mirroring real
   `VARCHAR(N)` syntax) and calls `thd->parse_error()` otherwise — every
   bare `col BEL_PAY_REF` column definition failed with a plain "syntax
   error" once the type derived from `Type_handler_varchar`. This was
   invisible to a normal build (it's a runtime parser behavior, not a
   compile error) and was only found by actually running `CREATE TABLE`
   against a live server. **Fix:** added
   `Type_handler_bel_pay_ref::Column_definition_set_attributes()`,
   bypassing `Type_handler_varchar`'s mandatory-length check and going
   straight to the shared `Type_handler_longstr` behavior instead (mirroring
   what `Type_handler_blob_common`, the previous base, already did), then
   forcing the intrinsic fixed length. Affected: `sql_type_bel_pay_ref.h`,
   `sql_type_bel_pay_ref.cc`.

3. **No `mysql-test` suite exercising the type through the server.**
   The plugin only had a standalone `assert()`-based C++ unit test for the
   pure-C++ validation core (still useful and kept as-is). Added
   `mysql-test/type_bel_pay_ref/` (`suite.pm`, `suite.opt`,
   `type_bel_pay_ref.test` + `.result`), modeled on the in-tree
   `MODULE_ONLY` plugin `plugin/disks` and reusing MariaDB's automatic
   `<PLUGIN>_SO` env-var / `--plugin-load-add` mechanism so the suite
   self-skips if the plugin wasn't built. Covers: plugin registration,
   canonical/compact storage, strict-mode rejection, the new
   `UNIQUE KEY` indexing behavior (finding 1) including a duplicate-key
   collision between compact- and canonical-form input, non-strict-mode
   behavior (finding 4 below), `NULL` handling, all six SQL functions
   (valid/invalid/`NULL` inputs), `CAST(... AS BEL_PAY_REF)`, and the
   multi-byte-charset rejection (finding 5 below). This suite is what
   caught findings 2 and 4 in the first place.

4. **`Field_bel_pay_ref::store()` always hard-aborted on an invalid value,
   regardless of SQL mode (real bug, caught by the new test suite).**
   The code called `my_error()` unconditionally before falling through to
   a "store the canonical minimum value" fallback path — but `my_error()`
   itself always aborts the statement, so that fallback path was dead code
   and non-strict SQL mode never actually behaved differently from strict
   mode. This was caught only once the `mysql-test` suite (finding 3) was
   run against a live server: `SET sql_mode=''; INSERT ...` with an invalid
   value still raised a hard `ER_WRONG_VALUE` error instead of the expected
   warning. **Fix:** `store()` now checks `thd->abort_on_warning` (the
   standard MariaDB idiom also used by e.g.
   `Field_longstr::report_if_important_data()`) and only calls `my_error()`
   in strict mode; otherwise it raises the same message as a warning via
   `push_warning_printf()`. Verified live: in non-strict mode the
   statement now succeeds with a warning and 0 rows affected (the invalid
   row is not written) instead of aborting. `README.md` updated to
   describe the actually-verified behavior instead of the previously
   assumed (and wrong) one. Affected: `sql_type_bel_pay_ref.cc`,
   `README.md`.

5. **Missing multi-byte-charset guard on the SQL functions.**
   `Item_bel_pay_ref_typecast::fix_length_and_dec` already rejected
   `CAST(... AS BEL_PAY_REF CHARACTER SET ucs2/utf16/utf32)`, but
   `BEL_PAY_REF_BASE/CHECK_DIGITS/FORMAT/COMPACT/GENERATE` did not apply
   the same guard, so `val_str()` would tag its always-ASCII output with
   a fixed-width multi-byte charset from the argument — mislabeled,
   incorrect data for such inputs. **Fix:** added the same
   `mbminlen > 1` guard to `Item_func_bel_pay_ref_string::fix_length_and_dec`
   in `bel_pay_ref_functions.cc`.

6. **Inconsistent output-on-failure contract in `compact()`.**
   `compact()` wrote directly into its output parameter via
   `extract_digits()`, so a failed/partial parse could leave the caller's
   string holding partial garbage instead of being left untouched, unlike
   `format()`, which always builds into a local temporary first. **Fix:**
   `bel_pay_ref_validation.cc`'s `compact()` now parses into a local
   `std::string` and only assigns to the output parameter on success,
   matching `format()`'s contract.

7. **`goto` used for a plain two-branch error path.**
   `Field_bel_pay_ref::store()` used `goto err;` where a direct
   `if (...) return ...;` reads more clearly and matches the rest of the
   file's style (no shared cleanup code justified the `goto`). **Fix:**
   refactored to a plain conditional in `sql_type_bel_pay_ref.cc` (folded
   into the same rewrite as finding 4).

8. **Redundant double-parse in the `BASE`/`CHECK_DIGITS` function path.**
   `Item_func_bel_pay_ref_string::val_str()` called `extract_digits()`
   and then `validate()` on the same input, and `validate()` internally
   calls `extract_digits()` again, parsing the string twice per call.
   **Fix:** both operations now call `bel_pay_ref::compact()` once (which
   already does extract + validate in a single parse, and is now
   failure-safe per finding 6). Affected: `bel_pay_ref_functions.cc`.

9. Documented the fixed-length/indexable storage and the verified
   non-strict-mode behavior (findings 1 and 4) in `README.md`.

### Verification performed

- Rebuilt the plugin against a full MariaDB Server source tree
  (`cmake --build --target type_bel_pay_ref`) after every change: clean
  build, no warnings throughout.
- Compiled the standalone validation core
  (`bel_pay_ref_validation.cc` + `tests/bel_pay_ref_validation_test.cc`)
  with `-Wall -Wextra -Wpedantic`: clean.
- Ran the validation core under AddressSanitizer + UndefinedBehaviorSanitizer
  through the existing unit test, plus a 2,000,000-iteration fuzz harness
  (random length 0-30, random bytes, including a null-pointer case) against
  `validate/extract_digits/format/compact/generate/calculate_check_digits`:
  zero crashes, zero sanitizer reports.
- Rebuilt `mariadbd` itself from the same source tree (the prebuilt binary
  in this environment predated a version bump and could not load the
  freshly built plugin) to get a live, version-matched server for
  functional testing — this is what actually surfaced findings 2 and 4;
  neither was visible from a clean compile alone.
- Ran the new `mysql-test/type_bel_pay_ref/type_bel_pay_ref.test` suite via
  `mysql-test-run.pl` against that live server with the plugin loaded, and
  used `--record` to write `type_bel_pay_ref.result` from real output
  rather than hand-authoring it; a subsequent non-`--record` run passes
  cleanly against that recorded baseline.

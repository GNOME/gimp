# Unit testing for libgimp

We should test every function in our released libraries and ensure they return
the correct data. This test infrastructure does this for the C library and the
Python 3 binding.

Every new test unit should be added both in C and Python 3.

## Procedure to add the C unit

C functions are tested in a real plug-in which is run by the unit test
infrastructure. Most of the boiler-plate code is contained in `c-test-header.c`
therefore you don't have to care about it.

All you must do is create a `gimp_c_test_run()` function with the following
template:

```C
static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 gint                  n_drawables,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  /* Each test must be surrounded by GIMP_TEST_START() and GIMP_TEST_END()
   * macros this way:
   */
  GIMP_TEST_START("Test name for easy debugging")
  /* Run some code and finish by an assert-like test. */
  GIMP_TEST_END(testme > 0)

  /* Do more tests as needed. */

  /* Mandatorily end the function by this macro: */
  GIMP_TEST_RETURN
}
```

This code must be in a file named only with alphanumeric letters and hyphens,
and prepended with `test-`, such as: `test-palette.c`.

The part between `test-` and `.c` must be added to the `tests` list in
`libgimp/tests/meson.build`.

## Procedure to add the Python 3 unit

Unlike C, the Python 3 API is not run as a standalone plug-in, but as Python
code directly interpreted through the `python-fu-eval` batch plug-in.

Simply add your code in a file named the same as the C file, but with `.py`
extension instead of `.c`.

The file must mandatorily start with a shebang: `#!/usr/bin/env python3`

For testing, use `gimp_assert()` as follows:

```py
#!/usr/bin/env python3

# Add your test code here.
# Then test that it succeeded with the assert-like test:
gimp_assert('Test name for easy debugging', testme > 0)

# Repeat with more tests as needed.
```

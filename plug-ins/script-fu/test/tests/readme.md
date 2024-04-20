# Testing ScriptFu using the testing framework

## Quick start

0. Rebuild GIMP.
The build must be a non-stable build (nightly/development version.)
1. View the Gimp Error Console dockable
2. Open the SF Console
3. Enter '(testing:load-test "tinyscheme.scm")'

Expect to finally see a report of testing in the SF Console.
Also expect to see "Passed" messages, as progress indicators,
in the Gimp Error Console.
You may also see much extraneous data in the SF Console,
since as a REPL, it prints the value yielded by each expression.

Some extreme test cases may take about a minute.
If you see a "Gimp is not responding" dialog, choose Wait.

"tinyscheme.scm" tests the embedded interpreter.
You can also try "pdb.scm" to test the PDB.
Or another test script to test a smaller portion.

## Organization and naming

The test language itself does not name a test.

The test scripts are in the repo at /plug-ins/script-fu/test/tests.

The filesystem of the repo organizes and names the tests.
The name of a file or directory indicates what is tested.
The tests don't know their own names.

A test script is usually many tests of one GIMP or ScriptFu object or function.
There may be many test script files for the same object.

Tests and test groups can be organized in directories in the source repo.
A directory of tests can be named for the GIMP object under test.

The leaf files and directories
are coded into larger test files.
The larger test files simply load all the files for a GIMP object.
Loading a file executes the tests and alters testing state.

The test files when installed are flattened into one directory.
Thus a test file that loads many tests loads them from the same top directory.

### Major test groups

1.  PDB: Tests ScriptFu binding to the GIMP PDB.
2.  tinyscheme: Tests the embedded TinyScheme interpreter.
3.  other: Special test programs, often contributed with a new feature of ScriptFu.

## Testing State

The process of testing produces a state in the testing framework and in Gimp.

### Testing framework state

The test framework state is the count of tests and info about failed tests.
It accumulates over a session of Gimp
(more precisely, over a session of ScriptFu Console
or over a session of any plugin that loads the testing framework.)

The tests themselves do not usually reset the test state using '(testing:reset)'.

You can get a boolean of the total testing framework state
using the predicate (testing:all-passed?) .

### Gimp State

Gimp state includes open images, installed resources, the selection, etc.
Testing has side effects on Gimp state.

To ensure tests succeed, you should test a new install of Gimp.
If you don't mind a few failed tests,
you can test later than a new install.

Tests may require that GIMP be newly started:

1. PDB tests may hardcode certain constant ID's and rely on GIMP
to consistently number ID's.

Tests may require that GIMP be newly installed:

1.  PDB tests may depend on the initial set of Gimp resources in ~/.config/GIMP

## Building for testing

### Non stable build

The test framework and test scripts are only installed in a non-stable build.

### Line numbers in error messages

The test scripts are intended to be portable across platforms
and robust to changes in the test scripts.
When testing error conditions (using assert-error)
the testing framework compares expected prefix of error messages
with actual error messages.
To do that requires either that TinyScheme be built without the compile option
to display file and line number in error messages,
OR that TinyScheme puts details such as line number as the suffix of error message.

In other words, the testing of error conditions is not exact,
only a prefix of the error message is compared.
When you are writing such a test,
write an expected error string that is a prefix that omits details.

In libscriptfu/tinyscheme/scheme.h :
```
# define SHOW_ERROR_LINE 0
```
## Test flavors

The testing framework can test normal operation and some error detection.
The test framework does not test detection of syntax errors because parsing errors
prevent the test framework from starting.

### Unit tests of small fragments

1.  Normal operation: "assert"
2.  Expected runtime errors: "assert-error"



### Functional tests of plugins

The tests are plugins themselves.
They are not usually automated, but require manual running and visual inspection.
They are found in /scripts/test

## Testing framework features

The "testing.scm" framework is simple.
Mostly it keeps stats for tests passed/failed
and some information about failed tests.

This section describes the "testing.scm" framework.
In the future, other test frameworks may coexist.

Some contributed tests have their own testing code
e.g. "byte IO".

### Tests are not embedded in the tested source

Any tests of Scheme code are NOT annotations
in the Scheme code they test.
Tests are separate scripts.

### Tests are declarative

Tests are declarative, short, and readable.
They may be ordered or have ordered expressions,
especially when they test side effects on the Gimp state.

### Tests can be order independent and repeated

Often, you can run tests in any order and repeat tests, up to a point.
Then test objects that have accumulated
might start to interfere with certain tests.

Tests generally should not hardcode GIMP ID's that GIMP assigns.

In general, run a large test, such as pdb.scm or tinyscheme.scm.
But you can also run a small test such as layer-new.scm.
Just be aware that if you run tests in an order of your choice,
and if you repeat tests in the same session,
you might start to see more errors than on the first run of a test
after a fresh start of Gimp.

### Some tests require a clean install

Tests of resources may try to create a resource (e.g. brush)
that a prior run of the test already created
and that was saved by Gimp as a setting.

For such tests, you may need to test only after a fresh install of Gimp
(when the set of resources is the set that Gimp installs.)

### The test framework does not name or number tests

The filesystem names the test files.

You identify a test by the code it executes and its order in a file.

### Progress

The test framework logs progress to the GIMP Error Console
using gimp-message.

The test framework displays failures, but not successes, as they occur.
Display is usually to the SF Console.

### History of test results

The test framework does not keep a permanent history of test results.
The test framework does not write into the file system.

It does not alter the testing scripts,
so you can load test scripts by name from a git repo
without dirtying the repo.

Test scripts may test Gimp features that write the file system.

### Known to fail tests

The test framework does not have a feature to ignore tests that fail.
That is, the framework does not support a third category of test result: known-to-fail.
Other frameworks might report success even though a known-to-fail test did fail.

You can comment out tests that fail.

### Tests cannot catch syntax errors

The test framework can not test detection of syntax errors
because parsing errors
prevent the test framework from starting.

## Writing tests

See /test/frameworks/testing.scm for more explanation of the testing language.

### Writing tests from examples

In the "MIT Scheme Reference" you might see examples like:

```
(vector 'a 'b 'c)  =>  #(a b c)
```

The '=>' symbol should be read as 'yields.'

You can convert to this test:
```
(assert '(equal?
            (vector 'a 'b 'c)
            #(a b c)))
```

Note the left and right hand sides of the MIT spec
go directly into the test.

### Equality in tests

The testing framework does not choose the notion of equality.

You can choose from among equal? string=? and other predicates.
Generally you should prefer equal?
since it tests for object sameness, component by component,
instead of pointer equality.

Often you don't need an equality predicate,
when the test expression itself has a boolean result.

### Quoting in tests

Note the use of backquote ` (backtick) and unquote ,  (comma).
When writing tests,
you must often do this to make certain expressions evaluate later,
after the assert statement starts and installs an error-hook.

The backquote makes an expression into data to pass to assert,
which will evaluate the expression.
Otherwise,  if the expression is evaluated before passing, an error may come before the assert function starts,
and the test is not properly caught or logged.

The unquote undoes the effect of the backquote: it makes the unquoted expression evaluate before passing it to an assert statement.

### Defining symbols outside a test expression

You can define symbols (say a variable or a function)
before a test expression
and refer to that symbol in the test expression
but you might need to unquote it so it evaluates
before the test expression function (assert or assert-error)
is evaluated.

## Internationalization

We intend the tests are independent of locale
(the user's preferred language.)

There is no test that changes the locale
as part of the test process.
(There is no API such as gimp-set-locale.)

To test that ScriptFu properly internationalizes,
you must change the locale and retest.
The printing of numbers is known to fail in German.

## Test coverage

You can get an approximate list of the internal PDB procedures tested:

```
>cd test/tests/PDB
>find . -name "*.scm" -exec grep -o "gimp-[a-z\-]*" {} \; | sort | uniq
```

That is, for all files with suffix .scm in the PDB directory,
grep for calls to the GIMP PDB which are like "gimp-", sort them, and get the unique names.

We strive to not use the gimp- prefix on names in comments,
exactly so this will find only actual calls.


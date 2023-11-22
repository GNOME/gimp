# test-file-plugins

Version 0.5
Jacob Boerema, 2021-2024

## Introduction

This is a Python 3 framework for testing GIMP file import and export plug-ins.
It can be run both as a plug-in from within GIMP (in case it is installed),
and from the command line in batch mode.


## Installation and set-up

1. For running as a plug-in in GIMP this plug-in needs to be in a location
   that GIMP knows of for plug-ins. By default it is only installed for
   unstable builds. For using the batch version, it is not necessary to
   have it installed.
2. There is a `tests` folder inside the plug-in folder for
   configuration files. The main configuration file is
   `config.ini` for the plug-in; I use a separate `batch-config.ini` for
   command line testing, but that's not required.
   This configuration file is where you define the file import and export
   plug-ins to test. Each plug-in has its own separate configuration
   file that defines the tests and test files which should be in the same
   folder.
   The location and name of the main configuration file can be changed by
   setting an environment variable: `GIMP_TESTS_CONFIG_FILE`.
3. The plug-in specific configuration file, e.g.
   `png-tests.ini` defines the specific test sets. This configuration file is
   expected to be in the same location as the main configuration file.
   Each test set can be enabled/disabled and needs to
   have a folder set where the test images can be found.
   This folder is expected to be relative to the base test data folder.
   By default this is the `./data/` folder, but it can also be
   changed, see below.
   The specific test images are defined in a file set in `files=`. These are
   expected to be in the same folder as the images they are testing.
   The root folder for all test images is by default `./data/`. This folder can
   be changed by setting the environment variable `GIMP_TESTS_DATA_FOLDER`.
4. The file that defines the images to be tested, should be a text file
   where each line contains the filename of one test image, optionally
   followed by a ',' and then a code for the expected result.
   Currently there are four results defined:
   - `EXPECTED_OK` (the default if omitted, if loading should succeed)
   - `EXPECTED_FAIL` (if loading should fail)
   - `EXPECTED_TODO` (if loading is currently expected to fail, but the
      intention is to fix this in the future)
   - `SKIP` (this temporarily skips testing a file, useful when a file causes
     problems working on other test files) Note: this is **only** intended
     for temporary debugging purposes. They should not be committed like that
     to our repository.
   We may add `EXPECTED_NO_CRASH` in the future, to define fuzzed images
   where we don't care whether they load or not, as long as there is no crash.

## How to run

Start GIMP, then start the plug-in from:
- Menu Filters, Development, Python-Fu, Test file import plug-ins.
Note: this plug-in is not installed for stable builds.

Or, from the commandline or a shell script:
```
export GIMP_TESTS_CONFIG_FILE="/location/of/config.ini"
export GIMP_TESTS_LOG_FILE="/location/of/logfile.log"
export GIMP_TESTS_DATA_FOLDER="/location/of/test-images/rootfolder/"
export PYTHONPATH="/location/of/your/python/script/"
cd location/of/plug-in/gimp-file-plugin-tests
cat batch-import-tests.py | gimp-console-2.99 -idf --batch-interpreter python-fu-eval -b - --quit
```

In case you run the batch version, the log file is written to the folder
where the plug-in resides. This location can be adjusted by setting the
environment variable `GIMP_TESTS_LOG_FILE`. Note that the path to this file
needs to exist!

To test for success or regressions in batch mode you can test whether there
were regressions.
You should grep for `Total number of regressions: 0`.

### Status

- The only thing currently being tested is whether the image loads or not. No
  testing is done to see if the image is correctly shown.
- When running all tiff tests from inside GIMP, it may crash, at least on
  Windows, due to the error console running out of resources because of
  the amount of messages.
- There is some preliminary export testing support, see `batch-export-tests.py`,
  but it is incomplete. That is: there is no error tracking yet, nor support
  for comparing exported images with reference images.

### File import plug-ins being tested

At this moment the following file import plug-ins have
tests available:
- file-bmp
- file-dcx
- file-dds
- file-fits
- file-gif
- file-jpeg
- file-pcx
- file-png
- file-pnm
- file-psb
- file-psd
- file-psd-load-merged
- file-psp
- file-sgi
- file-tga
- file-tiff


## Future enhancements

- Add tests for all our image loading plug-ins.
- Test if loaded images are loaded correctly, i.e.
  do they look like they should.
- Fuzzing of images before loading to test our handling of corrupt images.
  We may need to add another result type: `EXPECTED_NO_CRASH`, meaning we
  don't care whether it loads or not, as long as there is no crash.
- GUI to select/deselect tests.
- Improve file export testing.
- It would be nice to be able to capture all stdout/stderr output and
  possibly filter it for crashes, criticals and warnings, and add that to
  the log file. Maybe with an option to filter out certain kinds of messages.

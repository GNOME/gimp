#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#   GIMP - The GNU Image Manipulation Program
#   Copyright (C) 1995 Spencer Kimball and Peter Mattis
#
#   gimptestframework.py
#   Copyright (C) 2021-2024 Jacob Boerema
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""GIMP file plug-ins testing framework."""

import os
import configparser

import xml
from xml.etree.ElementTree import ElementTree, Element

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
from gi.repository import Gio


VERSION = "0.5"
AUTHORS = "Jacob Boerema"
YEARS   = "2021-2024"

EXPECTED_FAIL = 0
EXPECTED_OK   = 1
EXPECTED_TODO = 2

RESULT_FAIL   = 0
RESULT_OK     = 1
RESULT_CRASH  = 2


class PluginTestConfig(object):
    def __init__(self, log, path, testconfig):
        self.valid_config = False
        self.enabled = False

        self.plugin = testconfig.get('plugin-import')
        if self.plugin is None:
            self.plugin = testconfig.get('plugin')
            if self.plugin is not None:
                log.warning("Test is using deprecated 'plugin' parameter. Use 'plugin-import' instead!")
            else:
                log.error("Missing required 'plugin-import' parameter!")
                return

        tests = testconfig.get('tests')
        if tests is None:
            log.error("Missing required 'tests' parameter!")
            return
        testsuite_basename = tests + '-'
        self.tests = path + tests

        enabled = testconfig.get('enabled')
        if enabled == 'True':
            self.enabled = True

        self.extension = testconfig.get('extension','<missing>')
        self.valid_config = True

        # preliminary export testing support
        self.export_plugin = testconfig.get('plugin-export')
        self.export_tests = testconfig.get('tests-export')
        self.export_enabled = testconfig.get('enabled-export')

        self.testsuite_import = testsuite_basename + self.plugin
        if self.export_plugin is not None:
            self.testsuite_export = testsuite_basename + self.export_plugin


class ConfigLoader(object):
    def __init__(self, log, config_path, config_file):
        self.config_path = config_path
        self.config_file = config_file
        log.debug(f"Using config file: {self.config_path}{self.config_file}")
        self.tests = []
        self.export_tests = []
        self.problems = 0
        self.disabled = 0
        self.config = configparser.ConfigParser()
        self.config.read(self.config_path + self.config_file)

        msg  = "Available tests:"
        for key in self.config.sections():
            if key == 'main':
                pass
            else:
                # Add test config
                test = PluginTestConfig(log, self.config_path, self.config[key])
                if test.valid_config:
                    self.tests.append(test)
                    msg += f"\nPlugin: {test.plugin}, test config: {test.tests}, enabled: {test.enabled}"
                    if not test.enabled:
                        self.disabled += 1
                else:
                    log.warning(f"\nSkipping invalid config for {key}!")
                    self.problems += 1

                if test.export_plugin is not None and test.export_tests is not None:
                    log.info(f"Export test found for {key}")
                    self.export_tests.append(test)

        log.gimp_verbose_message(msg)

class FileLoadTest(object):
    def __init__(self, file_load_plugin_name, image_type, data_path, log, testsuite):
        self.log = log
        self.testsuite = testsuite
        self.data_root = data_path
        self.plugin_name = file_load_plugin_name
        self.image_type  = image_type

        self.unexpected_success_images = []
        self.unexpected_failure_images = []

        self.failure_reason = "unknown"

        self.total_tests  = 0
        self.total_failed = 0
        self.total_todo   = 0
        self.total_ok     = 0
        self.total_crash  = 0

    def run_file_load(self, image_file, expected):
        if not os.path.exists(image_file):
            msg = "Regression loading " + image_file + ". File does not exist!"
            self.failure_reason = msg
            self.log.error("--> " + msg)
            return RESULT_FAIL

        pdb_proc   = Gimp.get_pdb().lookup_procedure(self.plugin_name)
        if pdb_proc is None:
            msg = "Plug-in procedure '" + self.plugin_name + "' not found!"
            self.failure_reason = msg
            self.log.error("--> " + msg)
            return RESULT_FAIL
        pdb_config = pdb_proc.create_config()
        pdb_config.set_property('run-mode', Gimp.RunMode.NONINTERACTIVE)
        pdb_config.set_property('file', Gio.File.new_for_path(image_file))
        result = pdb_proc.run(pdb_config)
        status = result.index(0)
        img = result.index(1)
        if (status == Gimp.PDBStatusType.SUCCESS and img is not None and img.is_valid()):
            self.log.info ("Loading succeeded for " + image_file)
            img.delete()
            if expected == EXPECTED_FAIL:
                self.unexpected_success_images.append(image_file)
                msg = "Regression loading " + image_file + ". Loading unexpectedly succeeded."
                self.failure_reason = msg
                self.log.error("--> " + msg)
                return RESULT_FAIL
            elif expected == EXPECTED_TODO:
                self.unexpected_success_images.append(image_file)
                msg = "Loading unexpectedly succeeded for test marked TODO. Image: " + image_file
                self.failure_reason = msg
                self.log.error("--> " + msg)
                return RESULT_FAIL

            return RESULT_OK
        else:
            self.log.info ("Loading failed for " + image_file)
            if status == Gimp.PDBStatusType.CALLING_ERROR:
                # A calling error indicates the plug-in crashed, which should
                # always be considered failure!
                if expected == EXPECTED_OK:
                    self.unexpected_failure_images.append(image_file)
                elif expected == EXPECTED_TODO:
                    self.unexpected_success_images.append(image_file)
                msg = "Plug-in crashed while loading " + image_file + "."
                self.failure_reason = msg
                self.log.error("--> " + msg)
                return RESULT_CRASH
            elif expected == EXPECTED_OK:
                self.unexpected_failure_images.append(image_file)
                msg = "Regression loading " + image_file + ". Loading unexpectedly failed."
                self.failure_reason = msg
                self.log.error("--> " + msg)
                return RESULT_FAIL
            return RESULT_OK

    def load_test_images(self, test_images):
        test_list = []

        if not os.path.exists(test_images):
            msg = "Path does not exist: " + test_images
            self.log.error(msg)
            return None

        with open(test_images, encoding='UTF-8') as f:
            try:
                content = f.readlines()
            except UnicodeDecodeError as err:
                self.log.error(f"Invalid encoding for {test_images}: {err}")
                return None

            # strip whitespace
            content = [x.strip() for x in content]

        for line in content:
            data = line.split(",")
            data[0] = data[0].strip()
            if len(data) > 1:
                expected_str = data[1].strip()
            else:
                expected_str = 'EXPECTED_OK'
                data.append(expected_str)

            if expected_str == 'EXPECTED_FAIL':
                data[1] = EXPECTED_FAIL
            elif expected_str == 'EXPECTED_TODO':
                data[1] = EXPECTED_TODO
            elif expected_str == 'SKIP':
                continue
            else:
                # Assume that anything else is supposed to load OK
                data[1] = EXPECTED_OK
            test_list.append(data)

        return test_list

    def run_tests(self, image_folder, test_images, test_description):
        test_fail  = 0
        test_ok    = 0
        test_todo  = 0
        test_crash = 0
        test_total = 0
        test_images_list = self.load_test_images(test_images)
        if not test_images_list:
            # Maybe we should create another type of test failure (test_filesystem?)
            test_total += 1
            test_fail  += 1
            test_crash += 1
            self.total_failed += test_fail
            self.total_crash  += test_crash
            msg = f"No images found for '{test_description}'.'"
            self.log.error(msg)
            return
        test_total = len(test_images_list)

        for imgfile, expected in test_images_list:
            test_result = self.run_file_load(self.data_root + image_folder + imgfile, expected)
            el = Element("testcase")
            el.set('classname', self.testsuite.get('classname', 'unknown'))
            el.set('name', image_folder + imgfile)
            el.set('file', imgfile)
            self.testsuite.append(el)
            if test_result == RESULT_OK:
                test_ok += 1
            else:
                if test_result == RESULT_FAIL:
                    test_fail += 1
                elif test_result == RESULT_CRASH:
                    test_crash += 1
                    test_fail  += 1
                else:
                    test_crash += 1
                    test_fail  += 1
                    msg = "Invalid test result value: " + str(test_result)
                    self.log.error(msg)

                # Add failed testcase
                failure = Element("failure")
                failure.set('type', 'failure')
                failure.text = self.failure_reason
                el.append(failure)

            if expected == EXPECTED_TODO:
                test_todo += 1

        result_msg = "Test: " + test_description + "\n"
        if test_crash > 0:
            result_msg += "Number of plug-in crashes: " + str(test_crash) + "\n"

        if test_fail > 0:
            result_msg += "Failed " + str(test_fail) + " of " + str(test_total) + " tests"
            if test_todo > 0:
                result_msg += ", and " + str(test_todo) + " known test failures."
        elif test_todo > 0:
            result_msg += "All " + str(test_total) + " tests succeeded, but there are " + str(test_todo) + " known test failures."
        else:
            result_msg += "All " + str(test_total) + " tests succeeded."

        self.log.info("\n--- results ---")
        self.log.info(result_msg)
        if ((test_fail > 0 or test_todo > 0) and self.log.interactive):
            Gimp.message (result_msg)

        self.total_tests  += test_total
        self.total_todo   += test_todo
        self.total_failed += test_fail
        self.total_ok     += test_ok
        self.total_crash  += test_crash

    def show_results_total(self):
        msg  = "\n----- Test result totals -----"
        msg += f"\nTotal {self.image_type} tests: {self.total_tests}"
        if self.total_crash > 0:
            msg += f"\nTests crashed: {self.total_crash}"
        msg += f"\nTests failed: {self.total_failed}"
        if self.total_crash > 0:
            msg += f" (including crashes)"
        msg += f"\nTests todo: {self.total_todo}"
        self.log.message(msg)

        msg = ""
        for img in self.unexpected_success_images:
            msg += f"{img}\n"
        if len(msg) > 0:
            msg = "\nImages that unexpectedly loaded:\n" + msg
            self.log.message(msg)
        msg = ""
        for img in self.unexpected_failure_images:
            msg += f"{img}\n"
        if len(msg) > 0:
            msg = "\nImages that unexpectedly failed to load:\n" + msg
            self.log.message(msg)

class RunTests(object):
    def __init__(self, test, config_path, data_path, log, testsuite):
        self.test_count = 0
        self.regression_count = 0
        if os.path.exists(test.tests):
            self.plugin_test = FileLoadTest(test.plugin, test.extension, data_path, log, testsuite)
            cfg = configparser.ConfigParser()
            cfg.read(test.tests)
            for subtest in cfg.sections():
                if not 'description' in cfg[subtest]:
                    description = "Missing description for " + test.extension
                else:
                    description = cfg[subtest]['description']
                enabled = True
                if 'enabled' in cfg[subtest]:
                    if cfg[subtest]['enabled'] == 'False':
                        enabled = False
                folder = cfg[subtest]['folder']
                files = self.plugin_test.data_root + folder + cfg[subtest]['files']
                if enabled:
                    self.plugin_test.run_tests(folder, files, description)
                else:
                    log.info(f"Testing is disabled for: {description}.")

            self.plugin_test.show_results_total()
            self.test_count = self.plugin_test.total_tests
            self.regression_count = self.plugin_test.total_failed
        else:
            self.plugin_test = None
            log.error("Test path " + test.tests + " does not exist!")
            self.regression_count = 1

    def get_unexpected_success_regressions(self):
        return self.plugin_test.unexpected_success_images

    def get_unexpected_failure_regressions(self):
        return self.plugin_test.unexpected_failure_images

    def get_todo_count(self):
        return self.plugin_test.total_todo

    def get_crash_count(self):
        return self.plugin_test.total_crash


class GimpTestRunner(object):
    def __init__(self, log, test_type, test_cfg):
        self.log = log
        self.test_type = test_type
        self.test_cfg = test_cfg

        self.tests_total = 0
        self.error_total = 0
        self.todo_total  = 0
        self.crash_total = 0

        self.unexpected_success_images = []
        self.unexpected_failure_images = []

        el = Element("testsuites")
        el.set('name', test_type)
        self.xml_tree = ElementTree(el)

    def print_header(self, test_type):
        divider = "\n--------------------------------"
        msg  = f"\nGIMP file plug-in tests version {VERSION}\n"
        #msg += f"\n{AUTHORS}, {YEARS}\n"
        msg += f"\n--- Starting {test_type} test run ---"
        msg += divider

        if self.log.verbose:
            msg += f"\nConfig: {self.test_cfg.config_folder + self.test_cfg.config_file}"
            msg += f"\nLog: {self.test_cfg.log_file}"
            msg += f"\nData path: {self.test_cfg.data_folder}"

        self.log.message(msg)

    def print_footer(self, msg):
        msg += "\n--------------------------------"
        self.log.message(msg)

    def list_regressions(self):
        regression_list = ""
        msg = ""
        extra = "\n"
        for img in self.unexpected_success_images:
            msg += f"\n{img}"
        if len(msg) > 0:
            regression_list = "\nImages that unexpectedly loaded:" + msg + extra
            extra = ""

        msg = ""
        for img in self.unexpected_failure_images:
            msg += f"\n{img}"
        if len(msg) > 0:
            regression_list += "\nImages that unexpectedly failed to load:" + msg + extra

        return regression_list

    def run_tests(self):
        self.print_header(self.test_type)

        # Load test config
        cfg = ConfigLoader(self.log, self.test_cfg.config_folder, self.test_cfg.config_file)

        root = self.xml_tree.getroot()

        # Actual tests
        for test in cfg.tests:
            if test.enabled:
                self.log.consoleinfo(f"\nTesting {test.extension} import using {test.plugin}...\n")
                el = Element("testsuite")
                el.set('name', test.testsuite_import)
                el.set('classname', test.testsuite_import)
                plugin_tests = RunTests(test, cfg.config_path, self.test_cfg.data_folder, self.log, el)
                self.tests_total += plugin_tests.test_count
                self.error_total += plugin_tests.regression_count

                el.set('tests', str(plugin_tests.test_count))
                el.set('failures', str(plugin_tests.regression_count))
                el.set('errors', '0')
                el.set('skipped', '0')
                root.append(el)

                if plugin_tests.plugin_test is not None:
                    self.todo_total  += plugin_tests.get_todo_count()
                    self.crash_total += plugin_tests.get_crash_count()
                    if plugin_tests.regression_count > 0:
                        temp = plugin_tests.get_unexpected_success_regressions()
                        if len(temp) > 0:
                            self.unexpected_success_images.extend(temp)
                        temp = plugin_tests.get_unexpected_failure_regressions()
                        if len(temp) > 0:
                            self.unexpected_failure_images.extend(temp)
                else:
                    self.crash_total += 1
                self.log.consoleinfo(f"\nFinished testing {test.extension}\n")
            else:
                self.log.consoleinfo(f"Testing {test.extension} import using {test.plugin} is disabled.")

        root.set('tests', str(self.tests_total))
        root.set('failures', str(self.error_total))
        root.set('errors', '0')

        xml.etree.ElementTree.indent(self.xml_tree)
        self.xml_tree.write(self.test_cfg.xml_results_file, 'UTF-8')

        msg  = "\n--------- Test results ---------"
        if cfg.disabled > 0:
            msg += f"\nNumber of test sections disabled: {cfg.disabled}"
        if cfg.problems > 0:
            msg += f"\nNumber of test sections skipped due to configuration errors: {cfg.problems}"
        msg += f"\nTotal number of tests executed: {self.tests_total}"
        msg += f"\nTotal number of regressions: {self.error_total}"
        if self.crash_total > 0:
            msg += f"\nTotal number of crashes: {self.crash_total}"
        if self.todo_total > 0:
            msg += f"\nTotal number of todo tests: {self.todo_total}"
        if self.error_total > 0:
            msg += "\n" + self.list_regressions()
        self.print_footer(msg)

class GimpExportTestGroup(object):
    def __init__(self, group_test_cfg, group_name, group_description,
                 input_folder, output_folder):
        self.group_test_cfg = group_test_cfg
        self.group_name = group_name
        self.group_description = group_description
        self.input_folder = input_folder
        self.output_folder = output_folder

class GimpExportTestSections(object):
    def __init__(self, test, config_path, data_path, log):
        self.test_groups = []
        self.test_count = 0
        self.regression_count = 0
        self.export_test_path = config_path + test.export_tests
        if os.path.exists(self.export_test_path):
            cfg = configparser.ConfigParser()
            cfg.read(self.export_test_path)
            for subtest in cfg.sections():
                if not 'description' in cfg[subtest]:
                    description = "Missing description for " + test.extension
                    log.warning(f"Missing description for {test.extension}")
                else:
                    description = cfg[subtest]['description']
                    log.info(f"Test: {description}")
                enabled = True
                if 'enabled' in cfg[subtest]:
                    if cfg[subtest]['enabled'] == 'False':
                        enabled = False

                folder_input = cfg[subtest]['folder-input']
                folder_output = cfg[subtest]['folder-output']
                if enabled:
                    group = GimpExportTestGroup(test, test.extension, description,
                                                folder_input, folder_output)
                    self.test_groups.append(group)
                else:
                    log.info(f"Testing is disabled for: {description}.")

        else:
            log.error("Test path " + test.export_tests + " does not exist!")
            self.regression_count = 1

class GimpExportTest(object):
    def __init__(self, group_name, log):
        self.group_name = group_name
        self.log = log

        self.base_path = None
        self.input_path = None
        self.output_path = None
        self.file_import = None
        self.file_export = None

    def _setup(self, group_config, test_data_path):
        self.base_path = test_data_path
        self.input_path = self.base_path + group_config.input_folder
        self.output_path = self.base_path + group_config.output_folder
        if self.base_path is None or self.input_path is None or self.output_path is None:
            return False

        self.file_import = group_config.group_test_cfg.plugin
        self.file_export = group_config.group_test_cfg.export_plugin
        if self.file_import is None or self.file_export is None:
            return False
        return True

    def setup(self, group_config):
        pass

    def teardown(self, group_config):
        pass

    def run_test(self, group_config):
        pass

    def execute(self, group_config, test_data_path):
        if not self._setup(group_config, test_data_path):
            self.log.error("Invalid config, can't execute test!")
            return
        self.setup(group_config)
        self.run_test(group_config)
        self.teardown(group_config)

class GimpExportTestRunner(GimpTestRunner):
    def __init__(self, log, test_type, test_cfg):
        self.cfg = None
        self.log = log
        self.test_type = test_type
        self.test_cfg = test_cfg
        self.sections = None
        self.tests = []
        super().__init__(log, test_type, test_cfg)

    def load_test_configs(self):
        self.cfg = ConfigLoader(self.log, self.test_cfg.config_folder,
                                self.test_cfg.config_file)
        for test in self.cfg.export_tests:
            if test.export_enabled:
                self.sections = GimpExportTestSections(test, self.cfg.config_path,
                                                       self.test_cfg.data_folder,
                                                       self.log)
            else:
                pass

    def get_test_group_config(self, test_name):
        #FIXME maybe have this sorted and use a search...
        if self.sections is not None:
            for test in self.sections.test_groups:
                if test.group_name == test_name:
                    return test
        return None

    def add_test(self, test_class):
        self.log.info(f"Adding tests for {test_class.group_name}")
        self.tests.append(test_class)

    def run_tests(self):
        self.print_header(self.test_type)
        for test in self.tests:
            self.log.message(f"Running tests for {test.group_name}")
            test_config = self.get_test_group_config(test.group_name)
            if test_config is None:
                self.log.error(f"Could not find configuration for {test.group_name} tests!")
            else:
                test.execute(test_config, self.test_cfg.data_folder)
        self.print_footer("Export tests finished")

    def show_results(self):
        #FIXME export results are not implemented yet
        pass

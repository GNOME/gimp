#!/usr/bin/env python3
#
# shared boilerplate for the dogtail hermetic UI tests: session boot/launch/
# teardown, at-spi tree walking, fail/hard_error helpers, welcome-dialog
# dismissal.
#
# not a test itself: no args, no exit code of its own.

import os
import shutil
import subprocess
import sys
import tempfile
import time
from contextlib import contextmanager

try:
    from dogtail.hermetic.session import HermeticSession, HermeticSessionError, dump_tree
except ImportError:
    # meson.build only enables the ui suite with dogtail >= 2.1.0 installed,
    # so getting here is an environment error, not a skip.
    sys.stderr.write("ERROR: dogtail.hermetic not installed (pip install "
                     "'dogtail>=2.1.0'), but meson.build said it was present "
                     "at configure time\n")
    sys.exit(99)


def hard_error(msg):
    # meson's hard-error code: a precondition meson.build checked broke, not
    # the thing under test.
    # https://mesonbuild.com/Unit-tests.html#skipped-tests-and-hard-errors
    sys.stderr.write("ERROR: %s\n" % msg)
    sys.exit(99)


def fail(msg):
    sys.stderr.write("FAIL: %s\n" % msg)
    sys.exit(1)


def find_first(node, pred, max_depth=10, depth=0):
    # menus/dialogs are buried deep and node.child() wants an exact/unique
    # match we don't always have mid-search, so walk the whole subtree.
    try:
        if pred(node):
            return node
    except Exception:
        return None
    if depth >= max_depth:
        return None
    try:
        for child in node.children:
            found = find_first(child, pred, max_depth, depth + 1)
            if found:
                return found
    except Exception:
        pass
    return None


def find_all(node, pred, out=None, max_depth=20, depth=0):
    out = [] if out is None else out
    try:
        if pred(node):
            out.append(node)
    except Exception:
        pass
    if depth >= max_depth:
        return out
    try:
        for child in node.children:
            find_all(child, pred, out, max_depth, depth + 1)
    except Exception:
        pass
    return out


def wait_for(fn, timeout, interval=0.5):
    deadline = time.time() + timeout
    while time.time() < deadline:
        result = fn()
        if result is not None:
            return result
        time.sleep(interval)
    return None


def close_welcome_dialog(app, timeout=60):
    welcome = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "dialog"
                           and "welcome" in (n.name or "").lower()),
        timeout)
    if welcome is None:
        return
    # dialog can show up in the tree before its children, so wait for Close.
    close_btn = wait_for(
        lambda: find_first(welcome, lambda n: "button" in n.roleName
                           and n.name == "Close"),
        10)
    if close_btn is None:
        fail("welcome dialog Close button not found:\n%s" % dump_tree(welcome))
    close_btn.do_action_named("click")
    time.sleep(1)


@contextmanager
def gimp_session(gimp_exe, mutter_exe, extra_environment=None, install_input=False):
    if not os.path.exists(mutter_exe):
        hard_error("mutter not found at %s - meson.build already checked "
                   "for it, this is a build/infra bug" % mutter_exe)

    # HermeticSession execs plain "mutter" off $PATH and takes no path, so
    # put the one meson found first.
    os.environ["PATH"] = os.path.dirname(os.path.abspath(mutter_exe)) + \
        os.pathsep + os.environ.get("PATH", "")

    session = HermeticSession(virtual_monitor="1280x800", boot_timeout=20)
    try:
        session.boot()
    except HermeticSessionError as e:
        hard_error("hermetic session failed to boot: %s" % e)
    if install_input:
        session.install_input()

    config_dir = tempfile.mkdtemp(prefix="gimp-uitest-config-")
    session.config_dir = config_dir  # tests that read gimprc back need the path
    env = {"GIMP3_DIRECTORY": config_dir}
    if extra_environment:
        env.update(extra_environment)

    proc = None
    had_error = False
    try:
        proc = session.launch([gimp_exe, "--new-instance"], extra_environment=env)
        try:
            app = session.wait_for_app("gimp", timeout=120)
        except HermeticSessionError as e:
            fail("gimp never registered on the a11y bus: %s" % e)
        yield session, app, proc
    except BaseException:
        # fail()/hard_error() exit through here too, and the finally still
        # runs first. remember it, so a slow-to-die gimp doesn't stack a
        # second failure.
        had_error = True
        raise
    finally:
        hung = False
        if proc is not None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
                hung = True
                if had_error:
                    sys.stderr.write("WARN: gimp also didn't exit within 5s "
                                     "of terminate(), force-killed\n")
        session.teardown()
        shutil.rmtree(config_dir, ignore_errors=True)
        if hung and not had_error:
            # gimp hanging on SIGTERM after a clean run is a gimp bug. fail
            # here, after cleanup - failing in the kill branch skips teardown.
            fail("gimp did not exit within 5s of terminate() after "
                 "an otherwise successful run")

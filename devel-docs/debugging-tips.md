There are a few environment variables, options or other more-or-less
known tricks to debug LIGMA. Let's try to add them here for reminder and
newcomers.

## Basics ##

The basic thing is to build babl, GEGL and LIGMA with `--enable-debug`.

Note that if you also built glib from source with `--enable-debug`,
every pointers of destroyed GObject are overwritten with `0xaa` bytes,
so dereferencing one after destruction usually leads to a crash, which
is a good way to find some more vicious bugs. Also it makes pointers of
destroyed data easy to spot.

## Debug logs ##

You can see various LIGMA_LOG() calls in the code. These will only be
outputted when you set LIGMA_DEBUG environment variable to a
comma-separated list of domain.
For instance, for `LIGMA_LOG (XCF, "some string")` to be outputted,
run LIGMA like this:

> LIGMA_DEBUG=xcf ligma-2.10

Special flags are:
- "all" to output all domain logs;
- "list-all" to get a list of available domains.

## Debugging a warning ##

If you encounter a CRITICAL or WARNING message on console, you can make
so that LIGMA crashes on it, which will make it very easy to be tracked
down in a debugger (for instance GDB), by running LIGMA with:

> ligma-2.10 --g-fatal-warnings

Note that if all you want is a stacktrace, it is not necessary anymore
to use a debugger and --g-fatal-warnings. In Preferences > Debugging,
make sure that all type of errors are debugged, and that you have either
gdb or lldb installed. Then a graphical dialog will automatically appear
upon encountering any WARNING or CRITICAL with backtraces and variable
contents.

Alternatively running LIGMA with the CLI option --stack-trace-mode to
values "query" or "always" will output a stacktrace too on terminal.
But this happens only for crashes, so it still requires to use
--g-fatal-warnings for WARNINGs and CRITICALs.

Note: on Windows, even the debugging GUI happens only for crashes and
requires that you built with Dr. Mingw dependency.

## Debugging GTK ##

You can use GtkInspector by running LIGMA with:

> GTK_DEBUG=interactive ligma-2.99

Alternatively you may also start it at anytime with ctrl-shift-d
shortcut, if you first enable with:

> gsettings set org.gtk.Settings.Debug enable-inspector-keybinding true

See also: https://wiki.gnome.org/Projects/GTK%2B/Inspector

Note also that running LIGMA with `GDK_SCALE=2` (or other values) allow
to test the interface in another scaling than your native one. This
settings is also available in the GtkInspector.

## Debugging GEGL code ##

You may encounter this kind of warning upon exiting LIGMA:

> EEEEeEeek! 2 GeglBuffers leaked

To debug GeglBuffer leaks, make sure you built GEGL with -Dbuildtype=debug
or -Dbuildtype=debugoptimized, and set the environment variable
GEGL_DEBUG to "buffer-alloc".
Your system also needs to have the header "execinfo.h".

## Debugging babl ##

Profile conversion is done with babl by default when possible, which is
much faster.
Setting LIGMA_COLOR_TRANSFORM_DISABLE_BABL environment variable switch
back to the old lcms implementation, which can be useful for comparison.

## Debugging X Window System error ##

Make X calls synchronous so that your crashes happen immediately with:

> ligma-2.10 --sync

You can also break on `gdk_x_error()`.

## Debugging on Windows ##

Even when run from a `cmd`, the standard and error outputs are not
displayed in the terminal. For this reason, unstable builds (i.e. with
odd minor version) pop up a debug console at start.

If you are building stable versions of LIGMA for debugging and want this
debug console as well, configure with `--enable-win32-debug-console`.

## Testing older LIGMA versions ##

A useful trick when you want to quickly test a specific LIGMA older
version (e.g. to confirm a behavior change) is to install it with our
official flatpak. The flathub repository stores past builds (up to 20 at
the time of writing). You can list them with the following command:

```
$ flatpak remote-info --log flathub org.ligma.LIGMA
```

Each build will have a "Subject" line (a comment to indicate the build
reason, it may be just dependency updates or build fixes, or sometimes a
bump in LIGMA version) and a commit hash. When you have identified the
build you want to test, update it like this:

```
flatpak update --commit=<hash-of-build> org.ligma.LIGMA
```

Then just run your older LIGMA!

## Debugging on flatpak

### Generic flatpak abilities

If you want to inspect the sandbox environment, you can do so by
specifying a custom shell instead of the LIGMA binary with the following
command:

```
flatpak run --command=bash org.ligma.LIGMA
```

This will run the exact same environment as the flatpak, which should
also be identical on all machines.

Alternatively you can run the LIGMA Flatpak using the GNOME SDK as
runtime (instead of the .Platform) with the `--devel` option. In
particular, it gives you access to a few additional debug tools, such as
`gdb`. Therefore running LIGMA with --devel will give better stacktrace,
or you can run explicitly LIGMA inside GDB.

Additionally to install debug symbols in the sandbox, run:

```
flatpak install flathub org.ligma.LIGMA.Debug
flatpak install flathub org.gnome.Sdk.Debug
```

### Getting accurate traces from reported inaccurate traces

Even with reporter trace without debug symbols (yet debug symbols
installed on your side), if you make sure you use exactly the same
flatpak commit as the reporter (see `Testing older LIGMA versions`
section), you are ensured to use the same binaries. Hence you can trace
back the code line from an offset.

For instance, say that your trace has this output:

```
ligma-2.10(file_open_image+0x4e8)[0x5637e0574738]
```

Here is how you can find the proper code line in the Flatpak (provided
you use the right flatpak commit):

```
$ RUNTIME_VERSION=$(flatpak run org.ligma.LIGMA//stable -v |grep runtime=runtime/org.gnome.Platform/ |sed 's$runtime=runtime/org.gnome.Platform/$$')
$ flatpak install flathub org.ligma.LIGMA.Debug org.gnome.Sdk/$RUNTIME_VERSION org.gnome.Sdk.Debug/$RUNTIME_VERSION
$ flatpak run --devel --command=bash org.ligma.LIGMA
(flatpak) $ gdb /app/bin/ligma-2.10
(gdb) info line *(file_open_image+0x4e8)
Line 216 of "file-open.c" starts at address 0x4d5738 <file_open_image+1256> and ends at 0x4d5747 <file_open_image+1271>.
```

### Debugging though gdbserver

In some cases, when LIGMA grabs the pointer and/or keyboard, i.e. when
you debug something happening on canvas in particular, it might be very
hard to get back to your debugger, since your system won't be responding
to your keyboard or click events.

To work around this, you can debug remotely, or simply from a TTY (while
the desktop doesn't answer properly):

In your desktop, run LIGMA through a debugger server:

```
$ flatpak run --devel --command=bash org.ligma.LIGMA//beta
$ gdbserver :9999 /app/bin/ligma-2.99
```

Go to a TTY and run

```
$ gdb /app/bin/ligma-2.99
(gdb) target remote localhost:9999
(gdb) continue
```

Of course, before the `continue`, you may add whatever break points or
other commands necessary for your specific issue. LIGMA will start in the
desktop when you hit `continue` (it will likely be a slower start).

Then do your issue reproduction steps on LIGMA. When you need to debug,
you can go to the TTY whenever you want, not bothering about any
keyboard/pointer grabs.

Note that since copy-pasting is harder in a TTY, you might want to
redirect your output to a file, especially if you need to upload it or
read it slowly next to LIGMA code. For instance here are commands to
output a full backtrace into a file from the gdb prompt and exit (to
force the device ungrab by killing LIGMA and go work on desktop again):

```
(gdb) set logging file ligma-issue-1234.txt
(gdb) set logging on
(gdb) thread apply all backtrace full
(gdb) quit
```

## Debugging icons ##

See file `devel-docs/icons.txt` to learn more about our icons, and in
particular the environment variable LIGMA_ICONS_LIKE_A_BOSS.

## Debugging plug-ins ##

See file `devel-docs/debug-plug-ins.txt` for usage of environment
variable LIGMA_PLUGIN_DEBUG.

The CLI option --stack-trace-mode also applies to plug-ins, in order to
output a back trace on terminal.

## Performance logs ##

See file `devel-docs/performance-logs/performance-logs.md` for information
about LIGMA performance logs, which can help during optimization.

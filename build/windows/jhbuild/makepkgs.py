#!/bin/env python3
import getopt, os, sys, tarfile
from xml.etree import ElementTree

def makearchive (dest, manifest, root):
    """Makes an archive for the given jhbuild manifest"""

    filenames = None
    with open (manifest, "r") as f:
        filenames = f.readlines ()

    rootlen = len (root)
    with tarfile.open (dest, "w|bz2") as tar:
        for name in filenames:
            name = name.strip ()
            arcname = name [rootlen:]
            tar.add (name, arcname)

def parse_packagedb (packagedb_loc):
    """Parses a jhbuild packagedb.xml file"""

    tree = ElementTree.parse (packagedb_loc)
    root = tree.getroot ()
    packages = [child.attrib for child in root]

    return packages

def run (targets_loc, packages_loc):
    # Set up directories
    try:
        os.mkdir (packages_loc)
    except FileExistsError:
        pass

    # Loop over directories in targets/
    for target_name in os.listdir (targets_loc):
        target_loc = os.path.join (targets_loc, target_name)
        packagedb_loc = os.path.join (targets_loc, target_name, "_jhbuild", "packagedb.xml")
        manifests_loc = os.path.join (targets_loc, target_name, "_jhbuild", "manifests")

        # Skip folders that aren't jhbuild output dirs
        if not os.path.isfile (packagedb_loc):
            continue

        # Build arch
        build_arch = ""
        if target_name[-7:] == "-x86_64":
            build_arch = "x86_64"
        elif target_name[-5:] == "-i686":
            build_arch = "i686"
        else:
            print ("Warning: BUILD_ARCH suffix not recognized")

        # Create archives for each package
        for package in parse_packagedb (packagedb_loc):
            tar_name = package["package"] + '-' + build_arch + '-' + package["version"] + '.tar.bz2'
            tar_loc = os.path.join (packages_loc, tar_name)
            manifest_loc = os.path.join (manifests_loc, package["package"])

            if os.path.isfile (tar_loc):
                print ("Archive " + tar_name + " exists; Skipping")
                continue

            print ("Making " + tar_name)
            try:
                makearchive (tar_loc, manifest_loc, target_loc)
            except FileNotFoundError as e:
                print ()
                print ("!!!!!!!! Warning: Creation of " + tar_name + " failed !!!!!!!!")
                print (e)
                print ()

                try:
                    os.remove (tar_loc)
                except FileNotFoundError:
                    pass

# Usage information
def usage ():
    print ("""
Creates .tar.bz2 files for each of jhbuild's output packages

Available options:
 -t, --target-directory=DIRECTORY
    Output package files into DIRECTORY
""")

# Command line parameter parsing
def main ():
    try:
        opts, args = getopt.getopt (sys.argv[1:], "ht:", ["help", "target-directory="])
    except getopt.GetoptError as err:
        print (err)
        usage ()
        sys.exit (2)

    packages_loc = None
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-t", "--target-directory"):
            packages_loc = a
        else:
            assert False, "unhandled option"

    scriptdir_loc = os.path.dirname (os.path.realpath (__file__))
    targets_loc = os.path.join (scriptdir_loc, "targets")
    if packages_loc is None:
        packages_loc = os.path.join (targets_loc, "packages")

    run (targets_loc, packages_loc)

if __name__ == "__main__":
    main ()

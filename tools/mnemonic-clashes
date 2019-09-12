#!/bin/sh

srcdir="`dirname \"$0\"`/.."

find_actions () {
    for f in "$srcdir"/app/actions/*-actions.c; do
        < $f \
        tr '\n' ' ' | \
        grep -Po '{ *".*?".*?(NC_\("/*?" *, *)?".*?".*?}' | \
        perl -pe 's/{ *(".*?").*?(?:NC_\(".*?" *, *)?(".*?").*?}/\1: \2,/g'
    done
}

python3 - $@ <<END
from sys import argv
from itertools import chain
from glob import glob
from xml.etree.ElementTree import ElementTree

actions = {`find_actions`}

found_clashes = False

for file in glob ("$srcdir/menus/*.xml*"):
    tree = ElementTree (file = file)

    parents = {c: p for p in tree.iter () for c in p}

    def menu_path (menu):
        path = ""

        while menu:
            if menu.tag == "menu":
                if path:
                    path = " -> " + path

                path = menu.get ("name") + path

            menu = parents.get (menu)

        return path

    for menu in chain (tree.iter ("menubar-and-popup"),
                       tree.iter ("menu"),
                       tree.iter ("popup")):
        if len (argv) > 1 and menu.tag != argv[1]:
            continue

        mnemonics = {}

        found_clashes_in_menu = False

        for element in menu:
            action = element.get ("action")

            if action in actions:
                label = actions[action]

                if "_" in label:
                    mnemonic = label[label.find ("_") + 1].upper ()

                    if mnemonic not in mnemonics:
                        mnemonics[mnemonic] = []

                    mnemonics[mnemonic] += [action]

        mnemonic_list = list (mnemonics.keys ())
        mnemonic_list.sort ()

        for mnemonic in mnemonic_list:
            action_list = mnemonics[mnemonic]

            if len (action_list) > 1:
                if found_clashes:
                    print ()

                if not found_clashes_in_menu:

                    if menu.tag == "menu":
                        print ("In %s (%s):" % (menu.get ("action"),
                                                menu_path (menu)))
                    elif menu.tag == "popup":
                        print ("In %s:" % menu.get ("action"))
                    else:
                        print ("In top-level menu bar:")

                found_clashes         = True
                found_clashes_in_menu = True

                print ("    Mnemonic clash for '%s':" % mnemonic)

                for action in action_list:
                    print ("        %s: %s" % (action, actions[action]))

exit (found_clashes)
END

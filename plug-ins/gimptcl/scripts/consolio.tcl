#!@THEGIMPTCL@
# -*-Mode: Tcl;-*-
##################################################
# file: consolio.tcl
#
# This code has been modified by Eric L. Hernes to provide
# a console window for use with the Tcl extension to Gimp.
# 
# This code constructs the console window for an application.  It
# can be used by non-unix systems that do not have built-in support
# for shells.
#
# SCCS: @(#) console.tcl 1.28 96/03/07 16:32:14
#
# Copyright (c) 1995-1996 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# $Id$
#

proc gimptcl_query {} {
    gimp-install-procedure "gimptcl_consolio" \
	"An interactive gimptcl shell" \
	"Type commands at the prompt to do stuff" \
	"Eric L. Hernes" \
	"ELH" \
	"1997" \
        "<Toolbox>/Xtns/Gimptcl Consolio" \
	"" \
	extension \
	{
	    {int32 "run_mode" "Interactive, non-interactive"}
	} \
	{}
}

proc gimptcl_run {mode} {
    tkConsoleInit
}

proc consexit {args} {
    destroy .
    return $args
}

proc consputs {args} {
    switch [llength $args] {
	0 {
	   set a "\n"
	}
	1 {
	    set a "[lindex $args 0]\n"
	}
	2 {
	    if {[string compare [lindex $args 0] "-nonewline"] == 0} {
		set a [lindex $args 1]
	    } else {
		return [eval "orig_puts $args"]
	    }
	}
	3 {
	    return [orig_puts $args]
	}
    }
    .console.t insert end $a
    tkTextSetCursor .console.t end

}

proc tkConsoleInit {} {
    global tcl_platform
    frame .console
    text .console.t  -yscrollcommand ".console.sb set" -setgrid true
    scrollbar .console.sb -command ".console.t yview"
    pack .console.sb -side right -fill both
    pack .console.t -fill both -expand 1 -side left

    frame .ctl
    button .ctl.exit -text "Exit" -command "destroy ."
    label .ctl.currcmd -textvariable GimpPDBCmd
    pack .ctl.exit .ctl.currcmd -side left
    pack .console .ctl -side top -expand 1 -fill both
    tkConsoleBind .console.t

    .console.t tag configure stderr -foreground red
    .console.t tag configure stdin -foreground blue

    focus .console.t
    
    wm protocol . WM_DELETE_WINDOW { wm withdraw . }
    wm title . "The Great Consolio"
    flush stdout
    .console.t mark set output [.console.t index "end - 1 char"]
    tkTextSetCursor .console.t end
    .console.t mark set promptEnd insert
    .console.t mark gravity promptEnd left
    rename "puts" "orig_puts"
    rename "consputs" "puts"
    rename "exit" "orig_exit"
    rename "consexit" "exit"
    tkConsolePrompt
}

# tkConsoleInvoke --
# Processes the command line input.  If the command is complete it
# is evaled in the main interpreter.  Otherwise, the continuation
# prompt is added and more input may be added.
#
# Arguments:
# None.

proc tkConsoleInvoke {args} {
    set ranges [.console.t tag ranges input]
    set cmd ""
    if {$ranges != ""} {
	set pos 0
	while {[lindex $ranges $pos] != ""} {
	    set start [lindex $ranges $pos]
	    set end [lindex $ranges [incr pos]]
	    append cmd [.console.t get $start $end]
	    incr pos
	}
    }
    if {$cmd == ""} {
	tkConsolePrompt
    } elseif [info complete $cmd] {
	.console.t mark set output end
	.console.t tag delete input
	if {$cmd != ""} {
	    history add $cmd
	    catch "uplevel \#0 eval \{$cmd\}" result
	    if {$result != ""} {
		.console.t insert insert "$result\n"
	    }
	    tkConsoleHistory reset
	}
	tkConsolePrompt
    } else {
	tkConsolePrompt partial
    }
    .console.t yview -pickplace insert
}

# tkConsoleHistory --
# This procedure implements command line history for the
# console.  In general is evals the history command in the
# main interpreter to obtain the history.  The global variable
# histNum is used to store the current location in the history.
#
# Arguments:
# cmd -	Which action to take: prev, next, reset.

set histNum 1
proc tkConsoleHistory {cmd} {
    global histNum
    
    switch $cmd {
    	prev {
	    incr histNum -1
	    if {$histNum == 0} {
		set cmd {history event [expr [history nextid] -1]}
	    } else {
		set cmd "history event $histNum"
	    }
    	    if {[catch {eval $cmd} cmd]} {
    	    	incr histNum
    	    	return
    	    }
	    .console.t delete promptEnd end
    	    .console.t insert promptEnd $cmd {input stdin}
    	}
    	next {
	    incr histNum
	    if {$histNum == 0} {
		set cmd {history event [expr [history nextid] -1]}
	    } elseif {$histNum > 0} {
		set cmd ""
		set histNum 1
	    } else {
		set cmd "history event $histNum"
	    }
	    if {$cmd != ""} {
		catch {eval $cmd} cmd
	    }
	    .console.t delete promptEnd end
	    .console.t insert promptEnd $cmd {input stdin}
    	}
    	reset {
    	    set histNum 1
    	}
    }
}

# tkConsolePrompt --
# This procedure draws the prompt.  If tcl_prompt1 or tcl_prompt2
# exists in the main interpreter it will be called to generate the 
# prompt.  Otherwise, a hard coded default prompt is printed.
#
# Arguments:
# partial -	Flag to specify which prompt to print.

proc tkConsolePrompt {{partial normal}} {
    if {$partial == "normal"} {
	set temp [.console.t index "end - 1 char"]
	.console.t mark set output end
    	if [eval "info exists tcl_prompt1"] {
    	    eval "eval \[set tcl_prompt1\]"
    	} else {
    	    puts -nonewline "% "
    	}
    } else {
	set temp [.console.t index output]
	.console.t mark set output end
    	if [eval "info exists tcl_prompt2"] {
    	    eval "eval \[set tcl_prompt2\]"
    	} else {
	    puts -nonewline "> "
    	}
    }
    flush stdout
    .console.t mark set output $temp
    tkTextSetCursor .console.t end
    .console.t mark set promptEnd insert
    .console.t mark gravity promptEnd left
}

# tkConsoleBind --
# This procedure first ensures that the default bindings for the Text
# class have been defined.  Then certain bindings are overridden for
# the class.
#
# Arguments:
# None.

proc tkConsoleBind {win} {
    bindtags $win "$win Text . all"

    # Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
    # Otherwise, if a widget binding for one of these is defined, the
    # <KeyPress> class binding will also fire and insert the character,
    # which is wrong.  Ditto for <Escape>.

    bind $win <Alt-KeyPress> {# nothing }
    bind $win <Meta-KeyPress> {# nothing}
    bind $win <Control-KeyPress> {# nothing}
    bind $win <Escape> {# nothing}
    bind $win <KP_Enter> {# nothing}

    bind $win <Tab> {
	tkConsoleInsert %W \t
	focus %W
	break
    }
    bind $win <Return> {
	%W mark set insert {end - 1c}
	tkConsoleInsert %W "\n"
	tkConsoleInvoke
	break
    }
    bind $win <Delete> {
	if {[%W tag nextrange sel 1.0 end] != ""} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if [%W compare insert < promptEnd] {
		break
	    }
	}
    }
    bind $win <BackSpace> {
	if {[%W tag nextrange sel 1.0 end] != ""} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if [%W compare insert <= promptEnd] {
		break
	    }
	}
    }
    foreach left {Control-a Home} {
	bind $win <$left> {
	    if [%W compare insert < promptEnd] {
		tkTextSetCursor %W {insert linestart}
	    } else {
		tkTextSetCursor %W promptEnd
            }
	    break
	}
    }
    foreach right {Control-e End} {
	bind $win <$right> {
	    tkTextSetCursor %W {insert lineend}
	    break
	}
    }
    bind $win <Control-d> {
	if [%W compare insert < promptEnd] {
	    break
	}
    }
    bind $win <Control-k> {
	if [%W compare insert < promptEnd] {
	    %W mark set insert promptEnd
	}
    }
    bind $win <Control-t> {
	if [%W compare insert < promptEnd] {
	    break
	}
    }
    bind $win <Meta-d> {
	if [%W compare insert < promptEnd] {
	    break
	}
    }
    bind $win <Meta-BackSpace> {
	if [%W compare insert <= promptEnd] {
	    break
	}
    }
    bind $win <Control-h> {
	if [%W compare insert <= promptEnd] {
	    break
	}
    }
    foreach prev {Control-p Up} {
	bind $win <$prev> {
	    tkConsoleHistory prev
	    break
	}
    }
    foreach prev {Control-n Down} {
	bind $win <$prev> {
	    tkConsoleHistory next
	    break
	}
    }
    bind $win <Control-v> {
	if [%W compare insert > promptEnd] {
	    catch {
		%W insert insert [selection get -displayof %W] {input stdin}
		%W see insert
	    }
	}
	break
    }
    bind $win <Insert> {
	catch {tkConsoleInsert %W [selection get -displayof %W]}
	break
    }
    bind $win <KeyPress> {
	tkConsoleInsert %W %A
	break
    }
    foreach left {Control-b Left} {
	bind $win <$left> {
	    if [%W compare insert == promptEnd] {
		break
	    }
	    tkTextSetCursor %W insert-1c
	    break
	}
    }
    foreach right {Control-f Right} {
	bind $win <$right> {
	    tkTextSetCursor %W insert+1c
	    break
	}
    }
    bind $win <F9> {
	eval destroy [winfo child .]
	source $tk_library/console.tcl
    }
    foreach copy {F16 Meta-w Control-i} {
	bind $win <$copy> {
	    if {[selection own -displayof %W] == "%W"} {
		clipboard clear -displayof %W
		catch {
		    clipboard append -displayof %W [selection get -displayof %W]
		}
	    }
	    break
	}
    }
    foreach paste {F18 Control-y} {
	bind $win <$paste> {
	    catch {
	        set clip [selection get -displayof %W -selection CLIPBOARD]
		set list [split $clip \n\r]
		tkConsoleInsert %W [lindex $list 0]
		foreach x [lrange $list 1 end] {
		    %W mark set insert {end - 1c}
		    tkConsoleInsert %W "\n"
		    tkConsoleInvoke
		    tkConsoleInsert %W $x
		}
	    }
	    break
	}
    }
}

# tkConsoleInsert --
# Insert a string into a text at the point of the insertion cursor.
# If there is a selection in the text, and it covers the point of the
# insertion cursor, then delete the selection before inserting.  Insertion
# is restricted to the prompt area.
#
# Arguments:
# w -		The text window in which to insert the string
# s -		The string to insert (usually just a single character)

proc tkConsoleInsert {w s} {
    if {$s == ""} {
	return
    }
    catch {
	if {[$w compare sel.first <= insert]
		&& [$w compare sel.last >= insert]} {
	    $w tag remove sel sel.first promptEnd
	    $w delete sel.first sel.last
	}
    }
    if {[$w compare insert < promptEnd]} {
	$w mark set insert end	
    }
    $w insert insert $s {input stdin}
    $w see insert
}

# tkConsoleOutput --
#
# This routine is called directly by ConsolePutsCmd to cause a string
# to be displayed in the console.
#
# Arguments:
# dest -	The output tag to be used: either "stderr" or "stdout".
# string -	The string to be displayed.

proc tkConsoleOutput {dest string} {
    .console.t insert output $string $dest
    .console.t see insert
}

# tkConsoleExit --
#
# This routine is called by ConsoleEventProc when the main window of
# the application is destroyed.
#
# Arguments:
# None.

proc tkConsoleExit {} {
    exit
}

# now run the gimp main loop
gimp-main


#!/usr/bin/tclsh

proc scroll-string {str} {
    set l [string length $str]
    set t "$str$str"
    for {set i 0} {$i < $l} {incr i} {
	set sstr [string range $t $i [expr $i + 3]]
	puts $sstr
    }
}

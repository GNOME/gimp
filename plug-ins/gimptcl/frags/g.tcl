proc air-box {img drw x1 y1 x2 y2} {
    set p [list \
	       $x1 $y1 $x2 $y1\
	       $x2 $y1 $x2 $y2\
	       $x2 $y2 $x1 $y2\
	       $x1 $y2 $x1 $y1]

    set lp [llength $p]
    gimp-airbrush $img $drw 70 $lp $p
    gimp-displays-flush
}

proc paint-box {img drw x1 y1 x2 y2} {
    set p [list \
	       $x1 $y1 $x2 $y1\
	       $x2 $y1 $x2 $y2\
	       $x2 $y2 $x1 $y2\
	       $x1 $y2 $x1 $y1]

    set lp [llength $p]
    set fade [expr (abs($x2 - $x1) + abs($y2 - $y1))]
    gimp-paintbrush $img $drw $fade $lp $p
    gimp-displays-flush
}

proc air-circle {img drw n x y r} {
    set twopi [expr 2 * 3.1415926]
    set rinc [expr $twopi / $n]

    for {set theta 0} {$theta < $twopi} {set theta [expr $theta + $rinc]} {
	set X [expr $x + $r * sin($theta)]
	set Y [expr $y + $r * cos($theta)]
	lappend p $X $Y
    }
    lappend p [lindex $p 0] [lindex $p 1]
    set lp [llength $p]
    gimp-airbrush $img $drw 70 $lp $p
    gimp-displays-flush
}

proc paint-circle {img drw n x y r} {
    set twopi [expr 2 * 3.1415926]
    set rinc [expr $twopi / $n]

    for {set theta 0} {$theta < $twopi} {set theta [expr $theta + $rinc]} {
	set X [expr $x + $r * sin($theta)]
	set Y [expr $y + $r * cos($theta)]
	lappend p $X $Y
    }
    lappend p [lindex $p 0] [lindex $p 1]
    set lp [llength $p]
# [expr 3.1415926 * $r]
    set fade 0 
    gimp-paintbrush $img $drw $fade $lp $p
    gimp-displays-flush
}

proc n {} {
    set i [gimp-image-new 352 240 RGB]
    set d [gimp-layer-new $i 352 240 RGBA_IMAGE "New Layer" 100 NORMAL]
    gimp-image-add-layer $i $d 0
    gimp-drawable-fill $d BG_IMAGE_FILL
    gimp-display-new $i
    return [list $i $d]
}

proc t {} {
    set id [n]
    set i [lindex $id 0]
    set d [lindex $id 1]
# there's a bug in gimp-airbrush, this one'll core
# unless you've patched app/airbrush.c
#
    air-box $i $d 10 10 100 100
    paint-box $i $d 110 110 200 200
}

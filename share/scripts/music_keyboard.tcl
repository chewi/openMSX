# TODO:
# - optimize by precalcing the notes for all reg values
# - make a better visualisation if many channels are available (grouping?)

set_help_text toggle_music_keyboard \
{Puts a music keyboard on the On-Screen-Display for each channel and each
supported sound chip. It is not very practical yet if you have many
sound chips in your currently running MSX. The redder a key on the
keyboard is, the louder the note is played. Use the command again to
remove it. Note: it cannot handle run-time insertion/removal of sound
devices. It can handle changing of machines, though. Not all chips are
supported yet and some channels give not so useful note output (but
still it is nice to see something happening).
Note that displaying these keyboard may cause quite some CPU load!}

namespace eval music_keyboard {

# some useful constants
variable note_strings [list "C" "C#" "D" "D#" "E" "F" "F#" "G" "G#" "A" "A#" "B"]
# we define these outside the proc to gain some speed (they are precalculated)
variable loga [expr log(pow(2, 1/12.0))]
variable r3 [expr log(440.0)/$loga - 57]

variable keyboard_active false

variable soundchips
variable frequency_expr
variable volume_expr
variable prev_notes
variable note_key_color
variable num_notes
variable nof_channels
variable step_white
variable machine_switch_trigger_id
variable frame_trigger_id

proc freq_to_note { freq } {
	variable loga
	variable r3
	return [expr {($freq < 16) ? -1 : (log($freq)/$loga - $r3)}]
}

proc keyboard_init {} {
	variable num_notes
	variable nof_channels
	variable step_white
	variable note_strings
	variable prev_notes
	variable note_key_color
	variable soundchips
	variable frequency_expr
	variable volume_expr
	variable machine_switch_trigger_id

	set soundchips [list]
	set total_nof_channels 0

	foreach soundchip [machine_info sounddevice] {
		# determine number of channels
		set channel_count [soundchip_utils::get_num_channels $soundchip]
                # skip devices which don't have freq expressions (not implemented yet)
		if {[soundchip_utils::get_frequency_expr $soundchip 0] == "x"} continue

		lappend soundchips $soundchip
                set nof_channels($soundchip) $channel_count
		incr total_nof_channels $channel_count
                for {set i 0} {$i < $channel_count} {incr i} {
                        # create the expressions caches
			set frequency_expr($soundchip,$i) [soundchip_utils::get_frequency_expr $soundchip $i]
			set volume_expr($soundchip,$i) [soundchip_utils::get_volume_expr $soundchip $i]
		}
	}

	# and now create the visualisation (keyboards)
	set num_octaves 9
	set num_notes [expr $num_octaves * 12]
	set key_width 3
	set border 1
	set yborder 1
	set step_white [expr $key_width + $border];# total width of white keys
	set keyboard_height 8
	set white_key_height [expr $keyboard_height - $yborder]

	osd create rectangle music_keyboard -scaled true
	set channel_count 0
	foreach soundchip $soundchips {
		for {set channel 0} {$channel < $nof_channels($soundchip)} { incr channel} {
			osd create rectangle music_keyboard.chip${soundchip}ch${channel} -y [expr $channel_count * $keyboard_height] -w [expr ($num_octaves * 7) * $step_white + $border] -h $keyboard_height -rgba 0x101010A0
			set nof_blacks 0
			for {set note 0} {$note < $num_notes} { incr note} {
				set z -1
				set xcor 0
				if {[string range [lindex $note_strings [expr $note % 12]] end end] == "#"} {
					# black key
					set note_key_color($note) 0x000000
					set h [expr round($white_key_height * 0.7)]
					set xcor [expr ($key_width + 1) / 2]
					incr nof_blacks
				} else {
					# white key
					set h $white_key_height
					set note_key_color($note) 0xFFFFFF
					set z -2
				}				
				osd create rectangle music_keyboard.chip${soundchip}ch${channel}.key${note} -x [expr ($note - $nof_blacks) * $step_white + $border + $xcor] -y 0 -w $key_width -h $h -rgb $note_key_color($note) -z $z
			}

			set next_to_kbd_x [expr ($num_notes - $nof_blacks) * $step_white + $border]

			osd create rectangle music_keyboard.ch${channel}chip${soundchip}infofield -x $next_to_kbd_x -y [expr $channel_count * $keyboard_height] -h $keyboard_height -w [expr 320 - $next_to_kbd_x] -rgba 0x000000A0
			osd create text music_keyboard.ch${channel}chip${soundchip}infofield.notetext -rgb 0xFFFFFF -size [expr round($keyboard_height * 0.75)]
			osd create text music_keyboard.ch${channel}chip${soundchip}infofield.chlabel -rgb 0x1F1FFF -size [expr round($keyboard_height * 0.75)] -x 10 -text "[expr $channel + 1] ($soundchip)"
			set prev_notes($soundchip,$channel) 0
			incr channel_count
		}
	}

	set machine_switch_trigger_id [after machine_switch [namespace code music_keyboard_reset]]	
}

proc update_keyboard {} {

	variable num_notes
	variable nof_channels
	variable note_strings
	variable prev_notes
	variable note_key_color
	variable frame_trigger_id
	variable soundchips
	variable frequency_expr
	variable volume_expr

	foreach soundchip $soundchips {
		for {set channel 0} {$channel < $nof_channels($soundchip)} { incr channel} {
			set note [expr round([freq_to_note [eval $frequency_expr($soundchip,$channel)]])]
			set prevnote $prev_notes($soundchip,$channel)
			if {($note != $prevnote)} {
				osd configure music_keyboard.chip${soundchip}ch${channel}.key${prevnote} -rgb $note_key_color($prevnote)
			}
			if {($note < $num_notes) && ($note > 0)} {
				set volume [eval $volume_expr($soundchip,$channel)]
				set deviation [expr {round(255 * $volume)}]
				set color $note_key_color($note)
				if {$color > 0x808080} {
					set color [expr $color - ( ($deviation << 8) + $deviation)]
				} else {
					set color [expr $color + ($deviation << 16)]
				}
				osd configure music_keyboard.chip${soundchip}ch${channel}.key${note} -rgb $color

				if {$deviation > 0} {
					set note_text [lindex $note_strings [expr $note % 12]]
				} else {
					set note_text ""
				}
				set prev_notes($soundchip,$channel) $note
			} else {
				set note_text ""
			}
			osd configure music_keyboard.ch${channel}chip${soundchip}infofield.notetext -text $note_text
		}
	}
	set frame_trigger_id [after frame [namespace code update_keyboard]]
}

proc music_keyboard_reset {} {
	variable keyboard_active
	if {!$keyboard_active} {
		error "Please fix a bug in this script!"
	}
	toggle_music_keyboard
	toggle_music_keyboard
}

proc toggle_music_keyboard {} {
	variable keyboard_active
	variable frame_trigger_id
	variable volume_expr
	variable frequency_expr

	if {$keyboard_active} {
		catch {after cancel $machine_switch_trigger_id}
		catch {after cancel $frame_trigger_id}
		set keyboard_active false
		osd destroy music_keyboard
		unset volume_expr frequency_expr
	} else {
		set keyboard_active true
		keyboard_init
		update_keyboard
	}
	return ""
}

namespace export toggle_music_keyboard

} ;# namespace music_keyboard

namespace import music_keyboard::*

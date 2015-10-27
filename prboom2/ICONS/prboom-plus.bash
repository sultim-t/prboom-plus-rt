# bash completion for PrBoom+                              -*- shell-script -*-

_prboom_plus()
{
    local cur prev words cword
    _init_completion || return

    case $prev in
        -iwad|-file)
            _filedir wad
            ;;
        -deh)
            _filedir bex deh
            ;;
        -record|-playdemo|-timedemo|-fastdemo|-recordfromto)
            _filedir lmp
            ;;
    esac

    if [[ $cur == -* ]]; then
        COMPREPLY=( $( compgen -W '-1  -2  -3 -altdeath -aspect -auto -avg
        -avidemo -bexout -blockmap -boom_deh_parser -complevel -config
        -deathmatch -debugfile -deh -devparm -emulate -fast -fastdemo -ffmap
        -file -force_force_boom_brainawake -force_lxdoom_demo_compatibility
        -force_monster_avoid_hazards -force_no_dropoff -force_prboom_friction
        -force_remove_slime_trails -force_truncated_sector_specials -fullscreen
        -geom -height -iwad -levelstat -noaccel -noblit -nocheats -nodraw
        -nodrawers -nofullscreen -nojoy -nomonsters -nomouse -nomusic -nosfx
        -nosound -nowindow -playdemo -port -record -recordfromto
        -reject_pad_with_ff -resetgamma -respawn -save -setmem -shorttics
        -shotdir -skill -skipsec -solo-net -spechit -timedemo -timer -viddump
        -videodriver -vidmode -viewangle -warp -width -window' -- "$cur" ) )
    fi
} &&

complete -F _prboom_plus prboom-plus

# ex: ts=4 sw=4 et filetype=sh

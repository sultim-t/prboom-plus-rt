# bash completion for PrBoom+                              -*- shell-script -*-

_prboom_plus()
{
    local cur prev words cword
    _init_completion || return

    # Save the previous switch on the command line in the prevsw variable
    local i prevsw=""
    for (( i=1; $cword > 1 && i <= $cword; i++ )); do
        if [[ ${words[i]} == -* ]]; then
            prevsw=${words[i]}
        fi
    done

    # Allow adding more than one file with the same extension to the same switch
    case $prevsw in
        -iwad|-file)
            _filedir wad
            ;;
        -deh)
            _filedir '@(bex|deh)'
            ;;
        -record*|-*demo)
            _filedir lmp
            ;;
    esac

    if [[ $cur == -* ]]; then
        COMPREPLY=( $( compgen -W '-1 -2 -3 -altdeath -aspect -auto -avg
        -avidemo -bexout -blockmap -complevel -config -deathmatch -debugfile
        -deh -devparm -fast -fastdemo -ffmap -file -fullscreen -geom -height
        -iwad -levelstat -noaccel -noblit -nocheats -nodraw -nodrawers
        -nofullscreen -nojoy -nomonsters -nomouse -nomusic -nosfx -nosound
        -nowindow -playdemo -port -record -recordfromto -resetgamma -respawn
        -save -shorttics -shotdir -skill -skipsec -solo-net -spechit -timedemo
        -timer -viddump -videodriver -vidmode -viewangle -warp -width -window' \
        -- "$cur" ) )
    else
        # DoLooseFiles() takes any file names on the command line before the
        # first switch parm and inserts the appropriate -file, -deh or -playdemo
        # switch in front of them.
        if [[ -z "$prevsw" ]]; then
            _filedir '@(bex|deh|lmp|wad)'
        fi
    fi
} &&

complete -F _prboom_plus prboom-plus

# ex: ts=4 sw=4 et filetype=sh

#!/bin/sh

function usage() {
    cat >&2 << EOF
Usage:
    $progname [<options>] 
    
    Options:
        -h --help       show this help

    you have to run this script as root user

EOF
    exit $1
}

function die() 
{
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

# Parse command line options
options=$(getopt --longoptions 'clean,help' --options 'ch' --name "$progname" -- "$@") || usage $?
eval set -- "$options"
while true; do
    case "$1" in
        -h|--help) usage 0 ;;
        --) shift; break ;;
        ---) break ;;
        *)  die "Internal argument parsing error" ;;
    esac
done

jit_path=`echo $PWD/JIT_DEBUGGER`
display=`su - c4dev -c echo $DISPLAY`
if [[ -z "$var" ]]
then
    display=':0.0'
fi

source ../../../../SAFE_SET_KHENV
declare -x DISPLAY=$display
declare -x JIT_DEBUGGER=$jit_path
declare -x DBUS_SESSION_BUS_ADDRESS=""


#!/bin/sh

dst_root_mount="/SLES11SP3-chroot/mounts"
parent_env_file="oldenv.txt"

function usage() {
    cat >&2 << EOF
Usage:
    $progname [<options>] 
    
    Options:
        -c --clean      clean the environment setting up for fbe_test
        -h --help       show this help

    1. you have to run this script both before executing gosp3 and after executing gosp3
    2. after gosp3, you have to append 'source' ahead, i.e. 'source fbe_test_envsetup.sh'

EOF
    exit $1
}

function die() 
{
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

function clean() 
{
    if [[ ! -e "$dst_root_mount" ]]; then
        die "before cleaning up the fbe_test env, you have to exit gosp3 first"
    fi

    # del /usr/bin from /SLES11SP3-chroot/mounts
    sed -i '/\/usr\/bin/d' $dst_root_mount

    # unmount /SLES11SP3-chroot/usr/bin
    umount /SLES11SP3-chroot/usr/bin &> /dev/null

    # remove oldenv.txt
    rm -rf $parent_env_file &> /dev/null

    echo "cleaning up fbe_test environment: succeed"
}

# Parse command line options
options=$(getopt --longoptions 'clean,help' --options 'ch' --name "$progname" -- "$@") || usage $?
eval set -- "$options"
while true; do
    case "$1" in
        -c|--clean) clean; exit 1;;
        -h|--help) usage 0 ;;
        --) shift; break ;;
        ---) break ;;
        *)  die "Internal argument parsing error" ;;
    esac
done

if [[ -e "$dst_root_mount" ]]; then
    echo "------------------------------"
    echo "working mode: prev gosp3"
    echo "------------------------------"

    # add /usr/bin to /SLES11SP3-chroot/mounts
    found=0
    while read dir; do
        [[ ${dir} == "/usr/bin" ]] && found=1 && break
    done < $dst_root_mount
    if [[ $found == 0 ]]; then
        echo "/usr/bin" >> $dst_root_mount
    fi

    # save the env variable DISPLAY to oldenv.txt
    # since this env var would not be inheriented by gosp3
    echo "DISPLAY=$DISPLAY" > $parent_env_file

    # enable any clients to connect xserver
    xhost + &> /dev/null
    echo "setting up env (prev-gosp3) succeed"
else
    echo "------------------------------"
    echo "working mode: post gosp3"
    echo "------------------------------"

    # export the DISPLAY env var
    while read env_pair; do
        echo ${env_pair%=*}=${env_pair#*=}
        export ${env_pair%=*}=${env_pair#*=}
    done < $parent_env_file

    # setting up LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=$PWD
    export PATH=$PATH:$PWD

    echo "setting up env (post-gosp3) succeed"
fi



    



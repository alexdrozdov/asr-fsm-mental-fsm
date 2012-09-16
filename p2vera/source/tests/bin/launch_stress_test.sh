#!/bin/bash


max_procs=10
run_delay=1
run_task='LD_LIBRARY_PATH=`pwd`/ ./_tst_p2vera.bin > /dev/null &'
NO_ARGS=0
if [ $# -gt "$NO_ARGS" ] ; then
    while getopts "d:n:" option ; do
      case $option in
        d )
            run_delay=$OPTARG
            ;;
        n )
            max_procs=$OPTARG
            ;;
        * ) echo "Выбран недопустимый ключ. $option"
            ;;
      esac
    done
fi

function kill_tasks {
    for ((i=0; i < max_procs ; i++)) ; do
        if [ "${pids[$i]}" -gt "0" ] ; then
            kill -9 ${pids[$i]} > /dev/null
        fi
    done
    exit
}


for ((i=0; i < max_procs ; i++)) ; do
  pids[$i]=0
done

trap kill_tasks SIGHUP SIGINT SIGTERM

while true ; do
    for ((i=0; i < max_procs ; i++)) ; do
        if [ "${pids[$i]}" -gt "0" ] ; then
            kill -9 ${pids[$i]} > /dev/null
        fi
        eval $run_task
        pids[$i]=$!
        sleep $run_delay
    done
done
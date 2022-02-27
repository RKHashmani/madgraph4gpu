#!/bin/bash

#--------------------------------------------------------------------------------------

function checkProcdir()
{
  # Allow using this script only on the processes currently supported by generateAndCompare.sh
  procdir=$1 # e.g. ee_mumu or eemumu.auto
  proc=${procdir%.auto}
  case "${proc}" in
    ee_mumu) ;;
    gg_tt) ;;
    gg_ttg) ;;
    gg_ttgg) ;;
    gg_ttggg) ;;
    pp_tt) ;;
    uu_tt) ;;
    uu_dd) ;;
    bb_tt) ;;
    heft_gg_h) ;;
    *)
      echo "ERROR! Unknown process: '$proc'"
      exit 1
      return
      ;;
  esac
  # Check if the chosen procdir exists
  cd $TOPDIR
  if [ ! -d $procdir ]; then echo "ERROR! Directory not found $TOPDIR/$procdir"; exit 1; fi
  # Define the list of files to be checked
  files=$(\ls $procdir/src/*.cc $procdir/src/*.h $procdir/SubProcesses/*.cc $procdir/SubProcesses/*.h $procdir/SubProcesses/P1_*/check_sa.cc $procdir/SubProcesses/P1_*/CPPProcess.cc $procdir/SubProcesses/P1_*/CPPProcess.h $procdir/SubProcesses/P1_*/epoch_process_id.h)
  if [ "$files" == "" ]; then echo "ERROR! No files to check found in directory $TOPDIR/$procdir"; exit 1; fi
  # Check each file
  status=0
  badfiles=
  for file in $files; do
    filetmp=${file}.TMP
    \rm -f $filetmp
    \cp $file $filetmp
    ../../tools/mg-clang-format/mg-clang-format -i $filetmp
    if [ "$quiet" == "0" ]; then
      echo "-----------------------------------------------------------------"
      echo "[......] Check formatting in: $file"
      diff $file $filetmp
    else
      diff $file $filetmp > /dev/null
    fi
    if [ "$?" == "0" ]; then
      if [ $quiet -lt 2 ]; then echo "[....OK] Check formatting in: $file"; fi
      \rm -f $filetmp
    else
      if [ $quiet -lt 2 ]; then echo "[NOT OK] Check formatting in: $file"; fi
      status=$((status+1))
      badfiles="$badfiles $file"
      if [ "$rmbad" == "1" ]; then \rm -f $filetmp; fi
    fi
  done
  if [ $quiet -lt 2 ]; then echo "================================================================="; fi
  if [ "$status" == "0" ]; then
    echo "[....OK] Check formatting in: $procdir"
  else
    echo "[NOT OK] Check formatting in: $procdir"
    echo "[NOT OK] $status files with bad formatting:$badfiles"
    ###if [ "$rmbad" == "0" ]; then echo; echo "for f in $badfiles; do echo diff \$f \$f.TMP; diff \$f \$f.TMP; echo; done"; fi
  fi
  return $status
}

#--------------------------------------------------------------------------------------

function usage()
{
  echo "Usage: $0 [-q] [-rmbad] <proc>" # Only one process
  exit 1
}

#--------------------------------------------------------------------------------------

# Script directory
SCRDIR=$(cd $(dirname $0); pwd)

# Top source code directory for the chosen backend (this is OUTDIR in generateAndCompare.sh)
TOPDIR=$(dirname $SCRDIR) # e.g. epochX/cudacpp if $SCRDIR=epochX/cudacpp/CODEGEN

# Process command line arguments (https://unix.stackexchange.com/a/258514)
quiet=0
rmbad=0
for arg in "$@"; do
  shift
  if [ "$arg" == "-h" ] || [ "$arg" == "--help" ]; then
    usage; continue; # continue is unnecessary as usage will exit anyway...
  elif [ "$arg" == "-q" ]; then
    quiet=$((quiet+1)); continue
  elif [ "$arg" == "-rmbad" ]; then
    rmbad=1; continue
  else
    # Keep the possibility to collect more then one process
    # However, require a single process to be chosen (allow full cleanup before/after code generation)
    set -- "$@" "$arg"
  fi
done
if [ "$1" == "" ] || [ "$2" != "" ]; then usage; fi # Only one process
procdir=$1

# Check formatting for the chosen procee directory
checkProcdir $procdir

#!/bin/sh

user=`whoami`

if test `uname` = "Linux"; then
  shmids=`ipcs -m | grep $user | awk '{ print $1 }'`
  IPCRM="ipcrm shm"
else
  shmids=`ipcs -m | grep $user | awk '{ print $2 }'`
  IPCRM="ipcrm -m"
fi

for id in $shmids; do
  echo removing shared memory segment: $id
  $IPCRM $id
done

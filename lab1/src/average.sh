#!/bin/bash

ARGS_SUM=0
ARGS_ITER=0

while [[ $# -gt 0 ]]
do
  ARGS_SUM=$((ARGS_SUM + $1))
  ARGS_ITER=$((ARGS_ITER + 1))
  shift
done
echo ${ARGS_ITER}
echo "${ARGS_SUM}/${ARGS_ITER}"|bc -l

#!/bin/bash

outdir=${1:-./cilovy_adresar}
outfile=${2:-audio}
logfile=${3:-logfile.log}

./Reciever \
  --IP 147.229.14.27 \
  --out-dir ${outdir} \
  --out-file ${outfile} \
  --log-file ${outdir}/${logfile} \
  --show-level



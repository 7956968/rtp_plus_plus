#!/bin/bash

mkdir -p logs/results


cd logs

log_files=$(ls *.tar.gz)

if [ -d results ]; then
  rm -fr results
fi
mkdir results

for archive in $log_files 
do
  tar -xvzf $archive --strip=2 -C results
done

cd results
mkdir traces
mkdir logs
mv *_tr.txt traces

# remove symlinks
find . -maxdepth 1 -type l -exec rm -f {} \;

# rename files with host info
../../extractHostInfo.sh

mv RtpSender* logs
mv RtpReceiver* logs

cp ../../analyseLosses.sh logs
cp ../../generate_plot.sh logs
cp ../../process_logs.sh logs
cp ../../process_losses.sh logs
cp ../../process_RRs.sh logs
cp ../../template.R logs

# copy additional configuration info 
cp ../../mst.txt logs
cp ../../cloud_ips.txt logs
cp ../../t_start.txt logs
cp ../../t_stop.txt logs

cd logs
./process_logs.sh













































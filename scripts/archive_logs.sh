#!/bin/bash

# arg 1 = measurement name
cd logs
name_l=$1"_"`eval date +%Y%m%d`"_logs.tar.gz"
name_r=$1"_"`eval date +%Y%m%d`"_results.tar.gz"
tar -cvzf $name_l *.tar.gz
mv $name_l archive 
rm *.tar.gz

tar -cvzf $name_r results 
mv $name_r archive

rm -fr results
mkdir results

echo "$1 archived"


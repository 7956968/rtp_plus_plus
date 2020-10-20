#!/bin/sh

echo $1
# search for SSRC lines
# "Inserting new SSRC into DB:"

#grep -a "Inserting new SSRC into DB: " $1 

res=$( grep -a "Inserting new SSRC into DB: " $1 | awk '{print substr($10,2,length($10)-2)}')

#echo "Result:"
#echo $res

#IFS=', ' read -a array <<< $res

mkdir -p temp
mkdir -p detail
mkdir -p summary

for i in $res
do
  echo "Checking: $i"
#   :
# do whatever on $i
# search for line with RR Reporter SSRC: [0x97890fa5
  search_string="RR Reporter SSRC: \["$i
  echo "Search string: "$search_string
  match=$(grep -c "$search_string" $1)
  echo "Match " $match
  if [ $match -ne 0 ]; then
#    echo "No match"
#  else
#    echo "Match"
    fn="temp/"$1"_"$i".txt"
    fn2a="temp/"$1"_"$i"_trans.txt"
# need to escape string in sed
    fn2b="temp\/"$1"_"$i"_trans.txt"
    fn3="temp\/"$1"_"$i"_trans"
    grep -a "MemberEntry.cpp:357] \[0x[0-9,a-f]\{7,8\}] RR Reporter SSRC: \["$i $1 > $fn

    cat $fn | awk '\
    BEGIN   { print "Date Time RTT Jitter Loss LossFraction" } \
        { print substr($1,2) , " ", $2, " ", substr($24, 0, length($24) - 1), " ", substr($27,2,length($27)-2), " ", substr($29,2,length($29)-2), " ", substr($31,2,length($31)-2) } \
        END     {  } \
            ' > $fn2a

    sed "s/<<datafile>>/$fn2b/g" template.R > $fn".R"
    sed -i "s/<<imagefile>>/$fn3/g" $fn".R" 
    Rscript $fn".R"
  else
    echo "Warning: Couldn't find search string!"
  fi
done

mv temp/*_detail.png detail
mv temp/*_summary.png summary
# result=$( grep 'Inserting new SSRC into DB ' $1 |
# awk '{  first=match($0,"[0-9]+%")
#     last=match($0," packet loss")
#         s=substr($0,first,last-first)
#             print s}' )
# 
# echo $result


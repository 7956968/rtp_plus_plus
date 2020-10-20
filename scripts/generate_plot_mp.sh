#!/bin/sh

echo $1
# search for SSRC lines
# "Inserting new SSRC into DB:"

#grep -a "Inserting new SSRC into DB: " $1 

res=$( grep -a "Inserting new SSRC into DB: " $1 | awk '{print substr($10,2,length($10)-2)}')


#echo "Result:"
#echo $res

#IFS=', ' read -a array <<< $res

for i in $res
do
  echo $i
#   :
# do whatever on $i
# search for line with RR Reporter SSRC: [0x97890fa5
  search_string="RR Reporter SSRC: \["$i
#  echo "Search string: "$search_string
  match=$(grep -c "$search_string" $1)
#  echo "Match " $match
  if [ $match -ne 0 ]; then
#    echo "No match"
#  else
#    echo "Match"


# get flow ids
    flow_ids=$( grep -a "MpRtpMemberEntry.h:75] Initialising MemberEntry \["$i $1 | awk '{print $10}')
    echo "Flow ids:"
    echo $flow_ids 

    for fid in $flow_ids
    do
      echo "Flow id "$fid
      fn1=$1"_"$i"_"$fid".txt"
      fn2=$1"_"$i"_"$fid"_trans.txt"
      fn3=$1"_"$i"_"$fid"_trans"
#    grep -a "MemberEntry.cpp:329] RR Reporter SSRC: \["$i $1 > $fn
      grep -a "FlowSpecificMemberEntry.h:69] \[0x[0-9,a-f]\{8\}] Flow Id: "$fid" RR Reporter SSRC: \["$i $1 > $fn1
   
    
      cat $fn1 | awk '\
      BEGIN   { print "Date Time RTT Jitter Loss LossFraction" } \
          { print substr($1,2) , " ", $2, " ", substr($27, 0, length($27) - 1), " ", substr($30,2,length($30)-2), " ", substr($32,2,length($32)-2), " ", substr($34,2,length($34)-2) } \
          END     {  } \
              ' > $fn2

      sed "s/<<datafile>>/$fn2/g" template.R > $fn1".R"
      sed -i "s/<<imagefile>>/$fn3/g" $fn1".R" 
      Rscript $fn1".R"
    done
  fi
done
# result=$( grep 'Inserting new SSRC into DB ' $1 |
# awk '{  first=match($0,"[0-9]+%")
#     last=match($0," packet loss")
#         s=substr($0,first,last-first)
#             print s}' )
# 
# echo $result


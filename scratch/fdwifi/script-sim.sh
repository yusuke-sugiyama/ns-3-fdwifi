#!/bin/sh

NODEAMOUNTS="2 3 4 5"
RATES="0.12 0.01 0.005 0.0033 0.0025 0.002"
TRIALS=2

echo FD WiFiExample

pCheck=`which sqlite3`
if [ -z "$pCheck" ]
then
  echo "ERROR: This script requires sqlite3 (wifi-example-sim does not)."
  exit 255
fi

pCheck=`which sed`
if [ -z "$pCheck" ]
then
  echo "ERROR: This script requires sed (wifi-example-sim does not)."
  exit 255
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:bin/

if [ -e ../../data.db ]
then
  echo "Kill data.db? (y/n)"
  read ANS
  if [ "$ANS" = "yes" -o "$ANS" = "y" ]
  then
    echo Deleting database
    rm ../../data.db
  fi
fi

for trial in `seq 1 $TRIALS`
do
    export NS_GLOBAL_VALUE="RngRun=$trial"
    for amount in $NODEAMOUNTS
    do
	for rate in $RATES
	do
	    echo Trial $trial, amount $amount, rate $rate
	    ../../waf --run "fdwifi --format=db --nodeAmount=$amount --rate=$rate --run=run-$amount-$rate-$trial"
	done
    done
done

mv ../../data.db .

echo "Done; data in data.db ."

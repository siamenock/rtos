#/bin/bash

help() {
  echo "Usage: $0 [OPTIONS]" 
  echo ""
  echo "        OPTIONS   : -h help"
  echo "                    -f file"
  echo "                    -a IP"
  echo "                    -p port"
  echo "                    -c count"
  echo "                    -s sheet"
  echo "                    -t type"
  echo "                    -d debug"
}

# Default value
IP="192.168.200.10"
PORT=7
COUNT=100
FILE=""
TYPE=""
SHEET=false

while getopts "a:p:c:f:st:d" opt
do
  case $opt in
    h) 
      help
      exit
      ;;
    f) 
      FILE=$OPTARG
      ;;
    a) 
      IP=$OPTARG
      ;;
    p)
      PORT=$OPTARG
      ;;
    c)
      COUNT=$OPTARG
      ;;
    s)
      SHEET=true
      ;;
    t)
      TYPE=$OPTARG
      ;;
    d)
      set -x 
      ;;
    ?)
      help
      exit
      ;;
  esac
done


echo "Throughput test started to $IP:$PORT"

for byte in 64 128 256 512 1024 1500 
do
    COMMAND="java -cp bin Client 1 $byte $IP $PORT $COUNT"
    if [ -z $FILE ]; then
        $COMMAND
    else
        NAME="$FILE"_"$byte"
        echo "[TEST] Write result data to '$NAME'"
        echo $COMMAND > $NAME
        $COMMAND | tee -a $NAME
    fi
done

if $SHEET; then
    echo "[TEST] Write Google spreadsheet"
    if [ -z $FILE ]; then
        echo "No file found to be inserted in spreadsheet. Use -f [FILE]"
        exit
    fi
    if [ -z $TYPE ]; then
        echo "No data type assigned. Use -t [DATA TYPE]"
        exit
    fi

    python ./spreadsheet/spreadsheet.py "$FILE"_* -t -c $TYPE 
fi

clear;

FW_XML=$1
RT_XML=$2
echo "$RT_XML"

if head -n 1 "$RT_XML" | grep -q "xml"; then
  echo "'xml' found in 1st line - header update not required"
else
  echo "adding xml header info"
  sed -i '1i# <?xml version="1.0" encoding="UTF-8"?><events>' $RT_XML
  echo "</events>" >> $RT_XML
fi

python validateTestRun.py $FW_XML $RT_XML

export REQ_HEADERS="$1";curl example.com -Lso example.html;curl -i http://ip.jsontest.com/;IFS="," read -ra arr<<<$REQ_HEADERS;for i in "${arr[@]}";do curl -s http://headers.jsontest.com/ |jq -r ."\"$i\"";done;for f in *.json;do $(curl -sd "json=`cat $f`" -X POST 'http://validate.jsontest.com/'|jq -r ."validate")&&(echo $f>>valid.txt)||(echo $f>>invalid.txt);done;


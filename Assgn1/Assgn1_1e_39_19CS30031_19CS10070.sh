export REQ_HEADERS="$1";VER=$2;function log(){ [[ $VER == "VERBOSE" ]]&&echo $@;};curl example.com -Lso example.html -H 'User-Agent:Mozilla/5.0';log "Downloaded_example.com";curl -i http://ip.jsontest.com/ --header 'User-Agent:Mozilla/5.0';log "curled_ip.jsontest.com";IFS="," read -ra arr<<<$REQ_HEADERS;for i in "${arr[@]}";do curl -s http://headers.jsontest.com/ --header 'User-Agent:Mozilla/5.0'|jq -r ."\"$i\"";done;log "checked_req_headers";for f in *.json;do $(curl -sd "json=`cat $f`" -X POST 'http://validate.jsontest.com/' -H 'User-Agent:Mozilla/5.0'|jq -r ."validate")&&(echo $f>>valid.txt)||(echo $f>>invalid.txt);done;log "checked_json_validity";

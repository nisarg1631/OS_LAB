# REQ_HEADERS=''
function log(){ if [[ "$1"=="VERBOSE" ]];then echo "$@";fi }
curl example.com -Lso example.html
curl -vs http://ip.jsontest.com/ 2>&1|grep ">"

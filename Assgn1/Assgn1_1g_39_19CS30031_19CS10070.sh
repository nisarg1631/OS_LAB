for i in {1..150};do for j in {1..10};do printf "%d" "$(($RANDOM%30000-15000))"&&(($j==10))||printf ,;done;printf '\n';done>$1;((`awk -F, '{print$'$2'}' $1|grep -c "$3"`>0))&&echo "YES"||echo "NO"

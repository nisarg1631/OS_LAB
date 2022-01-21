num=$1;for i in $(seq $1);do while :;do(($i!=1&&$num%$i==0))||break&&echo $i&&num=$(($num/$i));done;done

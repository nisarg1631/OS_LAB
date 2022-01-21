for i in {1..10};do for j in {1..10};do printf "%d" "$(($RANDOM%30000-15000))">>ok.txt&&(($j==10))||printf ,>>ok.txt;done;printf '\n'>>ok.txt;done

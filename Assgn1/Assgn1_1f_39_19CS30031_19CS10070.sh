awk '{print$'$2'}' $1|tr A-Z a-z|sort|uniq -c|awk '{print$2" "$1}'|sort -k2nr>1f_output_$2_column.freq

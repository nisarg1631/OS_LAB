mkdir -p files_mod
sed 's/[[:blank:]]\+/,/g' $1|nl>files_mod/$(basename $1)

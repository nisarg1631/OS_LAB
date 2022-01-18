cp -R temp files_mod
find files_mod -exec sed -i 's/\s/,/g' {} \;

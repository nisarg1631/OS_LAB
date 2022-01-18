cp -R 1.b.files 1.b.files.out
find 1.b.files.out -f -exec sort {} -n -o {} \;
sort -n -m 1.b.files.out/*.txt | uniq -c | rev > 1.b.files.out.txt

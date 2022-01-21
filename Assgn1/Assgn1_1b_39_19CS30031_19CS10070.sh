cp -R 1.b.files 1.b.files.out
find 1.b.files.out -type f -exec sort {} -no {} \;
sort -nm 1.b.files.out/*.txt|uniq -c|awk '{print $2, $1}'>1.b.files.out.txt

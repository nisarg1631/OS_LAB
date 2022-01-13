# wget https://cse.iitkgp.ac.in/~mainack/OS/assignments/2021/01/1.b.files.zip
# unzip 1.b.files.zip
cp -R 1.b.files 1.b.files.out
find 1.b.files.out -exec sort {} -n -o {} \;
sort -n -m 1.b.files.out/*.txt | uniq -c | rev > 1.b.files.out.txt
# rm -rf 1.b.files.zip
## inorder to prevent the warning, add type -f before -exec so that it only processes plain text files

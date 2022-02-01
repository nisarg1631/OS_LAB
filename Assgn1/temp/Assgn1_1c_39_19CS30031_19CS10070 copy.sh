comm="find data -type f -not -path"
mkdir -p data/Nil
find data -type f -not -name "*.*" -not -path "data/Nil/*" -exec mv -t data/Nil {} +
find data -type f -not -path "data/Nil/*"|awk -F. '{print$(NF)}'|sort -u|xargs -I% sh -c 'mkdir -p data/%;find data -type f -name "*.%" -not -path "data/%/*" -exec mv -t data/% {} +;'
find data -type d -empty -delete

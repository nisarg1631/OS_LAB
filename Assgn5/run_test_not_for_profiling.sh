# rm -f a.out
# # g++ -std=c++17  -Wshadow  -Wall  -Wextra  -pedantic  -Wformat=2  -Wfloat-equal  -Wconversion  -Wlogical-op  -Wshift-overflow=2  -Wduplicated-cond  -Wcast-qual  -Wcast-align  -Wno-unused-result  -Wno-sign-conversion  -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=thread  -fno-sanitize-recover=all  -fstack-protector  -D_FORTIFY_SOURCE=2    -fsanitize=undefined  -D_GLIBCXX_DEBUG  -g  -DLMAO memlab.cpp -lpthread 
# g++ -std=c++17  -Wshadow  -Wall  -Wextra  -pedantic  -Wformat=2  -Wfloat-equal  -Wconversion  -Wlogical-op  -Wshift-overflow=2  -Wduplicated-cond  -Wcast-qual  -Wcast-align  -Wno-unused-result  -Wno-sign-conversion  -D_GLIBCXX_DEBUG_PEDANTIC   -fno-sanitize-recover=all  -fstack-protector  -D_FORTIFY_SOURCE=2  -fsanitize=address  -fsanitize=undefined  -fsanitize=leak -D_GLIBCXX_DEBUG  -g  memlab.cpp -lpthread 
# # g++ -std=c++17 memlab.cpp -lpthread
# ./a.out 6
# # g++  -std=c++17  -Wshadow  -Wall  -Wextra  -pedantic  -Wformat=2  -Wfloat-equal  -Wconversion  -Wlogical-op  -Wshift-overflow=2  -Wduplicated-cond  -Wcast-qual  -Wcast-align  -Wno-unused-result  -Wno-sign-conversion  -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=thread  -fno-sanitize-recover=all  -fstack-protector  -D_FORTIFY_SOURCE=2    -fsanitize=undefined  -D_GLIBCXX_DEBUG  -g  -DLMAO memlab.cpp memlab.cpp demo1.cpp  -o demo1 -lpthread
# # ./demo1
# rm -f demo2
# g++ -std=c++17  -Wshadow  -Wall  -Wextra  -pedantic  -Wformat=2  -Wfloat-equal  -Wconversion  -Wlogical-op  -Wshift-overflow=2  -Wduplicated-cond  -Wcast-qual  -Wcast-align  -Wno-unused-result  -Wno-sign-conversion  -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=thread  -fno-sanitize-recover=all  -fstack-protector  -D_FORTIFY_SOURCE=2    -fsanitize=undefined  -D_GLIBCXX_DEBUG  -g  -DLMAO memlab.cpp demo2.cpp -lpthread 
# ./a.out 6
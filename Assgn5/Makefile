CXXFLAGS = -Wshadow  -Wall  -Wextra  -pedantic  -Wformat=2  -Wfloat-equal  -Wconversion  -Wlogical-op  -Wshift-overflow=2  -Wduplicated-cond  -Wcast-qual  -Wcast-align  -Wno-unused-result  -Wno-sign-conversion  -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=thread  -fno-sanitize-recover=all  -fstack-protector  -D_FORTIFY_SOURCE=2    -fsanitize=undefined  -D_GLIBCXX_DEBUG  -g 
all: demo1 demo2  
demo1:
	g++  ${CXXFLAGS} memlab.cpp demo1.cpp  -o demo1 -lpthread
demo2:
	g++ ${CXXFLAGS} memlab.cpp demo2.cpp -o demo2 -lpthread
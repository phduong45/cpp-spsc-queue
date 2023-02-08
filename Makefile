CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
SANITIZERS := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: test clean

main: main.cpp include/bounded_queue.h
	$(CXX) $(CXXFLAGS) -O0 -g main.cpp -o main

test: main.cpp include/bounded_queue.h
	$(CXX) $(CXXFLAGS) $(SANITIZERS) -O1 -g main.cpp -o main
	./main

clean:
	rm -rf main main.dSYM test_spsc test_spsc.dSYM

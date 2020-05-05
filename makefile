CXX = g++

yc : main.cpp lexer.cpp
	$(CXX) -o $@ $^

.PHONY : clean
clean :
	rm -f yc 

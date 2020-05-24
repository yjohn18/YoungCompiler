CXX = clang++-9

yc : main.cpp lexer.cpp parser.cpp tools.cpp abstract_syntax_tree.cpp
	$(CXX) -g `llvm-config-9 --cxxflags --ldflags --system-libs --libs all` -std=c++14 $^ -o $@


.PHONY : clean
clean :
	rm -f yc 

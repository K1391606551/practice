bin= httpserver
src=main.cc
cc=g++
LD_FLAGS=-std=c++11 -lpthread 


$(bin):$(src)
	$(cc) $^ -o $@ $(LD_FLAGS)
.PHONY:clean
clean:
	rm -rf $(bin)
cc=g++
PARSER=parer 
DUG=debug
HTTP_SERVER=server 
.PHONY:all
all:$(PARSER) $(DUG) $(HTTP_SERVER)

$(PARSER):parser.cc 
	$(cc) $^ -o $@ -std=c++11 -lboost_system -lboost_filesystem 
$(DUG):debug.cc 
	$(cc) $^ -o $@ -std=c++11 -ljsoncpp
$(HTTP_SERVER):http_server.cc 
	$(cc) $^ -o $@ -std=c++11 -lpthread -ljsoncpp 

.PHONY:clean
clean:
	rm -f $(PARSER) $(DUG) $(HTTP_SERVER)

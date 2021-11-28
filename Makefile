CC = g++
CFLAGS = -std=c++17 -g -Wall -W

all: ./client/client ./server/server



./server/server: ./server/server.cpp
	mkdir -p ./mail
	$(CC) $(CFLAGS) ./server/server.cpp -o ./bin/server

./client/client: ./client/client.cpp
	$(CC) $(CFLAGS) ./client/client.cpp -o ./bin/client
#-lreadline


clean:
	clear
	rm -rf ./client/*.o ./server/*.o ./bin/server ./bin/client




	

# g++ options:
# -g        debugging information
# -Wall     show all warnings
# -c        compile only, no linking
# -o        specify output

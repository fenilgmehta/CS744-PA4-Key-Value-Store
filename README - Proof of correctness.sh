cat <<< "LISTENING_PORT 12345
SOCKET_LISTEN_N_LIMIT 1024
THREAD_POOL_SIZE_INITIAL 5
THREAD_POOL_GROWTH 1
CLIENTS_PER_THREAD 1
CACHE_SIZE 3" > "KVServer.conf"

make debug
g++ -DDEBUGGING_ON TestingDatabase.cpp KVStoreFileNames_DEBUG.obj -o "a.out"

# ---------------------------------------------------------------------------------------

# Terminal 1
./KVServer_db


# Terminal 2
./KVClient_db req/client_request_011.txt 127.0.0.1 12345
# FILE = "client_request_011.txt"
# 12       
# 2 1 one        ---> put 1:one
# 2 2 two        ---> put 2:two
# 2 3 three      ---> put 3:three
# 2 4 four       ---> put 4:four   (Eviction)
# 2 5 five       ---> put 5:five   (Eviction)
# 1 3            ---> get 3 ---> three (cache hit)
# 1 4            ---> get 4 ---> four  (cache hit)
# 1 3            ---> get 3 ---> three (cache hit)
# 1 5            ---> get 5 ---> five  (cache hit)
# 1 1            ---> get 1 ---> one   (cache miss), 4 should be evicted
# 1 4            ---> get 4 ---> four  (cache miss)
# 1 2            ---> get 2 ---> two   (cache miss)


# Make multiple client requests
aMax=2;
for i in {1..$aMax};
do
	# echo $i
	echo $i ; ./KVClient_db req/client_request_00"${i}".txt 127.0.0.1 12345 req/client_request_00"${i}"_sol.txt <<< "
" &
done



# Prove persistence of data in KVStore by actually opening the database files during the demo
./a.out . send 127.0.0.1 12345 r 1
./a.out . send 127.0.0.1 12345 r 2
./a.out . send 127.0.0.1 12345 r 3
./a.out . send 127.0.0.1 12345 r 4
./a.out . send 127.0.0.1 12345 r 5

# ./a.out . 1
./a.out . r 1
./a.out . r 2
./a.out . r 3
./a.out . r 4
./a.out . r 5


# Read all the database files
./a.out .

# wc -l req/client_request_006.txt
# 5001


# ---------------------------------------------------------------------------------------


# Server Conf - POOR performance
cat <<< "LISTENING_PORT 12345
SOCKET_LISTEN_N_LIMIT 1024
THREAD_POOL_SIZE_INITIAL 2
THREAD_POOL_GROWTH 1
CLIENTS_PER_THREAD 5
CACHE_SIZE 100" > "KVServer.conf"

aMax=5;
for i in {1..$aMax};
do
	echo $i
	time ./KVClient_db req/client_request_005.txt 127.0.0.1 12345 <<< "
	" 2> /dev/null &
done | grep "Time taken by function"




# Server Conf - GOOD performance
cat <<< "LISTENING_PORT 12345
SOCKET_LISTEN_N_LIMIT 1024
THREAD_POOL_SIZE_INITIAL 10
THREAD_POOL_GROWTH 1
CLIENTS_PER_THREAD 1
CACHE_SIZE 100" > "KVServer.conf"

aMax=5;
for i in {1..$aMax};
do
	echo $i
	time ./KVClient_db req/client_request_005.txt 127.0.0.1 12345 <<< "
	" 2> /dev/null &
done | grep "Time taken by function"




# Usage:
# ./a.out . KEY
# ./a.out . w KEY VALUE [HASH]                     WRITE data directly to the database. If HASH is passed, then hash1==hash2
# ./a.out . d KEY [HASH]                           DELETE data directly from the database. If HASH is passed, then hash1==hash2
# ./a.out . r KEY [HASH]                           READ data directly from the database. If HASH is passed, then hash1==hash2
# ./a.out . send IP PORT w KEY VALUE               Make WRITE request to the server
# ./a.out . send IP PORT d KEY                     Make DELETE request to the server
# ./a.out . send IP PORT r KEY                     Make READ request to the server
# ./a.out .                                        This will READ ALL DATABASE entries
# ./a.out DB_FILE_NAME [DB_FILE_NAME [...]]        Read database with name DB_FILE_NAME

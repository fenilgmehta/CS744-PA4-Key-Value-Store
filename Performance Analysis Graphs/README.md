# Performance Analysis

- Server Config
	- LISTENING_PORT 12348
	- SOCKET_LISTEN_N_LIMIT 1024
	- THREAD_POOL_SIZE_INITIAL 10
	- THREAD_POOL_GROWTH 5
	- CLIENTS_PER_THREAD 1
	- CACHE_SIZE 102400
- Total requests per client = 5000

```
+---------+--------------------------+-----------------------+------------------------------------+-----------------------------+
| Clients |        Total Load        | Time Taken in seconds |     Requests served per second     |  Response Time pre request  |
|         |     (Requests sent)      |                       |                                    |      (in Micro seconds)     |
+---------+--------------------------+-----------------------+------------------------------------+-----------------------------+
|    1    |           5000           |         6.019         |     5000 *  1 / 6.019 =   830      |  1 /   830 = 1204 μs        |
|    2    |          10000           |         3.954         |     5000 *  2 / 3.954 =  2529      |  1 /  2529 =  395 μs        |
|    3    |          15000           |         5.194         |     5000 *  3 / 5.194 =  2887      |  1 /  2887 =  346 μs        |
|    4    |          20000           |         4.166         |     5000 *  4 / 4.166 =  4800      |  1 /  4800 =  208 μs        |
|    5    |          25000           |         4.826         |     5000 *  5 / 4.826 =  5180      |  1 /  5180 =  193 μs        |
|    6    |          30000           |         4.170         |     5000 *  6 / 4.170 =  7194      |  1 /  7194 =  139 μs        |
|    7    |          35000           |         4.499         |     5000 *  7 / 4.499 =  7779      |  1 /  7779 =  128 μs        |
|    8    |          40000           |         4.463         |     5000 *  8 / 4.463 =  8962      |  1 /  8962 =  111 μs        |
|    9    |          45000           |         4.465         |     5000 *  9 / 4.465 = 10078      |  1 / 10078 =   99 μs        |
|   10    |          50000           |         3.756         |     5000 * 10 / 3.756 = 13312      |  1 / 13312 =   75 μs        |
+---------+--------------------------+-----------------------+------------------------------------+-----------------------------+
```


### Graphs

![Response Time vs Load - Line Graph](./Response%20Time%20vs%20Load%20-%20line-graph-1.png)
![Response Time vs Load - Scatter Plot](./Response%20Time%20vs%20Load%20-%20scatter-plot-1.png)

![Throughput vs Load - Line Graph](./Throughput%20vs%20Load%20-%20line-graph-2.png)
![Throughput vs Load - Scatter Plot](./Throughput%20vs%20Load%20-%20scatter-plot-2.png)


### Graphs Details
- Throughput vs Load
	- Requests served per second vs Total Load
		- Graph Title    : Throughput vs Load
		- X axis label   : Requests served per second
		- Y axis label   : Total load (i.e. Total requests sent)
		- X1 Y1 X2 Y2... : 830 5000 2529 10000 2887 15000 4800 20000 5180 25000 7194 30000 7779 35000 8962 40000 10078 45000 13312 50000
			- X -> 830 2529 2887 4800 5180 7194 7779 8962 10078 13312
			- Y -> 5000 10000 15000 20000 25000 30000 35000 40000 45000 50000
- Response Time (as seen from the client) vs Load (in requests per second)
	- Response time per request vs Total Load
		- Graph Title    : Response Time (as seen from the client) vs Load
		- X axis label   : Response time per request (in μs)
		- Y axis label   : Total load (i.e. Total requests sent)
		- X1 Y1 X2 Y2... : 1204 5000 395 10000 346 15000 208 20000 193 25000 139 30000 128 35000 111 40000 99 45000 75 50000
			- X -> 1204 395 346 208 193 139 128 111 99 75
			- Y -> 5000 10000 15000 20000 25000 30000 35000 40000 45000 50000
- Website used to generate the graphs
	- https://www.rapidtables.com/tools/scatter-plot.html
	- https://www.rapidtables.com/tools/line-graph.html


```py
>>> # NOTE: refer "Load Analysis - Raw Data.txt" for raw data which is used in this file
>>> # NOTE: the below values are in Micro Seconds
>>> a=[4276428, 3632233]
>>> sum(a) / len(a)
3954330.5

>>> a=[5561647, 5388909, 4633525]
>>> sum(a) / len(a)
5194693.666666667

>>> a=[3343597, 4534050, 3911033, 4878216]
>>> sum(a) / len(a)
4166724.0

>>> a=[3953909, 5270720, 5324497, 4911803, 4672638]
>>> sum(a) / len(a)
4826713.4

>>> a=[3118536, 6200475, 3927475, 3340781, 3965600, 4472541]
>>> sum(a) / len(a)
4170901.3333333335

>>> a=[4814784, 3581401, 2857822, 5413486, 5579736, 4314774, 4935655]
>>> sum(a) / len(a)
4499665.428571428

>>> a=[4688136, 4990316, 2650417, 4794108, 5900487, 3111549, 5517008, 4057840]
>>> sum(a) / len(a)
4463732.625

>>> a=[3920099, 5092111, 4422000, 3688335, 3628910, 5476732, 5508608, 3747831, 4701227]
>>> sum(a) / len(a)
4465094.777777778

>>> a=[3005552, 2655087, 2786605, 2823337, 3345530, 2892465, 4298365, 5765138, 5673981, 4317401]
>>> sum(a) / len(a)
3756346.1
```

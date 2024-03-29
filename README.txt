算法1：
算法分析：
	时间于空间最优考虑：
	   由于数据量可能比价大，放在一个hashmap中单线程处理肯定吃不消，hashmap在数据量增加到10^7之后性能很差。那么改进的方案是，首先把要处理的数据（千万级别x0000000） 分别都 key%100，存到100个集合中然后，使用多线程分别使用hashmap去重并统计出100个集合的key频率出现的最大值（在插入接受后即可得出），然后最后把这100结果进行排序即可得到全局出现频率最大的数字。
	
	时间复杂度：O(n)，hash表使用关键字来计算表位置。当这个表的大小合适，并且计算算法合适的情况下，hash表的算法复杂度为O(1)的，但是这是理想的情况下的，如果hash表的关键字计算与表位置存在冲突，那么最坏的复杂度为O(n)。使用多线程处理的时候，算法最坏情况是所有数据全部相同，都落入一个set,那么多线程蜕变为单线程，如key平均落到不同的set中，那么能充分利用多线程多核心的优势。
	
	空间复杂度：O(n): 空间复杂度为集合中不同的数字的个数，最坏情况下即为n，最好情况下为1如果数据量大于千万级，达到10^10方级别，那么这个一般服务器多核同时处理也是心有余而力不足，那么只能用mapreduce 的方法，将job map到不同的服务器上并行执行，最后reduce再整合结构，其中如果每个集合还是很大，可能	对每个集合不能直接放到内存，那么也只能一部分一部分从持久化存储介质重取出来进行处理。


测试：
测试环境：
	    cpu  :   16核，frequece: 1.6GHZ, cache size: 12288 KB   
	    mem：12G
                    os: ubuntu10.04 
	    kernel:2.6.32-38-generic
测试数据1：
key（范围100以内）								
								
单	时间（ms）		5线程	时间（ms）		10线程	时间（ms）	MC（最大重复次数）
1	0.6			1	0.412			1	0.5		2
2	0.8			2	0.441			2	0.57		4
3	1			3	0.786			3	0.86		18
4	2.56			4	0.786			4	0.762		132
5	16			5	4.2			5	2.81		1068
6	128			6	40			6	18		10245
7	1240			7	254			7	173		100725
8	12349			8	2474			8	1550		1003071
9	123500			9	24971			9	18790		10006908



测试数据2：
1000000	key(范围百万以内)								
单	时间（ms）		5线程	时间（ms）		10线程	时间（ms）	MC（最大重复次数）
1	0.679			1	0.438			1	0.83		1
2	0.563			2	0.663			2	0.841		1
3	0.857			3	0.554			3	0.866		2
4	3.802			4	2.631			4	1.937		2
5	34.415			5	18.709			5	13.779		4
6	271.364			6	123.648			6	88.493		9
7	1506.442		7	385.418			7	243.524		30
8	13116.472		8	3316.828		8	1673.301	153
9	129157.657		9	32716.933		9	15828.889	1146

测试数据2：
全随机	key范围10亿以内								
单线程	时间			5线程	时间（ms）		10线程			MC（最大重复次数）
1	0.792			1	0.587			1	0.506		1
2	0.704			2	0.483			2	0.54		1
3	1.164			3	0.696			3	0.65		1
4	5.628			4	2.65			4	2.248		1
5	39.378			5	18.76			5	14.695		2
6	367.354			6	154.896			6	129.455		2
7	3933.16			7	925.183			7	1291.174	3
8	58748.394		8	19288.351		8	13002.2931	5
9	5941730.63		9	182880.351		9	114046.2279	9

（注：各个测试的曲线图在test.xlsx中）



算法2：分析
	空间最优考虑：
	   如果数据量比较小，那肯定也是直接在内存中使用对数据hashmap处理。插入hashmap结束时，结果也就出来了
	时间复杂度: hashmap的插入复杂度.
	空间复杂度：不重复的key的数目，O（n）
	   如果数据量比较大，可能首先要对数据集合进行外部排序，在排序完后，对排好序的数据集合一部分一部分得分析，
	使用简单得一次遍历即可找到Most common key。
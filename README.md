bloomfilter
===========

bloom filter with expiration

/**
 * 带淘汰的bloom filter
 * 当with_expire_ = true为使用带淘汰版本，此时需要使用2倍大小的内存，存在两块
 * bloom filter: bf1, bf2
 * 	- 初始bf1为空，新数据加入到bf1中
 * 	- 当bf1中数据个数达到thresh1_个时，双写bf1与bf2
 * 	- 当bf1中数据个数达到thresh2_个时，清空bf1，bf2成为新的bf1
 * 
 * Some Code and Though from @upbits
 *
 * 使用：(见test.cpp)
 * 	BloomFilter<int> bf；
 * 	int elem = 1;
 * 	bf.Add(elem);
 * 	bf.In(elem);
 * 	bf.Clear();
 */

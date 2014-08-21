#ifndef __BLOOM_FILTER_H__
#define __BLOOM_FILTER_H__

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

#include <iostream>
#include <math.h>

using namespace std;

#define BYTE_BITS           (8)
#define MIX_UINT64(v)       ((uint32_t)((v >> 32) ^ (v)))

#define SET_BIT(arr, n)   (arr[n/BYTE_BITS] |= (1 << (n % BYTE_BITS)))
#define GET_BIT(arr, n)   (arr[n/BYTE_BITS] & (1 << (n % BYTE_BITS)))

// hash function
///
///  函数FORCE_INLINE，加速执行
///
// MurmurHash2, 64-bit versions, by Austin Appleby
// https://sites.google.com/site/murmurhash/
__attribute__((always_inline)) uint64_t MurmurHash2_x64( const void *key, int len, uint32_t seed)
{
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)key;
	const uint64_t * end = data + (len/8);

	while(data != end)
	{
		uint64_t k = *data++;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h ^= k;
		h *= m;
	}

	const uint8_t * data2 = (const uint8_t*)data;

	switch(len & 7)
	{
	case 7: h ^= ((uint64_t)data2[6]) << 48;
	case 6: h ^= ((uint64_t)data2[5]) << 40;
	case 5: h ^= ((uint64_t)data2[4]) << 32;
	case 4: h ^= ((uint64_t)data2[3]) << 24;
	case 3: h ^= ((uint64_t)data2[2]) << 16;
	case 2: h ^= ((uint64_t)data2[1]) << 8;
	case 1: h ^= ((uint64_t)data2[0]);
	        h *= m;
	};
 
	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

typedef struct filter {
	uint8_t *array;		// 指向分配的bloom filter空间
	uint64_t use_item;	// 当前的数据个数
} Filter;

template<typename T>
class BloomFilter {
	public:
		BloomFilter(uint64_t max_item, bool expire, double prob_false, double thresh1, double thresh2);
		~BloomFilter();

		int Add(const T &key);
		bool In(const T &key);
		void Clear(int clear);
		void Print();
		
	private:
		enum FILTER_TYPE { MAIN_FILTER = 1, AUXI_FILTER = 2 };
		
		uint64_t max_item_;	// 存储的最大数据个数
		double prob_false_;	// 假阳概率
		bool with_expire_;	// true-带淘汰，需要分配两倍的空间
		uint64_t thresh1_;	// 达到阀值1则双写两个filter
		uint64_t thresh2_;	// 达到阀值2则清空该filter，单写另一个
		
		int pos_num_;	// 单个数据占用bit个数
		uint64_t filter_size_;		// bloom filter的大小，按字节
		uint64_t filter_bit_size_;	// bloom filter的大小，按位
		
		Filter filter_[2];		// 使用的bloom filter
		int main_filter_idx_;	// 主bloom filter，辅=!mail_filter_idx_
		
		// 辅助函数
		Filter *_GetFilter(enum FILTER_TYPE type);
		void _CalcBloomFilterParam(uint32_t n, double p, uint64_t *pm, int *pk);
		void _InitBloomFilter(Filter *filter);
		void _AddBloomFilter(Filter *filter, const T &key);
		void _ClearBloomFilter(Filter *filter);
};

template<typename T>
void BloomFilter<T>::_CalcBloomFilterParam(uint32_t n, double p, uint64_t *pm, int *pk)
{
    /**
     *  n - Number of items in the filter
     *  p - Probability of false positives, float between 0 and 1 or a number indicating 1-in-p
     *  m - Number of bits in the filter
     *  k - Number of hash functions
     *
     *  f = ln(2) × ln(1/2) × m / n = (0.6185) ^ (m/n)
     *  m = -1 * ln(p) × n / 0.6185
     *  k = ln(2) × m / n = 0.6931 * m / n
    **/

    uint32_t m, k;

    // 计算指定假阳概率下需要的比特数
    m = (uint32_t) ceil(-1 * log(p) * n / 0.6185);
    m = (m - m % 64) + 64;                  // 8字节对齐

    // 计算哈希函数个数
    k = (uint32_t) (0.6931 * m / n);
    k++;

    *pm = m;
    *pk = k;
    return;
}

template<typename T>
Filter *BloomFilter<T>::_GetFilter(enum FILTER_TYPE type)
{
	if (type == MAIN_FILTER) {
		return &filter_[main_filter_idx_];
	} else if (type == AUXI_FILTER) {
		return &filter_[!main_filter_idx_];
	} else {
		return NULL;
	}

	return NULL;
}

template<typename T>
void BloomFilter<T>::_InitBloomFilter(Filter *filter)
{
	if (filter == NULL)
		return;

	filter->array = (uint8_t *)calloc(1, filter_size_);
	if (filter->array == NULL) {
		cerr << "No Memory Allocate for Bloom Filter" << endl;	
		return;
	}
	filter->use_item = 0;
}

template<typename T>
void BloomFilter<T>::_AddBloomFilter(Filter *filter, const T &key)
{
	uint64_t pos;
    uint64_t hash1 = MurmurHash2_x64((const void *)&key, sizeof(key), 0);
    uint64_t hash2 = MurmurHash2_x64((const void *)&key, sizeof(key), MIX_UINT64(hash1));
    
    for (int i = 0; i < pos_num_; i++)
    {
		pos = (hash1 + i * hash2) % filter_bit_size_;
		SET_BIT(filter->array, pos);
    }
	filter->use_item++;

    return;
}

template<typename T>
void BloomFilter<T>::_ClearBloomFilter(Filter *filter)
{
	if (filter == NULL)
		return;
	memset(filter->array, 0, filter_size_);
	filter->use_item = 0;
}



// 主体
template<typename T>
BloomFilter<T>::BloomFilter(uint64_t max_item = 100000, 
		bool expire = false, 
		double prob_false = 0.00001,
		double thresh1 = 0.5, 
		double thresh2 = 0.8)
	: max_item_(max_item), with_expire_(expire), prob_false_(prob_false),  
	thresh1_(uint64_t(max_item * thresh1)), thresh2_(uint64_t(max_item * thresh2))
{
	Filter *filter;
	
	// 计算需要的bit位数和每个数据占用的bit个数
	_CalcBloomFilterParam(max_item_, prob_false_, &filter_bit_size_, &pos_num_);
	filter_size_ = filter_bit_size_ / BYTE_BITS;

	// 初始化0为主filter
	main_filter_idx_ = 0;
	
	// 分配bloom filter空间
	filter = _GetFilter(MAIN_FILTER);
	_InitBloomFilter(filter);
	
	// 带淘汰时使用第二块filter
	if (with_expire_) {
		filter = _GetFilter(AUXI_FILTER);
		_InitBloomFilter(filter);
	}

#ifndef USING_DEPLOY_ENV
	cout << ">> Bloom Filter: Size=" << filter_size_ << "bytes, MaxItem=" << 
		max_item_ << ", Accurate=" << prob_false_ << ", WithExpire=" 
		<< with_expire_ << endl;
#endif
}

template<typename T>
BloomFilter<T>::~BloomFilter()
{
	Filter *f = _GetFilter(MAIN_FILTER);
	free(f->array);
	f->array = NULL;
	f->use_item = 0;

	if (with_expire_) {
		Filter *filter = _GetFilter(AUXI_FILTER);
		free(filter->array);
		filter->array = NULL;
		filter->use_item = 0;
	}
}

template<typename T>
int BloomFilter<T>::Add(const T &key)
{
	Filter *main_filter = _GetFilter(MAIN_FILTER);
	
	// 添加到主filter
	_AddBloomFilter(main_filter, key);
	
	if (main_filter->use_item >= thresh1_) {
		// 带淘汰时添加到辅filter
		if (with_expire_) {	
			_AddBloomFilter(_GetFilter(AUXI_FILTER), key);
		}
	}

	if (main_filter->use_item >= thresh2_) {
		/**
		 * if: 带淘汰 切换主辅filter，并清空旧主filter
		 * else: 不带淘汰 告警
		 *
		 * */
		if (with_expire_) {
			// 交换主辅bloom filter
			main_filter_idx_ = !main_filter_idx_;
		
			// 清空旧主bloom filter
			_ClearBloomFilter(main_filter);
			
#ifndef USING_DEPLOY_ENV
			cout << "Switch Bloom Filter" << endl;
#endif
		} else {
			cout << "Bloom Filter Reach " << thresh2_ * 100.0 / max_item_ << "%" << endl;
		}
	}

	return 0;
}

template<typename T>
bool BloomFilter<T>::In(const T &key)
{
	uint64_t pos;
	Filter *filter;
    uint64_t hash1 = MurmurHash2_x64((const void *)&key, sizeof(key), 0);
    uint64_t hash2 = MurmurHash2_x64((const void *)&key, sizeof(key), MIX_UINT64(hash1));
    
    for (int i = 0; i < pos_num_; i++)
    {
		pos = (hash1 + i * hash2) % filter_bit_size_;
		filter = _GetFilter(MAIN_FILTER);
		if (GET_BIT(filter->array, pos) == 0)
			return false;
    }

	return true;
}

template<typename T>
void BloomFilter<T>::Clear(int clear)
{
	_ClearBloomFilter(_GetFilter(MAIN_FILTER));
	if (with_expire_) {
		_ClearBloomFilter(_GetFilter(AUXI_FILTER));
	}
}

template<typename T>
void BloomFilter<T>::Print()
{
	cout << "Bloom Filter Info:" << endl;
	cout << "  Max Item: " << max_item_ << endl;
	cout << "  With-Expiration: " << with_expire_ << endl;
	cout << "  Thresh(Double-Write): " << thresh1_ << endl;
	cout << "  Thresh(Expire): " << thresh2_ << endl;
	cout << "  Data Occupy " << pos_num_ << " bits" << endl;

	Filter *filter = _GetFilter(MAIN_FILTER);
	cout << "  Current use [" << main_filter_idx_ << "] Filter" <<endl;
	cout << "   - Use " << filter->use_item << " Item" << endl;
	
	if (with_expire_) {
		filter = _GetFilter(AUXI_FILTER);
		cout << "  Auxiliary use [" << !main_filter_idx_ << "] Filter" <<endl;
		cout << "   - Use " << filter->use_item << " Item" << endl;

	}
}

#endif


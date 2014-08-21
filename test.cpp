#include <iostream>

#include "bloom_filter.h"

using namespace std;

int main()
{
	BloomFilter<int> bf;

	int elem1 = 1;
	bf.Add(elem1);
	cout << "Adding " << elem1 << endl;
	int elem2 = 2;
	bf.Add(elem2);
	cout << "Adding " << elem2 << endl;
	int elem3 = 3;
	bf.Add(elem3);
	cout << "Adding " << elem3 << endl;
	int elem4 = 4;
	bf.Add(elem4);
	cout << "Adding " << elem4 << endl;

	cout << elem1 << " is in BloomFilter: " << bf.In(1) << endl;
	cout << elem2 << " is in BloomFilter: " << bf.In(2) << endl;
	cout << elem3 << " is in BloomFilter: " << bf.In(3) << endl;
	cout << elem4 << " is in BloomFilter: " << bf.In(4) << endl;

	bf.Print();

	int elem5 = 5;
	bf.Add(elem5);
	cout << "Adding " << elem5 << endl;
	
	bf.Print();

	int elem6 = 6;
	bf.Add(elem6);
	cout << "Adding " << elem6 << endl;
	
	int elem7 = 7;
	bf.Add(elem7);
	cout << "Adding " << elem7 << endl;
	
	bf.Print();
	
	int elem8 = 8;
	bf.Add(elem8);
	cout << "Adding " << elem8 << endl;

	bf.Print();
	
	int elem9 = 9;
	bf.Add(elem9);
	cout << "Adding " << elem9 << endl;

	bf.Print();

	cout << elem1 << " is in BloomFilter: " << bf.In(elem1) << endl;
	cout << elem2 << " is in BloomFilter: " << bf.In(elem2) << endl;
	cout << elem3 << " is in BloomFilter: " << bf.In(elem3) << endl;
	cout << elem4 << " is in BloomFilter: " << bf.In(elem4) << endl;
	
	return 0;
}


#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>
#include <cstddef>
using namespace std;

class Block
{
private:
	int number_cnt;
	vector <int> Numbers;
public:
	int current_time;
	unsigned int tag;
	int valid;
	int dirty;
	Block(int block_size){
				dirty = tag = current_time = valid = 0;
				number_cnt = block_size/4;
				Numbers.reserve(number_cnt);

	}

	int read_number(unsigned int addr){
		unsigned int offset = addr & (number_cnt-1);
		valid = 1;
		current_time = -1;
		return Numbers[offset];
	}
	vector <int> read_Numbers(unsigned int addr){
		valid = 1;
		current_time = -1;
		return Numbers;
	}
	void write_num(unsigned int addr, int data){
		unsigned int offset = addr & (number_cnt-1);
		Numbers[offset] = data;
		valid = 1;
		current_time = -1;
	}
	void write_nums(unsigned int addr, vector <int> data){
		Numbers = data;
		valid = 1;
		current_time = -1;
	}

};


class Set
{
private:
	int block_cnt;
	int offsetwidth;
	int indexwidth;
	int tagwidth;
	vector <Block> Blocks;
public:
	Set(int cache_size, int associativity, int block_size){
		block_cnt = associativity;
		offsetwidth = log2(block_size);
		indexwidth = log2(cache_size/(block_size*associativity));
		tagwidth = 32 - offsetwidth - indexwidth;

		int tempsum = 0;
		for (int i=0; i<=tagwidth-1; i++)
		{
			tempsum = tempsum + pow(2,i);
		}
		tagwidth = tempsum +1;

		for (int i=0; i <= (block_cnt-1); i++)
			Blocks.push_back(Block(block_size));
	}

	unsigned int LRUblock(void){
		unsigned int z;
		int x = Blocks[0].current_time;
		for (int i=0; i <= (block_cnt-1); i++)
		{
			if (Blocks[i].current_time > x)
			{
				x = Blocks[i].current_time;
				z = i;
			}
		}
		return z;
	}
	Block * read_block(unsigned int addr){
		unsigned int tag = ((addr >> offsetwidth) >> indexwidth) & (tagwidth-1);
		int i = 0;
		while (i <= (block_cnt-1))
		{
			if ((tag == Blocks[i].tag) && (Blocks[i].valid == 1))
			{
				Blocks[i].read_number(addr);
				return &Blocks[i];
				break;
			}
			i++;
		}
		return NULL;
	}
	Block * write_block(unsigned int addr, vector <int> data){
		unsigned int y = LRUblock();
		unsigned int tag = ((addr >> offsetwidth) >> indexwidth) & (tagwidth-1);
		Block * temp = &Blocks[y];
		Blocks[y].write_nums(addr, data);
		Blocks[y].tag = tag;
		return temp;
	}

	Block * write_through(unsigned int addr, int data){
		unsigned int tag = ((addr >> offsetwidth) >> indexwidth) & (tagwidth-1);
		int i = 0;
		while (i <= (block_cnt-1))
		{
			if ((tag == Blocks[i].tag) && (Blocks[i].valid == 1))
			{
				Blocks[i].write_num(addr,data);
				return &Blocks[i];
				break;
			}
			i++;
		}
		return NULL;
	}
	Block * write_back(unsigned int addr, int data){
		unsigned int tag = ((addr >> offsetwidth) >> indexwidth) & (tagwidth-1);
		int i = 0;
		while (i <= (block_cnt-1))
		{
			if ((tag == Blocks[i].tag) && (Blocks[i].valid == 1))
			{
				Blocks[i].write_num(addr,data);
				Blocks[i].dirty = 1;
				return &Blocks[i];
				break;
			}
			i++;
		}
		return NULL;
	}
	void timestampUpdate(){
		for (int i=0; i <= (block_cnt-1); i++)
			Blocks[i].current_time++;
	}
};


class Cache
{
private:
	int cache_size;
	int associativity;
	int block_size;
	int count_set;
	int offsetwidth;
	vector <Set> Sets;
public:
	Cache(int a, int b, int c){
		cache_size = a;
		associativity = b;
		block_size = c;
		count_set = cache_size/(associativity*block_size);
		offsetwidth = log2(block_size);
		for (int i=0; i <= (count_set-1); i++)
			Sets.push_back(Set(cache_size, associativity, block_size));
	}

	Block * read_block(unsigned int addr){
		unsigned int index = (addr >> offsetwidth) & (count_set-1);
		return Sets[index].read_block(addr);
	}

	void timestampUpdate(){
		for (int i=0; i <= (count_set-1); i++)
			Sets[i].timestampUpdate();
	}

	Block * write_block(unsigned int addr, vector <int> data){
		unsigned int index = (addr >> offsetwidth) & (count_set-1);
		return Sets[index].write_block(addr, data);
	}

	Block * write_through(unsigned int addr, int data){
		unsigned int index = (addr >> offsetwidth) & (count_set-1);
		return Sets[index].write_through(addr, data);
	}

	Block * write_back(unsigned int addr, int data){
		unsigned int index = (addr >> offsetwidth) & (count_set-1);
		return Sets[index].write_back(addr, data);
	}

};


unsigned int atoh(string currinsaddr)
{
	unsigned int addr;
	stringstream ss;
	ss << hex << currinsaddr;
	ss >> addr;
	return addr;
}


int main(int argc, char * argv[])
{
	ifstream trace_file (argv[1]);
	int cache_size, associativity, block_size, write_policy, victim_cache;
	Cache L1 (cache_size, associativity, block_size);
	Cache Victim_cache (block_size*victim_cache, victim_cache, block_size);
	cout << "Cache Size:" ;
	cin >> cache_size ;
	cout << "Block Size:";
	cin >> block_size ;
	cout << "Write Policy:";
	cin >> write_policy ;
	cout << "Victim Cache BlockCount:";
	cin >> victim_cache;
	int assoc;
	cout << "Associativity:1 for Direct Mapped,2 for Two Way, 3 for Fully associative:";
	cin >> assoc;
 	(assoc == 3) ? associativity = cache_size/block_size:associativity = assoc;


	string currins, currdata, currinsaddr;
	int data;
	int addr;
	int tot_access = 0;
	int tot_read_l1, tot_read_victim, tot_write_l1, tot_write_victim;
	int hit_read_l1 = 0, miss_read_l1 = 0, hit_write_l1 = 0, miss_write_l1 = 0;
	int hit_read_victim = 0, miss_read_victim = 0, hit_write_victim = 0, miss_write_victim = 0;
	int memtocache = 0, cachetomem = 0;
	double missL1 = 0, missvictim = 0;

	while ( trace_file.good() )
	{
		tot_access++;
		currins = currdata = currinsaddr = "";
		trace_file >> currins;
		trace_file >> currinsaddr;
		addr = atoh(currinsaddr);


		if ((currins != "") && (currinsaddr != ""))
		{
			if (currins == "r")
			{
				if (L1.read_block(addr) != NULL)
				{
					hit_read_l1++;
				}
				else if (Victim_cache.read_block(addr) != NULL)
				{
					miss_read_l1++;
					hit_read_victim++;
					Block * temp = L1.write_block(addr,(*Victim_cache.read_block(addr)).read_Numbers(addr));
					(*Victim_cache.read_block(addr)).write_nums(addr,(*temp).read_Numbers(addr));
				}
				else
				{
					miss_read_l1++;
					miss_read_victim++;
					vector <int> dummy (block_size/4);
					dummy[(addr & ((block_size/4)-1))] = data;
					//From Memory - 1 block
					memtocache++;
					Block * temp = L1.write_block(addr,dummy);
					Block * temp1 = Victim_cache.write_block(addr,(*temp).read_Numbers(addr));
					if ((*temp1).dirty == 1)
					{
						cachetomem++;
					}
					else if (write_policy == 2)
					{
						if ((*temp1).valid == 1)
						{
							cachetomem++;
						}
					}
				}
			}
			else if (currins == "w")
			{
				if (write_policy == 1)
				{
					if (L1.write_back(addr,data) != NULL)
					{
						hit_write_l1++;
					}
					else if (Victim_cache.write_back(addr,data) != NULL)
					{
						miss_write_l1++;
						hit_write_victim++;
						Block * temp = L1.write_block(addr,(*Victim_cache.read_block(addr)).read_Numbers(addr));
						(*Victim_cache.read_block(addr)).write_nums(addr,(*temp).read_Numbers(addr));
					}
					else
					{
						miss_write_l1++;
						miss_write_victim++;
						vector <int> dummy (block_size/4);
						dummy[(addr & ((block_size/4)-1))] = data;
						cachetomem++;
						Block * temp = L1.write_block(addr,dummy);
						Block * temp1 = Victim_cache.write_block(addr,(*temp).read_Numbers(addr));
						if ((*temp1).dirty == 1)
						{
							cachetomem++;
						}
						else if (write_policy == 2)
						{
							if ((*temp1).valid == 1)
							{
								cachetomem++;
							}
						}
					}
				}
				else if (write_policy == 2)
				{
					if (L1.write_through(addr,data) != NULL)
					{
						hit_write_l1++;
						cachetomem++;
					}
					else if (Victim_cache.write_through(addr,data) != NULL)
					{
						miss_write_l1++;
						hit_write_victim++;
					}
					else
					{
						miss_write_l1++;
						miss_write_victim++;
						cachetomem++;
					}
				}
				else
					cout << "Error! Wrong Format!!";
			}
			else
			{
				cout << "Error! Wrong Format";
				return 0;
			}
		}
		else
			tot_access--;
		L1.timestampUpdate();
		Victim_cache.timestampUpdate();
	}
		trace_file.close();

	tot_read_l1 = hit_read_l1 + miss_read_l1;
	tot_write_l1 = hit_write_l1 + miss_write_l1;
	tot_read_victim = hit_read_victim + miss_read_victim;
	tot_write_victim = hit_write_victim + miss_write_victim;
	missL1 = (double) (miss_read_l1 + miss_write_l1) / (tot_read_l1 + tot_write_l1);
	missvictim =  (double) (miss_read_victim + miss_write_victim) / (tot_read_victim + tot_write_victim);

	cout << "Total count of Cache access: " << tot_access << endl ;
	cout << "Total Reads in L1 cache:" << tot_read_l1 << endl ;
	cout << "Hits occured:" << hit_read_l1 << endl;
	cout << "Misses occured: " << miss_read_l1 << endl;
	cout << "Writes in L1: " << tot_write_l1 << endl;
	cout << "Hits in L1: " << hit_write_l1 << endl;
	cout << "Misses in L1: " << miss_write_l1 << endl;
	cout << "Miss Rate: " << missL1 << endl;
	cout << "Victim Cache Reads: " << tot_read_victim << endl;
	cout << "Hits : " << hit_read_victim << endl;
	cout << "Misses: " << miss_read_victim << endl;
	cout << "Victim Writes: " << tot_write_victim << endl;
	cout << "Hits: " << hit_write_victim << endl;
	cout << "Misses: " << miss_write_victim << endl;
	cout << "Victim Miss Rate: " << missvictim << endl;
	cout << "Bytes Transfered from Mem to Cache: " << memtocache*block_size << endl ;
	cout << "Bytes Transfered from Cache to Mem: " << cachetomem*block_size << endl;
	return 0;
}

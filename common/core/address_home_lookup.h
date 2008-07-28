#ifndef ADDRESSHOMELOOKUP_H_
#define ADDRESSHOMELOOKUP_H_

class AddressHomeLookup
{
public:
	AddressHomeLookup(unsigned int, unsigned int);
	virtual ~AddressHomeLookup();

	/* TODO: change this return type to a node number type */
	unsigned int find_home_for_addr(unsigned int) const;
private:
	unsigned int total_mem_size_bytes;
	unsigned int num_nodes;
	unsigned int cache_lines_per_node;
};

#endif /*ADDRESSHOMELOOKUP_H_*/

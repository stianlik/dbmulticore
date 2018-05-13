#include<iostream>

ostream& operator <<(ostream& stream, const Tuple &tuple) {
	stream << tuple;
	return stream;
}

ostream& operator <<(ostream& stream, const TuplePair &tuple) {
	stream << tuple.left << tuple.right;
	return stream;
}

ostream& operator <<(ostream& stream, const Part &tuple) {
	stream << tuple.partkey << "|" 
		<< tuple.name << "|" 
		<< tuple.mfgr << "|" 
		<< tuple.brand << "|" 
		<< tuple.type << "|" 
		<< tuple.size << "|" 
		<< tuple.container << "|" 
		<< tuple.retailprice << "|" 
		<< tuple.comment << "|";
	return stream;
}

ostream& operator <<(ostream& stream, const PartSupp &tuple) {
	stream << tuple.partkey  << "|"
		<< tuple.suppkey << "|"  
		<< tuple.availqty << "|"
		<< tuple.supplycost << "|"
		<< tuple.comment << "|";
	return stream;
}


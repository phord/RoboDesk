#include<iostream>
#include <iomanip>
#include <fstream>
#include "LogicData.h"

using namespace std;
LogicData ld(0);
unsigned long NOW = 0;

int main(int argc, char **argv) {
	ifstream fin;
	fin.open(argv[1]);
	
	// skip first line (t v markers)
	fin.ignore(100,'\n');

	while (fin) {
		double t=0, v=0;
		fin >> t ;
		fin >> v;
		NOW = t * 1000;
		ld.PinChange(v > 1.2);
		//cout << (v>1.2 ? '-' : '_') ;
		uint32_t x = ld.ReadTrace();
		if (x) cout << hex << x << endl;
	}
	fin.close();
	cout << endl;
	return 0;
}

#include<iostream>
#include<cstdlib>
#include<ctime>
#include<cstring>

using namespace std;
int main(int argc, char* argv[]) {
	srand(time(NULL));

	if (argc < 3) {
		cout << "./gen rows cols [ skew | uniform ]" << endl;
		exit(1);
	}

	int rows = atoi(argv[1]);
	int cols = atoi(argv[2]);

	if (argc >= 3 && strcmp(argv[3],"skew") == 0) {
		for (int i = rows; i > 0; --i) {
			for (int j = 0; j < cols; ++j) {
				cout << i << '|';
			}
			cout << endl;
		}
	}
	else if (argc >= 3 && strcmp(argv[3],"acorr") == 0) {
		int x = rows;
		for (int i = 1; i <= rows; ++i) {
			cout << i << '|' << x-- << '|' << endl;
		}
	}
	else {
		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				cout << rand() << '|';
			}
			cout << endl;
		}
	}

	return 0;
}

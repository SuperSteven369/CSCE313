#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;  

int buffer = 0;
Semaphore Adone (0);
Semaphore Cdone (1);
Semaphore Bdone (0);
Semaphore mtx (1); 
int bdone = 0; 
	
void Afunc(int pno) {
	while (true) {
		Cdone.P();
		mtx.P();
		buffer ++;
		cout << ">>>>>>>>>>>>>>>>>>>>A [" <<pno<<"] got <<<<<<<<<<" << buffer << endl;
		mtx.V();
		mtx.P();
		Adone.V();
		Adone.V();
		mtx.V();
	}
}

void Bfunc(int cno) {
	while (true) {
		Adone.P();
		mtx.P();
		cout << ">>>>>>>>>>>>>>>>>>>>B [" <<cno<<"] got <<<<<<<<<<" << buffer << endl;
		mtx.V();
		usleep (500000);
		mtx.P();
		bdone++;
		if (bdone == 2) { 
			bdone = 0;
			for (int i = 0; i < 1; i++)
				Bdone.V();
		}
		mtx.V();
	}
}

void Cfunc(int cno) {
	while (true) {
		Bdone.P();
		mtx.P();
		cout << ">>>>>>>>>>>>>>>>>>>>C [" <<cno<<"] got <<<<<<<<<<" << buffer << endl;
		mtx.V();
		mtx.P();
		Cdone.V();
		mtx.V();
	}
}
int main() {
	vector<thread> A;
	vector<thread> B;
	vector<thread> C; 

	for (int i = 0; i < 100; i++)
		A.push_back(thread (Afunc, i + 1));

	for (int i = 0; i < 100; i++)
		B.push_back(thread (Bfunc, i + 1));

	for (int i = 0; i < 100; i++)
		C.push_back(thread (Cfunc, i + 1));

	for (int i = 0; i <A.size (); i++)
		A [i].join();
	
	for (int i = 0; i < B.size (); i++)
		B [i].join();
		
	for (int i = 0; i < C.size (); i++)
		C [i].join();
}


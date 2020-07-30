#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;

int buffer = 0;
Semaphore consumerdone (1);
Semaphore producerdone (0);
Semaphore mtx (1); // will use as mutex
int cdone = 0; // number of consumers done consuming

// each producer gets an id, which is pno	
void producer_function(int pno) {
	int count = 0; // each producer threads has its own count
	while (true){
        consumerdone.P();
		buffer ++;
		cout << "Producer [" << pno << "] left buffer=" << buffer << endl;
		producerdone.V();
    }
}
// each consumer gets an id cno
void consumer_function(int cno) {
	while (true){
		//do necessary wait
		producerdone.P();
		cout << ">>>>>>>>>>>>>>>>>>>>Consumer [" <<cno<<"] got <<<<<<<<<<" << buffer << endl;
		usleep (500000);
        consumerdone.V();
	}
}
int main (){
	vector<thread> producers;
	vector<thread> consumers;

	for (int i=0; i< 100; i++)
		producers.push_back (thread (producer_function, i+1));

	for (int i=0; i< 300; i++)
		consumers.push_back(thread (consumer_function, i+ 1));

	
	for (int i=0; i<consumers.size (); i++)
		consumers [i].join();
	
	for (int i=0; i<producers.size (); i++)
		producers [i].join();
	
}


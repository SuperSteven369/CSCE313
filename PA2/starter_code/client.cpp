/*
    Author: Siyuan Yang
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/21/20
 */

#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <iostream>
using namespace std;

int main(int argc, char *argv[]){
    srand(time_t(NULL));
    // Initialize the variables
    int opt;
    int patient = -1;
    int ecg = -1;
    int buffercapacity = MAX_MESSAGE;
    double time = -1.0;
    char* filename;
    bool REQUEST_FILE = false;
    bool REQUEST_DATA = false;
    bool CREATE_CHANNEL = false;

    // Run the server as a child process
    if (fork() == 0) {
        char* argv[] = {"./server", NULL};
        execv(argv[0], argv);        
    } else {
        while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != EOF) {
            switch(opt) {
                case 'p': // patient
                    patient = atoi(optarg);
                    REQUEST_DATA = true;
                    break;
                case 't': // time
                    time = atof(optarg);
                    REQUEST_DATA = true;
                    break;
                case 'e': // ecg number
                    ecg = atoi(optarg);
                    REQUEST_DATA = true;
                    break;
                case 'f': // file
                    filename = optarg;
                    REQUEST_FILE = true;
                    break;
                case 'c': // channel
                    CREATE_CHANNEL = true;
                    break;
                case 'm': // capacity
                    buffercapacity = atoi(optarg);
                    break;
                default:
                    break;
            }
        }
    
        FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
        
        // Requesting Data Points
        if (REQUEST_DATA) {
            if (time <= 0 && ecg <= 0) {
                if (patient <= 0) patient = 1;
                time = 0;
                ofstream outfile; // create the output file
                outfile.open("received/x" + to_string(patient) + ".csv"); // put under the received folder
                timeval start, end;
                gettimeofday(&start, NULL); // start timing
                while (time < 59.996) { // Data types is double with range [0.00, 59.996]
                    outfile << time << ",";
                    // message 1
                    datamsg msg1(patient, time, 1); // initialize message 1 with ecg1
                    chan.cwrite(&msg1, sizeof(msg1)); // cwrite
                    char* buffer1 = new char [buffercapacity]; // cread
                    int nbytes1 = chan.cread(buffer1, buffercapacity); // cread
                    outfile << *((double*)buffer1) << ","; // output data to the file
                    // message 2
                    datamsg msg2(patient, time, 2); // initialize message 2 with ecg2
                    chan.cwrite(&msg2, sizeof(msg2)); // cwrite
                    char* buffer2 = new char [buffercapacity]; // cread
                    int nbytes2 = chan.cread(buffer2, buffercapacity); // cread
                    outfile << *((double*)buffer2) << endl; // output data to the file
                    time += 0.004; // collected every 4ms
                }   
                gettimeofday(&end, NULL); // end timing
                outfile.close(); // close the output file
                double timeDiff1 = end.tv_sec - start.tv_sec; // calculate time taken for requesting data
                cout << "Time Taken For Requesting Data: " << timeDiff1  << setprecision(6) << " seconds" << endl;  
            } else {
                datamsg msg3(patient, time, ecg); // initialize message 3
                chan.cwrite(&msg3, sizeof(msg3)); // cwrite
                char* buffer3 = new char [buffercapacity]; // cread
                int nbytes3 = chan.cread(buffer3, buffercapacity); // cread
                cout << "Requested Data Point: " << *((double*)buffer3) << endl;
            }
        } 

        // Requesting Files
        if (REQUEST_FILE) {
            // File name either with the extension .dat or .csv
            string str(filename);
            string extension = str.substr(str.find_last_of('.') + 1);
            string file_name;
            if (extension == "dat") {
                file_name = "received/" + str;
            } else {
                file_name = "received/y1.csv";
            }
            int fd = open(file_name.c_str(),O_WRONLY|O_CREAT, 0666);
            filemsg file_msg(0, 0); // send a special file message setting to zero
            char* mbuf = new char[sizeof(file_msg) + sizeof(filename) + 1]; // using offset
            memcpy(mbuf, &file_msg, sizeof(file_msg)); // copy memory
            memcpy(mbuf + sizeof(file_msg), filename, sizeof(filename) + 1); // copy memory
            chan.cwrite(mbuf, sizeof(file_msg) + sizeof(filename) + 1); // cwrite
            char* buffer4 = new char [buffercapacity]; // cread
            int nbytes4 = chan.cread(buffer4, buffercapacity); // cread
            __int64_t length = *(__int64_t*)buffer4; // sends back the length of the file
            // initialize variables, would be used later in the while loop
            int i = 0;
            int capacity = MAX_MESSAGE;
            timeval start, end;
            gettimeofday(&start, NULL); // start timing
            while (length > i) {
                if (length < capacity + i) capacity = length - i;
                filemsg file_msg(i, capacity); // send a file message with MAX_MESSAGE capacity
                memcpy(mbuf, &file_msg, sizeof(file_msg)); // copy memory
                memcpy(mbuf + sizeof(file_msg), filename, sizeof(filename) + 1); // copy memory
                chan.cwrite(mbuf, sizeof(file_msg) + sizeof(filename) + 1); // cwrite
                char* buffer5 = new char [buffercapacity]; // cread
                int nbytes5 = chan.cread(buffer5, buffercapacity); // cread
                write(fd, buffer5, sizeof(char)*capacity); // write
                i += capacity; // add capacity to i
            }
            gettimeofday(&end, NULL); // end timing
            close(fd); // close the file
            double timeDiff2 = end.tv_sec - start.tv_sec; // calculate time taken for requesting files
            if (timeDiff2 == 0) { // if too small
                timeDiff2 = end.tv_usec - start.tv_usec; // calculate in microsecond
                cout << "Time Taken For Requesting Files: " << timeDiff2  << setprecision(6)<< " microseconds" << endl;
            } else {
                cout << "Time Taken For Requesting Files: " << timeDiff2 << " seconds" << endl;
            }
        }

        // Creating A New Channel
        if (CREATE_CHANNEL) {
            MESSAGE_TYPE new_channel = NEWCHANNEL_MSG; // create a new channel
            chan.cwrite(&new_channel, sizeof(MESSAGE_TYPE)); // cwrite
            char* buffer6 = new char [buffercapacity]; // cread
            int nbytes6 = chan.cread(buffer6, buffercapacity); // cread
            FIFORequestChannel chan2(buffer6, FIFORequestChannel::CLIENT_SIDE); // open the new channel
            datamsg msg(1, 59.00 , 2); // send a message
            chan2.cwrite(&msg, sizeof(msg)); // cwrite
            char* buffer7 = new char [buffercapacity]; // cread
            int nbytes7 = chan2.cread(buffer7, buffercapacity); // cread
            cout << "Test Result @new_channel: " <<  *((double*)buffer7) << endl; // output to screen the sent message
            MESSAGE_TYPE m = QUIT_MSG; // clean up the resources
            chan.cwrite(&m, sizeof(MESSAGE_TYPE)); // cwrite
        }

        // closing the channel    
        MESSAGE_TYPE m = QUIT_MSG; 
        chan.cwrite(&m, sizeof (MESSAGE_TYPE));
        wait(NULL);
    }
}
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

string ltrim(string str, const string chars = "\t\n\v\f\r ") {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
string rtrim(string str, const string chars = "\t\n\v\f\r ") {
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}
string trim(string str, const string chars = "\t\n\v\f\r ") {
    return ltrim(rtrim(str, chars), chars);
}
// split the line by the given symbol that gives us separate commands
vector<string> split(string commandline, string delim) {
    vector<string> outputline;
    int pos = 0;
    int qmark = 0;
    int dmark = 0;
    while (commandline.size()) {
        int found = commandline.find(delim, pos);
        if (found == string::npos) {
            string lastpart = trim(commandline);
            if (lastpart.size() != 0) outputline.push_back(lastpart);
            break;            
        }
        for (int i = pos; i < found; i++) {
            if (commandline[i] == '\"') {
                dmark++;
            } else if (commandline[i] == '\'') {
                qmark++;
            }
        }
        bool ONLY_QMARK = (qmark%2 == 0) && (qmark != 0);
        bool ONLY_DMARK = (dmark%2 == 0) && (dmark != 0);
        bool NO_MARK = (qmark == 0) && (dmark == 0);
        if (ONLY_QMARK || ONLY_DMARK || NO_MARK) {
            string temp = trim(commandline.substr(0, found));
            commandline = commandline.substr(found + 1);
            if (temp.size() != 0) outputline.push_back(temp);
        } else {
            pos = found + 1;
        }
    }
    return outputline;
}
// remove the chars from the string
void clear(string& str, string chars) {
    for (char c: chars) {
        str.erase(remove(str.begin(), str.end(), c), str.end());
    }
}
// parse and execute the command line
void execute(string commandline) {
    vector<string> cmdVec = split(commandline, " "); 
    int size = cmdVec.size();
    for (int i = 0 ; i < size ; i++) {
        string chars = "\"\'";
        clear(cmdVec[i], chars);
    }
    char** cmdChar = new char* [size + 1];
    for (int i = 0; i < size; i++) {
        cmdChar[i] = new char[cmdVec[i].size() + 1];
        strcpy(cmdChar[i], cmdVec[i].c_str());
    }
    cmdChar[size] = NULL;
    char** args = cmdChar;
    if (execvp(args[0], args) == -1) {
        cerr << "Command not found" << endl;
    }
}
int main() {
    char cwd[10000];
    vector<string> PATH;
    vector<int> pidList; // pid of each background process in a list
    string white = "\033[0m";
    string green = "\033[1m\033[32m";
    string blue = "\033[1m\033[34m";
    string red = "\033[1m\033[31m";
    int stdin = dup(0);
    int stdout = dup(1);
    while (true) {
        time_t now = time(0);
        char* dt = ctime(&now);
        cout << green << "SuperSteven Shell" << white << ":";
        cout << blue << getcwd(cwd, sizeof(cwd)) << white;
        cout << red << " " << dt << white << "$ ";
        string inputline;
        getline (cin, inputline); // get a line from standard input
        vector<string> cmdline = split(inputline, " ");
        if (cmdline[0] == "cd") {
            getcwd(cwd, sizeof(cwd)); // get current working directory
            if (cmdline[1] == "-") {
                chdir(PATH.back().c_str());
            } else {
                chdir(cmdline[1].c_str());
            }
            PATH.push_back(string(cwd));
        } else if (cmdline[0] == "exit" || cmdline[0] == "quit") {
            cout << "Bye!! End of SuperSteven Shell" << endl;
            break;
        }  
        else {
            vector<string> levels = split(inputline, "|"); // PIPE
            for (int i = 0; i < levels.size(); i++) {
                // set up the pipe
                int fd[2];
                pipe(fd);
                int pid = fork();
                bool background_process = false; // set the background process to false
                size_t foundAMP = levels[i].find('&'); //AMP
                if (foundAMP != string::npos) { // dealing with Ampersand
                    background_process = true;
                    pidList.push_back(pid);
                    clear(levels[i], "&");
                }
                if (pid == 0) { // in the child process
                    // 1. redirect the output to the next level
                    bool REDIRECT = false;
                    vector<string> checkout = split(levels[i], ">");
                    vector<string> checkin = split(levels[i], "<");
                    if (checkout.size() > 1 || checkin.size() > 1) REDIRECT = true;
                    cmdline = split(levels[i], " ");
                    if (cmdline[0] == "jobs") execute("false"); // handling the command "jobs"
                    if (REDIRECT) {
                        vector<string> redirList;
                        for (char c: levels[i]) {
                            if (c == '<') {
                                redirList.push_back("input");
                            } else if (c == '>') {
                                redirList.push_back("output");
                            }
                        }
                        vector<string> redirName;
                        vector<string> strVec = split(levels[i], "<");
                        for (string str : strVec) {
                            vector<string> tmp = split(str, ">");
                            redirName.insert(redirName.end(), tmp.begin(), tmp.end());
                        }
                        for (int k = 0; k < redirList.size(); k++) {
                            if (redirList[k] == "input") {
                                int fd2;
                                if ((fd2 = open(redirName[k + 1].c_str(), O_RDONLY, 0666)) == -1) {
                                    cerr << redirName[k + 1].c_str() << endl;
                                    exit(EXIT_FAILURE);
                                }
                                dup2(fd2, 0); // redirect STDIN to fd2
                                close(fd2); // close the fd2
                                if (i < levels.size() - 1) {
                                    dup2(fd[1], 1); // redirect STDOUT to fd[1]
                                    close (fd[1]); // close the fd[1]
                                }
                            }
                            else if (redirList[k] == "output") {
                                int fd3;
                                if ((fd3 = open(redirName[k + 1].c_str(), O_WRONLY | O_CREAT, 0666)) == -1) {
                                    cerr << redirName[k + 1].c_str() << endl;
                                    exit(EXIT_FAILURE);
                                }
                                dup2(fd3, 1); // redirect STDOUT to fd3
                                close(fd3); // close the fd3
                            }
                        }
                        execute(redirName[0]);
                    } else {
                        if (i < levels.size() - 1) {
                            dup2(fd[1], 1); // redirect STDOUT to fd[1]
                            close(fd[1]); // close the fd[1]
                        }                
                        // 2. execute the command at this level
                        execute(levels[i]); 
                    }
                } else { // in the parent process
                    if (!background_process) { // Background processes
                        while (true) {
                            int waitpid = wait(0); // wait for the child process 
                            if (waitpid != pid) {
                                cout << "waitpid: " << waitpid << ", pid: " << pid << endl;
                                while (pidList.size()) {
                                    if (waitpid == pidList[i]) break;
                                }                             
                            } else if (waitpid == pid) {
                                break;
                            }
                        }
                    }
                    dup2(fd[0], 0); // now redirect the input for the next loop iteration
                    close(fd[1]); // fd [1] MUST be closed, otherwise the next level will wait
                }
            }
            dup2(stdout,1);
            dup2(stdin,0);
        }
    }
    close(stdout);
    close(stdin);
}
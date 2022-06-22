#include <process.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <conio.h>
#include <algorithm>
#include <string>

#include "Employee.h"


employee* emps;
HANDLE* hReadyEvents;
CRITICAL_SECTION cs;
bool *isEmpModifying;
int empCount;
int employeeSize = sizeof(employee);

const std::string pipeName = "\\\\.\\pipe\\pipe_name";
const int BUFF_LENGTH = 10;
const int MESSAGE_LENGTH = 10;
char buff[BUFF_LENGTH];

int generateCountOfClient() {
    srand(time(0));
    return (rand() / 5 + 3) % 5 + 2;
}

int empCmp(const void* p1, const void* p2){
    return ((employee*)p1)->num - ((employee*)p2)->num;
}

void print(employee emp){
    std::cout << "Number: " << emp.num << "\tName: " << emp.name << "\tHours: " << emp.hours << std::endl;
}

employee* findEmp(const int ID){
    employee key;
    key.num = ID;
    return (employee*)bsearch(&key, emps, empCount, employeeSize, empCmp);
}

void sortEmps(){
    qsort(emps, empCount, employeeSize, empCmp);
}

void writeData(std::string fileName){
    std::fstream fin(fileName.c_str(), std::ios::out | std::ios::binary);
    fin.write(reinterpret_cast<const char *>(emps), employeeSize * empCount);

    std::cout << "Data has been writing."<<std::endl;
    fin.close();
}

void readDataSTD(){
    emps = new employee[empCount];

    std::cout << "Enter number, name and working hours of each employee: " << std:: endl;
    for(int i = 0; i < empCount; ++i){
        std::cout << "№" << i + 1 << ": ";
        std::cin >> emps[i].num >> emps[i].name >> emps[i].hours;
    }
}

void startPocesses(const int COUNT){
    for(int i = 0; i < COUNT; ++i) {
        std::string cmdArgs = "..\\..\\Client\\cmake-build-debug\\Client.exe ";
        std::string eventName = "READY_EVENT_";
        itoa(i + 1, buff, BUFF_LENGTH);

        eventName += buff;
        cmdArgs += eventName;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        si.cb = sizeof(STARTUPINFO);
        ZeroMemory(&si, sizeof(STARTUPINFO));

        hReadyEvents[i] = CreateEvent(nullptr, TRUE, FALSE, eventName.c_str());

        char tempArg[60];
        strcat(tempArg, cmdArgs.c_str());
        bool isStarted = CreateProcess(nullptr, tempArg, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
        if (!isStarted) {
            std::cout << "Creation process error!" << std::endl;

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}

DWORD WINAPI messaging(LPVOID _pipe){
    employee* errorEmp = new employee;
    errorEmp->num = -1;

    HANDLE _hPipe = (HANDLE)_pipe;
    while(1){
        DWORD bytes;
        char message[MESSAGE_LENGTH];

        bool isRead = ReadFile(_hPipe, message, MESSAGE_LENGTH, &bytes, nullptr);

        if(isRead == false){
            if(ERROR_BROKEN_PIPE == GetLastError()){
                std::cout << "Client disconnected." << std::endl;
            }
            else {
                std::cerr << "Error in reading a message." << std::endl;
            }
            break;
        }

        if(strlen(message) != 0) {
            char command = message[0];
            message[0] = ' ';
            int id = atoi(message);

            DWORD bytesWritten;
            EnterCriticalSection(&cs);
            employee* empToSend = findEmp(id);
            LeaveCriticalSection(&cs);

            if(nullptr == empToSend){
                empToSend = errorEmp;
            }
            else{
                int ind = empToSend - emps;

                if(isEmpModifying[ind]) {
                    empToSend = errorEmp;
                }
                else{
                    if(command == 'r') {
                        std::cout << "Requested to read ID " << id << ".";
                    } else if(command == 'w') {
                        std::cout << "Requested to modify ID " << id << ".";
                        isEmpModifying[ind] = true;
                    } else {
                        std::cout << "Unknown request. ";
                    }
                }
            }
            bool isSent = WriteFile(_hPipe, empToSend, employeeSize, &bytesWritten, nullptr);
            if(isSent) {
                std::cout << "Answer is sent.\n";
            }
            else {
                std::cout << "Error in sending answer.\n";
            }

            if(empToSend != errorEmp && 'w' == command){
                isRead = ReadFile(_hPipe, empToSend, employeeSize, &bytes, nullptr);
                if(!isRead){
                    std::cerr << "Error in reading a message." << std::endl;
                    break;
                }
                else{
                    std::cout << "Employee record changed.\n";

                    isEmpModifying[empToSend - emps] = false;
                    EnterCriticalSection(&cs);
                    sortEmps();
                    LeaveCriticalSection(&cs);
                }
            }
        }
    }
    FlushFileBuffers(_hPipe);
    DisconnectNamedPipe(_hPipe);
    CloseHandle(_hPipe);
    delete errorEmp;
    return 0;
}

void openPipes(int clientCount){
    HANDLE hPipe;
    HANDLE* hThreads = new HANDLE[clientCount];
    for(int i = 0; i < clientCount; ++i){
        hPipe = CreateNamedPipe(pipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, nullptr);
        if(!ConnectNamedPipe(hPipe, nullptr)){
            std::cout << "No connected clients.\n";
            break;
        }
        if (INVALID_HANDLE_VALUE == hPipe){
            std::cerr << "Create named pipe failed.\n";
            getch();
            return;
        }
        hThreads[i] = CreateThread(nullptr, 0, messaging, (LPVOID)hPipe,0,nullptr);
    }
    std::cout << "Clients connected to pipe.\n";
    WaitForMultipleObjects(clientCount, hThreads, TRUE, INFINITE);
    std::cout << "All clients are disconnected.\n";
    delete[] hThreads;
}

int main() {
    std::string filename;
    std::cout << "Input name of file: " << std:: endl;
    std::cin >> filename;
    std::cout << "Input count of employees: " <<std::endl;
    std::cin >> empCount;

    readDataSTD();
    writeData(filename);
    sortEmps();

    int countOfClient = generateCountOfClient();
    isEmpModifying = new bool[empCount];

    for(int i = 0; i < empCount; ++i) {
        isEmpModifying[i] = false;
    }

    InitializeCriticalSection(&cs);
    HANDLE handleStartALL = CreateEvent(nullptr, TRUE, FALSE, "START_ALL");

    hReadyEvents = new HANDLE[countOfClient];
    startPocesses(countOfClient);

    WaitForMultipleObjects(countOfClient, hReadyEvents, TRUE, INFINITE);

    SetEvent(handleStartALL);

    openPipes(countOfClient);

    for(int i = 0; i < empCount; ++i) {
        print(emps[i]);
    }

    std::cout << "Press any key to exit..." << std::endl;
    getch();
    DeleteCriticalSection(&cs);

    delete[] isEmpModifying;
    delete[] hReadyEvents;
    delete[] emps;

    return 0;
}
#include <iostream>
#include <conio.h>
#include <windows.h>
#include <string>
#include "Employee.h"

const int COMMAND_LENGTH = 12;
const int MS_PIPE_WAIT = 2000;
const std::string IP_NAME = "START_ALL";
const std::string PIPE_NAME = "\\\\.\\pipe\\pipe_name";

void errorMessage(std::string message) {
    std::cerr <<  message;
    getch();
}

void print(employee emp){
    std::cout << "Number: " << emp.num << "\tName: " << emp.name << "\tHours: " << emp.hours << std::endl;
}

void messaging(HANDLE handlePipe){
    std::cout << "If you want to quit, press Ctrl+Z..."<<std::endl;

    while(1){
        std::cout << "Enter 'r' to read or 'w' to write command and ID of employee: ";
        char command[COMMAND_LENGTH];
        std::cin.getline(command, COMMAND_LENGTH, '\n');

        if(std::cin.eof()) {
            errorMessage("File is empty... \n");
            return;
        }

        bool isSent;
        DWORD bytesWritten;
        isSent = WriteFile(handlePipe, command, COMMAND_LENGTH, &bytesWritten, nullptr);

        if(!isSent){
            errorMessage("Message cannot be sent...\n");
            return;
        }

        bool isRead;
        DWORD readBytes;
        employee tempEmployee;
        isRead = ReadFile(handlePipe, &tempEmployee, sizeof(tempEmployee), &readBytes, nullptr);

        if(!isRead){
            errorMessage("Error in receiving answer...\n");
            continue;
        }

        if(tempEmployee.num < 0) {
            errorMessage("Employee not found\n");
            continue;
        }

        print(tempEmployee);

        if ('w' == command[0]) {
            std::cout << "Enter number of employee: "<<std::endl;
            std::cin >> tempEmployee.num;
            std::cout << "Enter name of employee: "<<std::endl;
            std::cin >> tempEmployee.name;
            std::cout << "Enter working hours of employee: " << std::endl;
            std::cin >> tempEmployee.hours;

            std::cin.ignore(2, '\n');

            isSent = WriteFile(handlePipe, &tempEmployee, sizeof(tempEmployee), &bytesWritten, nullptr);

            if (!isSent) {
                errorMessage("Error in sending...\n");
                break;
            }

            std::cout << "New record has been sent.\n";
        }
    }
}

int main(int argc, char** argv) {
    HANDLE handleReadyEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, argv[1]);
    HANDLE handleStartEvent = OpenEvent(SYNCHRONIZE, FALSE, IP_NAME.c_str());

    if(handleStartEvent == nullptr || handleReadyEvent == nullptr){
        std::cerr << "Error in action with event..." << std::endl;
        getch();
        return GetLastError();
    }

    SetEvent(handleReadyEvent);
    WaitForSingleObject(handleStartEvent, INFINITE);
    HANDLE handlePipe;

    std::cout << "Process is started." << std::endl;

    while(1) {
        handlePipe = CreateFile(PIPE_NAME.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        const bool FLAG = WaitNamedPipe(PIPE_NAME.c_str(), MS_PIPE_WAIT) + (INVALID_HANDLE_VALUE != handlePipe);
        if (FLAG) {
            if(INVALID_HANDLE_VALUE != handlePipe) {
                break;
            }
            errorMessage(MS_PIPE_WAIT + "timed out...\n");
            return 0;
        }
    }
    messaging(handlePipe);
    return 0;
}
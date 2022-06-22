#ifndef OS_LAB_5_EMPLOYEE_H
#define OS_LAB_5_EMPLOYEE_H

#include <cstring>

struct employee{
public:
    int num;
    char name[10];
    double hours;

    employee() {
        num = 0;
        strcpy(name, "Unknown");
        hours = 0;
    }

    employee(int num, const char nametmp[], double hours) {
        this->num = num;
        strcpy(name, nametmp);
        this->hours = hours;
    }

    int empCmp(const void* p1, const void* p2){
        return ((employee*)p1)->num - ((employee*)p2)->num;
    }
};


#endif //OS_LAB_5_EMPLOYEE_H

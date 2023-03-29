#pragma once
#include <iostream>
#include <string>

class Task
{
public:
    Task(int a = 0, int b = 0, char op = '0')
        : _a(a), _b(b), _operator(op)
    {
    }

    int operator()()
    {
        return handle();
    }
    
    int handle()
    {
        int result = 0;
        switch (_operator)
        {
        case '+':
            result = _a + _b;
            break;
        case '-':
            result = _a - _b;
            break;
        case '*':
            result = _a * _b;
            break;
        case '/':
            if (_b == 0)
            {
                std::cout << "div zero, abort" << std::endl;
                result = -1;
            }
            else
            {
                result = _a / _b;
            }
            break;
        case '%':
            if (_b == 0)
            {
                std::cout << "mod zero, abort" << std::endl;
                result = -1;
            }
            else
            {
                result = _a % _b;
            }
            break;
        default:
            std::cout << "非法操作: " << _operator << std::endl;
            break;
        }

        return result;
    }

    void get(int *a, int *b, char *op)
    {
        *a = _a;
        *b = _b;
        *op = _operator;
    }
private:
    int _a;
    int _b;
    char _operator;
};
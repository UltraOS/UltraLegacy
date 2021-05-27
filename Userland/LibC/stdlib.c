#include "string.h"
#include "ctype.h"
#include "stdbool.h"
#include "limits.h"

int abs(int n)
{
    return n < 0 ? -n : n;
}

long labs(long n)
{
    return n < 0 ? -n : n;
}

long long llabs(long long n)
{
    return n < 0 ? -n : n;
}

void exit(int exit_code)
{
   // process_exit(exit_code);
}

int atoi(const char* str)
{
    size_t i = 0;

    while (str[i] && isspace(str[i]))
        i++;

    if (!str[i])
        return 0;

    bool is_negative = false;
    if (str[i] == '-' || str[i] == '+') {
        is_negative = str[i++] == '-';
    }

    int result = 0;

    while (str[i] && isdigit(str[i])) {
        if (result == 0 && str[i] == '0') {
            i++;
            continue;
        }

        char digit = str[i] - '0';
        if (is_negative)
            digit *= -1;

        if (result > (INT_MAX / 10) || (result == (INT_MAX / 10) && digit > 7))
            return INT_MAX;
        if (result < (INT_MIN / 10) || (result == (INT_MIN / 10) && digit < -8))
            return INT_MIN;

        result *= 10;
        result += digit;

        i++;
    }

    return result;
}

// very naive :(
double atof(const char* str)
{
    double result = 0.0;
    double multiplier = 1.0;

    bool point_seen = false;

    size_t i = 0;

    while (str[i] && isspace(str[i]))
        i++;

    if (str[i] == '-') {
        i++;
        multiplier = -1.0;
    } else if (str[i] == '+') {
        i++;
    }

    for (; str[i]; ++i) {
        if (point_seen && !isdigit(str[i]))
            return result * multiplier;

        if (str[i] == '.') {
            point_seen = true;
            continue;
        }

        double digit = str[i] - '0';

        if (point_seen)
            multiplier /= 10.0;

        result *= 10;
        result += digit;
    }

    return result * multiplier;
}

long syscall_1(long function, long arg0)
{
    long result;
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(function), "b"(arg0)
                 : "memory");
    return result;
}

long syscall_2(long function, long arg0, long arg1)
{
    long result;
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(function), "b"(arg0), "c"(arg1)
                 : "memory");
    return result;
}

long syscall_3(long function, long arg0, long arg1, long arg2)
{
    long result;
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(function), "b"(arg0), "c"(arg1), "d"(arg2)
                 : "memory");
    return result;
}
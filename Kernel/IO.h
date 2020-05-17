#pragma once

#include "Types.h"

class IO
{
public:
    template<u16 port>
    static void out8(u8 data)
    {
        asm volatile("outb %0, %1" ::"a"(data), "Nd"(port));
    }

    template<u16 port>
    static void out16(u16 data)
    {
        asm volatile("outw %0, %1" ::"a"(data), "Nd"(port));
    }

    template<u16 port>
    static void out32(u32 data)
    {
        asm volatile("outl %0, %1" ::"a"(data), "Nd"(port));
    }

    template<u16 port>
    static u8 in8()
    {
        u8 data;
        asm volatile("inb %1, %0" :"=a"(data) :"Nd"(port));
        return data;
    }

    template<u16 port>
    static u16 in16()
    {
        u16 data;
        asm volatile("inw %1, %0" :"=a"(data) :"Nd"(port));
        return data;
    }

    template<u16 port>
    static u32 in32()
    {
        u32 data;
        asm volatile("inl %1, %0" :"=a"(data) :"Nd"(port));
        return data;
    }
};

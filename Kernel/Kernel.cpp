#include "Types.h"
#include "Log.h"
#include "InterruptDescriptorTable.h"
#include "InterruptServiceRoutines.h"

static constexpr u32 video_memory = 0xB8000;

void write_string(const char* string, u8 color = 0xF);

class Checker
{
public:
    Checker()
    {
        write_string("Constructor called", 0x2);
    }

    ~Checker()
    {
        write_string("Destructor called", 0x2);
    }
};

static Checker c;

void write_string(const char* string, u8 color)
{
    static u16* memory = reinterpret_cast<u16*>(video_memory);

    while (*string)
    {
        u16 colored_char = *(string++);
        colored_char |= color << 8;

        *(memory++) = colored_char;
    }
}

namespace kernel {
    extern "C" void run()
    {
        cli();
        InterruptServiceRoutines::install();
        InterruptDescriptorTable::the().install();
        sti();

        constexpr auto greeting = "Hello from the kernel! Let's try to divide by zero!\n";
        log() << greeting;
        write_string(greeting, 0x4);

        asm volatile("div %0" :: "r"(0));
    }
}

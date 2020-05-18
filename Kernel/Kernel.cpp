#include "Types.h"
#include "Log.h"

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
        constexpr auto greeting = "Hello from the kernel!\n";
        log() << greeting;
        write_string(greeting, 0x4);
    }
}

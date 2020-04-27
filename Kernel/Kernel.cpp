static constexpr unsigned long video_memory = 0xB8000;

void write_string(const char* string, unsigned char color = 0xF);

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

void write_string(const char* string, unsigned char color)
{
    static unsigned short* memory = reinterpret_cast<unsigned short*>(video_memory);

    while (*string)
    {
        unsigned short colored_char = *(string++);
        colored_char |= color << 8;

        *(memory++) = colored_char;
    }
}

extern "C" void run()
{
    write_string("Hello from the kernel!", 0x4);
}

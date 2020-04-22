static constexpr unsigned long video_memory = 0xB8000;

void write_string(const char* string, unsigned char color = 0xF)
{
    static unsigned short* memory = reinterpret_cast<unsigned short*>(video_memory);

    while (*string)
    {
        unsigned short colored_char = *(string++);
        colored_char |= color << 8;

        *(memory++) = colored_char;
    }
}

typedef void (*constructor)();

extern "C"
{
    constructor global_constructors_begin;
    constructor global_constructors_end;

    void init_global_objects()
    {
        for (constructor ctor = global_constructors_begin; ctor != global_constructors_end; ctor++)
            (*ctor)();
    }
}

extern "C" void run()
{
    write_string("Hello from the kernel!", 0x4);

    for (;;) {}
}

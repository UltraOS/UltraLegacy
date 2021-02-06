#pragma once

namespace kernel {

class PCI {
public:
    static void detect_all();

    static PCI& the()
    {
        return s_instance;
    }


private:
    static PCI s_instance;
};

}

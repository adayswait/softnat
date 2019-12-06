#include <iostream>
#include <thread>
#include "softnat.h"
constexpr const char *VERSION = "0.0.1";

int main(int argc, char *argv[])
{

    char **p = argv;

    while (*p)
    {
        if (strcmp(*p, "-v") == 0)
        {
            std::cout << VERSION << std::endl;
            p++;
            continue;
        }
        p++;
    }

    smuggler s("10.1.1.248", 3002, "10.1.1.248", 6379);
    // insider i("10.1.1.248", 6379, "10.1.1.248", 3002);
    // handler h("10.1.1.248", 3002, 3001);

    return 0;
}

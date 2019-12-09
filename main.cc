#include <cstring>
#include <cstdlib>
#include <iostream>
#include "softnat.h"

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "usage" << std::endl;
        return 0;
    }
    char *ap_act = argv[2];
    char *ap_psv = argv[3];
    int ap_act_len = strlen(ap_act);
    int ap_psv_len = strlen(ap_psv);

    char *addr_act, *addr_psv;
    unsigned short port_act, port_psv;
    char ap_separator[2] = ":";
    char *tk = strtok(ap_act, ap_separator);
    if (tk == ap_act && strlen(tk) == ap_act_len)
    {
        std::cout << ": not found in ap_act" << std::endl;
        return -1;
    }
    if (tk != nullptr)
    {
        addr_act = (char *)malloc(strlen(tk) + 1);
        strcpy(addr_act, tk);
    }
    else
    {
        std::cout << "ap_act invalid 1" << std::endl;
        return -1;
    }
    tk = strtok(nullptr, ap_separator);
    if (tk != nullptr)
    {
        port_act = atoi(tk);
    }
    else
    {
        std::cout << "ap_act invalid 2" << std::endl;
        return -1;
    }

    tk = strtok(ap_psv, ap_separator);
    if (tk == ap_psv && strlen(tk) == ap_psv_len)
    {
        std::cout << ": not found in ap_psv" << std::endl;
        return -1;
    }
    if (tk != nullptr)
    {
        addr_psv = (char *)malloc(strlen(tk) + 1);
        strcpy(addr_psv, tk);
    }
    else
    {
        std::cout << "ap_psv invalid 1" << std::endl;
        return -1;
    }
    tk = strtok(nullptr, ap_separator);
    if (tk != nullptr)
    {
        port_psv = atoi(tk);
    }
    else
    {
        std::cout << "ap_psv invalid 2" << std::endl;
        return -1;
    }

    if (strcmp(argv[1], "-smuggler") == 0 || strcmp(argv[1], "-s") == 0)
    {
        softnat::smuggler s(addr_act, port_act, addr_psv, port_psv);
    }
    else if (strcmp(argv[1], "-handler") == 0 || strcmp(argv[1], "-h") == 0)
    {
        softnat::handler h("0.0.0.0", port_act, port_psv);
    }
    else if (strcmp(argv[1], "-insider") == 0 || strcmp(argv[1], "-i") == 0)
    {
        softnat::insider i(addr_act, port_act, addr_psv, port_psv);
    }
    else
    {
        std::cout << "invalid mode : " << argv[1] << std::endl;
        return -1;
    }

    return 0;
}

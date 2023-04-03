#include <iostream>
#include <string>
#include <memory>
#include "Util.hpp"
#include "HttpServer.hpp"

static void Usage(std::string proc)
{
    std::cout << "Usage:\n\t" << proc << " port" << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(5);
    }
    uint16_t port = atoi(argv[1]);
    std::shared_ptr<HttpServer> serv(new HttpServer(port));
    serv->InitServer();
    serv->Loop();

    return 0;
}
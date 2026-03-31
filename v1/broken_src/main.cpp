#include "../broken_include/httpserver.h"
#include <memory>
using namespace http;
int main()
{
    std::shared_ptr<Httpserver> svr(new Httpserver(8080));
    svr->Init();
    svr->Run();
}
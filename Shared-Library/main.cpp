#include <iostream>
#include "addition.h"
#include <dlfcn.h>

int main() {
    void *handle = dlopen("./libaddition.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot open library: " << dlerror() << '\n';
        return 1;
    }
    Addition* (*create)();
    create = (Addition* (*)())dlsym(handle, "create_object");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol create: " << dlsym_error << '\n';
        dlclose(handle);
        return 1;
    }
    Addition* add = (Addition*)create();
    std::cout << "2 + 3 = " << add->add(2, 3) << '\n';
    delete add;
    dlclose(handle);
    return 0;
}

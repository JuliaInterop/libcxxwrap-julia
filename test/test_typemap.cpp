#include "typemaptest.h"
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

int main()
{
  typedef void (*init_types_t)();
  typedef void (*show_types_t)();
  init_types_t lib_init_types;
  show_types_t lib_show_types;

  #ifdef __APPLE__
    const std::string lib_extension = ".dylib";
  #else
    const std::string lib_extension = ".so";
  #endif

  #ifdef _WIN32
    HMODULE handle_typemaplib = LoadLibraryA("./libtypemaplib.dll");
    if (!handle_typemaplib) {
      std::cerr << "Failed to load library 'libtypemaplib': " << GetLastError() << std::endl;
      return 1;
    }

    lib_init_types = (init_types_t)GetProcAddress(handle_typemaplib, "init_types");
    if (!lib_init_types) {
      std::cerr << "Failed to load symbol 'init_types': " << GetLastError() << std::endl;
      FreeLibrary(handle_typemaplib);
      return 1;
    }
  #else
    void* handle_typemaplib = dlopen(("./libtypemaplib" + lib_extension).c_str(), RTLD_LAZY);
    if (!handle_typemaplib) {
      std::cerr << "Failed to load library 'libtypemaplib': " << dlerror() << std::endl;
      return 1;
    }

    lib_init_types = (init_types_t)dlsym(handle_typemaplib, "init_types");
    if (!lib_init_types) {
      std::cerr << "Failed to load symbol 'init_types': " << dlerror() << std::endl;
      dlclose(handle_typemaplib);
      return 1;
    }
  #endif

  lib_init_types();  

  #ifdef _WIN32
    HMODULE handle_typemaptestlib = LoadLibraryA("./libtypemaptestlib.dll");
    if (!handle_typemaptestlib) {
      std::cerr << "Failed to load library 'libtypemaptestlib': " << GetLastError() << std::endl;
      FreeLibrary(handle_typemaplib);
      return 1;
    }

    lib_show_types = (show_types_t)GetProcAddress(handle_typemaptestlib, "show_types");
    if (!lib_show_types) {
      std::cerr << "Failed to load symbol 'show_types': " << GetLastError() << std::endl;
      FreeLibrary(handle_typemaplib);
      FreeLibrary(handle_typemaptestlib);
      return 1;
    }
  #else
    void* handle_typemaptestlib = dlopen(("./libtypemaptestlib" + lib_extension).c_str(), RTLD_LAZY);
    if (!handle_typemaptestlib) {
      std::cerr << "Failed to load library 'libtypemaptestlib': " << dlerror() << std::endl;
      dlclose(handle_typemaplib);
      return 1;
    }

    lib_show_types = (show_types_t)dlsym(handle_typemaptestlib, "show_types");
    if (!lib_show_types) {
      std::cerr << "Failed to load symbol 'show_types': " << dlerror() << std::endl;
      dlclose(handle_typemaplib);
      dlclose(handle_typemaptestlib);
      return 1;
    }
  #endif

  lib_show_types();

  return 0;
}

#include <cstdlib>
#include <cstring>

static
bool readint_maybe(const char* str, int base, uint64_t& ret)
{
    char* p = NULL;
    uint64_t maybe = 0;
    maybe = strtoul(str, &p, base);
    if (p - str != strlen(str)) {
        return false;
    }
    ret = maybe;
    return true;
}

template<typename INT>
bool readint(const char* str, INT& ret)
{
    uint64_t maybe = 0;

    if (readint_maybe(str, 10, maybe)) {
        ret = maybe;
        return true;
    }
    if (readint_maybe(str, 16, maybe)) {
        ret = maybe;
        return true;
    }
    return false;
}


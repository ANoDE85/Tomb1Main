#include "args.h"

#include "log.h"
#include "specific/memory.h"

#include <windows.h>
#include <shellapi.h>
#include <string.h>

int get_command_line(char ***args, int *arg_count)
{
    LPWSTR *l_arg_list;
    int l_arg_count;

    l_arg_list = CommandLineToArgvW(GetCommandLineW(), &l_arg_count);
    if (!l_arg_list) {
        LOG_ERROR("CommandLineToArgvW failed");
        return 0;
    }

    *args = S_Memory_Alloc(l_arg_count * sizeof(char **));
    *arg_count = l_arg_count;
    for (int i = 0; i < l_arg_count; i++) {
        size_t size = wcslen(l_arg_list[i]) + 1;
        (*args)[i] = S_Memory_Alloc(size);
        wcstombs((*args)[i], l_arg_list[i], size);
    }

    LocalFree(l_arg_list);

    return 1;
}

#include "filesystem.h"

#include "specific/memory.h"

#include <stdio.h>

struct MYFILE {
    FILE *fp;
};

MYFILE *FileOpen(const char *path, FILE_OPEN_MODE mode)
{
    MYFILE *file = S_Memory_Alloc(sizeof(MYFILE));
    switch (mode) {
    case FILE_OPEN_WRITE:
        file->fp = fopen(path, "wb");
        break;
    case FILE_OPEN_READ:
        file->fp = fopen(path, "rb");
        break;
    default:
        file->fp = NULL;
        break;
    }
    if (!file->fp) {
        S_Memory_Free(file);
        return NULL;
    }
    return file;
}

size_t FileRead(void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fread(data, item_size, count, file->fp);
}

size_t FileWrite(const void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fwrite(data, item_size, count, file->fp);
}

void FileSeek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode)
{
    switch (mode) {
    case FILE_SEEK_SET:
        fseek(file->fp, pos, SEEK_SET);
        break;
    case FILE_SEEK_CUR:
        fseek(file->fp, pos, SEEK_CUR);
        break;
    case FILE_SEEK_END:
        fseek(file->fp, pos, SEEK_END);
        break;
    }
}

size_t FileSize(MYFILE *file)
{
    size_t old = ftell(file->fp);
    fseek(file->fp, 0, SEEK_END);
    size_t size = ftell(file->fp);
    fseek(file->fp, old, SEEK_SET);
    return size;
}

void FileClose(MYFILE *file)
{
    fclose(file->fp);
    S_Memory_Free(file);
}

int FileDelete(const char *path)
{
    return remove(path);
}

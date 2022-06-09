#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <regex.h>
#include <linux/limits.h>

#define LEN 32

static int (* original_chmod)(const char*, mode_t mode) = NULL;
static int (* original_chown)(const char*, uid_t, gid_t) = NULL;
static int (* original_close)(int) = NULL;
static int (* original_creat)(const char*, mode_t) = NULL;
static int (* original_fclose)(FILE*) = NULL;
static FILE* (* original_fopen)(const char*, const char*) = NULL;
static size_t (* original_fread)(void*, size_t, size_t, FILE *) = NULL;
static size_t (* original_fwrite)(const void*, size_t, size_t, FILE *) = NULL;
static int (* original_open)(const char*, int, ...) = NULL;
static ssize_t (* original_read)(int, void*, size_t) = NULL;
static int (* original_remove)(const char*) = NULL;
static int (* original_rename)(const char*, const char*) = NULL;
static FILE* (* original_tmpfile)(void) = NULL;
static ssize_t (* original_write)(int, const void*, size_t) = NULL;

FILE* output_file = NULL;

void find_output_file_ptr() {
    if(output_file == NULL) {
        if(getenv("output_file_fd") != NULL) {
            int fd = atoi(getenv("output_file_fd"));
            output_file = fdopen(fd, "w");
        }
        else{
            output_file = stderr;
        }
    }
}

void find_original_call(void** f, char* f_name) {
    if(*f == NULL) {
        *f = dlsym(RTLD_DEFAULT, f_name);
    }
}

void get_real_filename(const char* path, char* buffer) {
    realpath(path, buffer);
}

void get_fd_path(int fd, char* buffer) {
    char fd_path[500] = "";
    char link_dest[PATH_MAX];
    int link_dest_size;

    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%d", getpid(), fd);
    link_dest_size = readlink(fd_path, link_dest, sizeof(link_dest)-1);
    link_dest[link_dest_size] = '\0';
    // snprintf(buffer, sizeof(buffer), "%s", link_dest);
    strcpy(buffer, link_dest);
}

void get_isprint_string(char* str, int count, char* buffer) {
    // strcpy(buffer, str);
    strncpy(buffer, str, LEN);
    buffer[32] = '\0';
    for(int i = 0; i < strlen(buffer); i++) {
        if(!isprint(buffer[i])){
            buffer[i] = '.';
        }
    }
}

//////////////////////////////////
// shared object method
//////////////////////////////////

int chmod(const char *pathname, mode_t mode) {
    find_original_call((void**)&original_chmod, "chmod");
    int response;
    if(original_chmod != NULL){
        response = original_chmod(pathname, mode);

        // print log
        find_output_file_ptr();

        char buffer[400];
        get_real_filename(pathname, buffer);
        fprintf(output_file, "[logger] chmod(\"%s\", %o) = %d\n", buffer, mode, response);
    }

    return response;
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    find_original_call((void**)&original_chown, "chown");
    int response;
    if(original_chown != NULL) {
        response = original_chown(pathname, owner, group);
        
        // print log
        find_output_file_ptr();
        
        char buffer[400];
        get_real_filename(pathname, buffer);
        fprintf(output_file, "[logger] chown(\"%s\", %o, %o) = %d\n", buffer, owner, group, response);
    }
    
    return response;
}

int close(int fd) {
    find_original_call((void**)&original_close, "close");
    
    int response;
    if(original_close != NULL) {
        char buffer[500];
        get_fd_path(fd, buffer);

        int fd_cp;
        if(fd == 2)
            fd_cp = dup(2);

        response = original_close(fd);
        
        if(fd == 2)
            dup2(fd_cp, 2);
        
        find_output_file_ptr();
        fprintf(output_file, "[logger] close(\"%s\") = %d\n", buffer, response);

    }
    return response;
}

int creat(const char* path, mode_t mode) {
    find_original_call((void**)&original_creat, "creat");

    int response;
    if(original_creat != NULL) {
        response = original_creat(path, mode);

        // print log
        char buffer[500];
        get_real_filename(path, buffer);

        find_output_file_ptr();
        fprintf(output_file, "[logger] creat(\"%s\", %o) = %d\n", buffer, mode, response);
    }
    
    return response;
}

int fclose(FILE* stream) {
    find_original_call((void**)&original_fclose, "fclose");

    int response;
    if(original_fclose != NULL) {     
        char buffer[500];
        int fd = fileno(stream);
        get_fd_path(fd, buffer);

        int output_fd = fileno(output_file);
        int fd_cp;
        if(fd == 2){
            fd_cp = dup(2);
        }

        response = original_fclose(stream);
        
        if(fd == 2 && output_fd == 2){
            dup2(fd_cp, 2);
            stderr = fdopen(2, "w");
            output_file = stderr;
        }

        find_output_file_ptr();
        fprintf(output_file, "[logger] fclose(\"%s\") = %d\n", buffer, response);
    }

    return response;
}

FILE* fopen(const char* restrict pathname, const char* restrict mode) {
    find_original_call((void**)&original_fopen, "fopen");

    FILE* response;
    if(original_fopen != NULL) {
        response = original_fopen(pathname, mode);

        //print log
        char buffer[500];
        get_real_filename(pathname, buffer);

        find_output_file_ptr();
        fprintf(output_file, "[logger] fopen(\"%s\", \"%s\") = %p\n", buffer, mode, response);
    }

    return response;
}

size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {
    find_original_call((void**)&original_fread, "fread");

    size_t response;
    if(original_fread != NULL) {
        response = original_fread(ptr, size, nmemb, stream);

        // print log
        char buffer1[500];
        char buffer2[500];
        int fd = fileno(stream);
        get_fd_path(fd, buffer1);
        get_isprint_string((char*)ptr, size, buffer2);

        find_output_file_ptr();
        fprintf(output_file, "[logger] fread(\"%s\", %lu, %lu, \"%s\") = %lu\n", buffer2, size, nmemb, buffer1, response);
        
    }

    return response;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {
    find_original_call((void**)&original_fwrite, "fwrite");

    size_t response;
    if(original_fwrite != NULL) {
        response = original_fwrite(ptr, size, nmemb, stream);

        // print log
        char buffer1[500];
        char buffer2[500];
        int fd = fileno(stream);
        get_fd_path(fd, buffer1);
        get_isprint_string((char*)ptr, size, buffer2);

        find_output_file_ptr();
        fprintf(output_file, "[logger] fwrite(\"%s\", %lu, %lu, \"%s\") = %lu\n", buffer2, size, nmemb, buffer1, response);
    }

    return response;
}

int open(const char* path, int oflag, ...) {
    find_original_call((void**)&original_open, "open");

    int mode = 0;
    if(__OPEN_NEEDS_MODE (oflag)) {
        va_list arg;
        va_start (arg, oflag);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    int response;
    if(original_open != NULL) {
        response = original_open(path, oflag, mode);

        // print log
        char buffer[500];
        get_real_filename(path, buffer);

        find_output_file_ptr();
        fprintf(output_file, "[logger] open(\"%s\", %o, %o) = %d\n", buffer, oflag, mode, response);
    }

    return response;
}

ssize_t read(int fd, void *buf, size_t count) {
    find_original_call((void**)&original_read, "read");

    ssize_t response;
    if(original_read != NULL) {
        response = original_read(fd, buf, count);

        // print log
        char buffer1[500];
        char buffer2[500];
        get_fd_path(fd, buffer1);
        get_isprint_string((char*)buf, count, buffer2);

        find_output_file_ptr();
        fprintf(output_file, "[logger] read(\"%s\", \"%s\", %lu) = %lu\n", buffer1, buffer2, count, response);
    }

    return response;
}

int remove(const char *path) {
    find_original_call((void**)&original_remove, "remove");

    int response;
    if(original_remove != NULL) {
        response = original_remove(path);

        // print log
        char buffer[500];
        get_real_filename(path, buffer);

        find_output_file_ptr();
        fprintf(output_file, "[logger] remove(\"%s\") = %d\n", buffer, response);
    }

    return response;
}

int rename(const char* old, const char* new) {
    find_original_call((void**)&original_rename, "rename");

    int response;
    if(original_rename != NULL) {
        char buffer1[500], buffer2[500];
        get_real_filename(old, buffer1);

        response = original_rename(old, new);

        get_real_filename(new, buffer2);

        find_output_file_ptr();
        fprintf(output_file, "[logger] rename(\"%s\", \"%s\") = %d\n", buffer1, buffer2, response);
    }

    return response;
}

FILE* tmpfile(void) {
    find_original_call((void**)&original_tmpfile, "tmpfile");

    FILE* response;
    if(original_tmpfile != NULL) {
        response = original_tmpfile();

        // print log
        find_output_file_ptr();
        fprintf(output_file, "[logger] tmpfile() = %p\n", response);
    }

    return response;
}

ssize_t write(int fd, const void* buf, size_t count) {
    find_original_call((void**)&original_write, "write");

    ssize_t response;
    if(original_write != NULL) {
        response = original_write(fd, buf, count);

        // print log
        char buffer1[500], buffer2[500];
        get_fd_path(fd, buffer1);
        get_isprint_string((char*)buf, count, buffer2);
        
        find_output_file_ptr();
        fprintf(output_file, "[logger] write(\"%s\", \"%s\", %lu) = %lu\n", buffer1, buffer2, count, response);
    }

    return response;
}
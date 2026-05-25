
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "ini.h"

#define MAX_SECTION 50
#define MAX_NAME    50

static char* rstrip(char* s) {
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p))) *p = '\0';
    return s;
}

static char* lskip(const char* s) {
    while (*s && isspace((unsigned char)(*s))) s++;
    return (char*)s;
}

static char* find_chars_or_comment(const char* s, const char* chars) {
    int was_space = 0;
    while (*s && (!chars || !strchr(chars, *s)) &&
           !(was_space && *s == ';') &&
           !(was_space && *s == '#')) {
        was_space = isspace((unsigned char)(*s));
        s++;
    }
    return (char*)s;
}

static char* strncpy0(char* dest, const char* src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
    return dest;
}

int ini_parse_stream(ini_reader reader, void* stream,
                     ini_handler handler, void* user)
{
    char line[INI_MAX_LINE];
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME]  = "";
    char* start;
    char* end;
    char* name;
    char* value;
    int   lineno = 0;
    int   error  = 0;

    while (reader(line, INI_MAX_LINE, stream) != NULL) {
        lineno++;
        start = line;

#if INI_ALLOW_BOM
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
            (unsigned char)start[1] == 0xBB &&
            (unsigned char)start[2] == 0xBF)
            start += 3;
#endif
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#') {
            
        }
#if INI_ALLOW_MULTILINE
        else if (*prev_name && *start == ' ' && start == line) {
            
            if (*start) {
                if (!handler(user, section, prev_name, start) && !error)
                    error = lineno;
            }
        }
#endif
        else if (*start == '[') {
            end = find_chars_or_comment(start + 1, "]");
            if (*end == ']') {
                *end = '\0';
                strncpy0(section, start + 1, sizeof(section));
                *prev_name = '\0';
            } else if (!error) {
                error = lineno;
            }
        } else if (*start) {
            end = find_chars_or_comment(start, "=:");
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
#if INI_ALLOW_INLINE_COMMENTS
                end = find_chars_or_comment(value, INI_INLINE_COMMENT_PREFIXES);
                if (*end)
                    *end = '\0';
#endif
                rstrip(value);
                strncpy0(prev_name, name, sizeof(prev_name));
                if (!handler(user, section, name, value) && !error)
                    error = lineno;
            } else if (!error) {
                error = lineno;
            }
        }
#if INI_STOP_ON_FIRST_ERROR
        if (error) break;
#endif
    }
    return error;
}

int ini_parse_file(FILE* file, ini_handler handler, void* user) {
    return ini_parse_stream((ini_reader)fgets, file, handler, user);
}

int ini_parse(const char* filename, ini_handler handler, void* user) {
    FILE* file;
    int   error;
    file = fopen(filename, "r");
    if (!file) return -1;
    error = ini_parse_file(file, handler, user);
    fclose(file);
    return error;
}

int ini_parse_string(const char* string, ini_handler handler, void* user) {
   
    char buf[INI_MAX_LINE];
    const char* pos = string;
    int   lineno = 0;

   
    struct ctx { const char* cur; };
    struct ctx c = { string };

  
    FILE* f = tmpfile();
    if (!f) return -1;
    fputs(string, f);
    rewind(f);
    int r = ini_parse_file(f, handler, user);
    fclose(f);
    return r;
}

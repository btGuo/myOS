#ifndef FS_PATH_PARSE_H
#define FS_PATH_PARSE_H

char *path_parse(const char *path, char *res);
void split_path(const char *path, char *filename, char *dirname);

#endif

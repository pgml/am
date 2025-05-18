#ifndef  ARCHIVE_READER_H
#define  ARCHIVE_READER_H

char **list_zip_entries(const char *path, int *count_out);
void free_zip_entries(char **list, int count);

#endif

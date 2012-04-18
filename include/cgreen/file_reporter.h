#ifndef FILE_REPORTER_HEADER
#define FILE_REPORTER_HEADER

#ifdef __cplusplus
  extern "C" {
#endif

#include <cgreen/reporter.h>

TestReporter *create_file_reporter(char *file_name);

#ifdef __cplusplus
    }
#endif

#endif

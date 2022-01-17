#include <stddef.h>

#ifndef __cplusplus
  #include <stdbool.h>
#endif

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

#include <sanitizer/dfsan_interface.h>

void taint_print(dfsan_label label, int line);
EXTERN void __dfsw_read_function_args(dfsan_label label, char* fileName, char* functionName, int x);
EXTERN void __dfsw_read_function_args_2(dfsan_label label, char* fileName, char* functionName, int x);
EXTERN void __dfsw_exit_function(int x);
EXTERN void __dfsw_exit_function_2(int x);
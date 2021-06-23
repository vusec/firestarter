#ifndef RCVRY_LOG_H
#define RCVRY_LOG_H
#include <stdio.h>

#define FSCRIBE(FPTR, ...)                                \
          if (NULL != FPTR) {                             \
                  fprintf(FPTR, __VA_ARGS__);             \
                  fflush(FPTR);                           \
          }

#define RCVRY_PRINT_PREFIX

#ifdef RCVRY_PRINT_PREFIX
#ifndef RCVRY_PRINT_TO_FILE
#define RCVRY_PREFIX(L)		\
	printf("[rcvry site:%d] %s: ", rcvry_current_site_id, L);
#else /* defined RCVRY_PRINT_TO_FILE */
#define RCVRY_PREFIX(L)		\
	FSCRIBE(rcvry_fprint_fptr, "[rcvry site:%d] %s: ", rcvry_current_site_id, L);
#endif
#else
#ifndef RCVRY_PRINT_TO_FILE
#define RCVRY_PREFIX(L)		\
        printf("[rcvry site:%d] %s: ", rcvry_current_site_id, L); 
#else
#define RCVRY_PREFIX(L)		\
        FSCRIBE(rcvry_fprint_fptr, "[rcvry site:%d] %s: ", rcvry_current_site_id, L); 
#endif
#endif

#ifdef RCVRY_NO_PRINT
#define rcvry_print_level(...)	{;}
#else
#ifndef RCVRY_PRINT_TO_FILE
#define rcvry_print_level(L, ...) \
        RCVRY_PREFIX(L); printf(__VA_ARGS__);
#else	
#define rcvry_print_level(L, ...) \
        RCVRY_PREFIX(L); FSCRIBE(rcvry_fprint_fptr, __VA_ARGS__);
#endif
#endif

#define rcvry_print_info(...) \
              rcvry_print_level("INFO", __VA_ARGS__);
#define rcvry_print_warning(...) \
              rcvry_print_level("WARNING", __VA_ARGS__);
#define rcvry_print_error(...) \
              rcvry_print_level("ERROR", __VA_ARGS__);
#ifdef DEBUG_PRINT
#define rcvry_print_debug(...) \
              rcvry_print_level("DEBUG", __VA_ARGS__);
#else
#define rcvry_print_debug(...) 	{;}
#endif

#endif

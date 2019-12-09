#ifndef SITE_MON_GSM_DEBUG_H
#define SITE_MON_GSM_DEBUG_H

#include <stdio.h>

#ifndef NDEBUG
    #define DEBUG_LOG(stream, str) { \
        fprintf(stream, str); \
    }
#else
    #define DEBUG_LOG(stream, str) { \
    }
#endif // NDEBUG

#endif // SITE_MON_GSM_DEBUG_H

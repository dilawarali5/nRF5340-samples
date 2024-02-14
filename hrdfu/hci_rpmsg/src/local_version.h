/**
 * @file local_version.h
 * 
 */

#include <stdint.h>

#ifndef local_version_h
#define local_version_h

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    char Major;
    char Minor;
    char Build;
}netCoreFwVersion_Struct;

#ifdef __cplusplus
}
#endif
#endif
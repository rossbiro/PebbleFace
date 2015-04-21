#ifndef REMOTESCREEN_ERRORS_H_
#define REMOTESCREEN_ERRORS_H_

#include "Standard.h"
// Don't care about fixing the nubmers until 1.0
enum ErrorCodes {
    ENOMEM = 1,
    ENOWINDOW, 
    ENOLAYER,
    EINVALID_OP,
    EINVALID_TRANSACTION,
    ENOFONT,
};

#ifndef RCC
#define RCC(x) do { ret = (x); if (ret < 0) goto error_out; } while (0)
#endif
  
#endif

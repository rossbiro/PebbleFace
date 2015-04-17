#ifndef REMOTESCREEN_ERROS_H_
#define REMOTESCREEN_ERRORS_H_

// Don't care about fixing the nubmers until 1.0
enum ErrorCodes {
    ENOMEM = 1,
    ENOWINDOW, 
    ENOLAYER,
    EINVALID_OP,
};

#define RCC(x) do { ret = (x); if (ret < 0) goto error_out; } while (0)

#endif

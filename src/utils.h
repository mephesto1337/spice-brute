#ifndef __UTILS_H__DEFINED__
#define __UTILS_H__DEFINED__

#include <spice-client.h>
#include <sys/socket.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

const char *sockaddr_to_string(const struct sockaddr *sa, socklen_t salen);
int connect_helper(SpiceSession *session);

#endif // __UTILS_H__DEFINED__

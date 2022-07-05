#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <spice-client.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "check.h"
#include "utils.h"

int connect_helper(SpiceSession *session) {
    int sock = -1;
    struct addrinfo *results = NULL;
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM, .ai_flags = AI_ADDRCONFIG};
    const char *hostname = NULL;
    const char *service = NULL;

    assert(session != NULL);
    g_object_get(G_OBJECT(session), "host", &hostname, "port", &service, NULL);
    assert(hostname != NULL);
    assert(service != NULL);

    CHK(getaddrinfo(hostname, service, &hints, &results), != 0);
    for (struct addrinfo *ai = results; ai != NULL; ai = ai->ai_next) {
        if ((sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
            continue;
        }
        if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            close(sock);
            sock = -1;
            continue;
        }
        fprintf(stderr, "Connnected to %s\n", sockaddr_to_string(ai->ai_addr, ai->ai_addrlen));
        break;
    }
    freeaddrinfo(results);
    return sock;
}

const char *sockaddr_to_string(const struct sockaddr *sa, socklen_t salen) {
    static char buf[256];

    switch (sa->sa_family) {
    case AF_INET: {
        char ip[INET_ADDRSTRLEN];
        assert(salen >= sizeof(struct sockaddr_in));
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
        snprintf(buf, sizeof(buf), "%s:%hu", ip, htons(sin->sin_port));
        break;
    }
    case AF_INET6: {
        char ip6[INET6_ADDRSTRLEN];
        assert(salen >= sizeof(struct sockaddr_in6));
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        inet_ntop(AF_INET, &sin6->sin6_addr, ip6, sizeof(ip6));
        snprintf(buf, sizeof(buf), "[%s]:%hu", ip6, htons(sin6->sin6_port));
        break;
    }
    case AF_UNIX: {
        assert(salen >= sizeof(struct sockaddr_un));
        const struct sockaddr_un *sun = (const struct sockaddr_un *)sa;
        snprintf(buf, sizeof(buf), "unix://%s", sun->sun_path);
        break;
    }
    default:
        assert(0 && "Unsupported socket family");
    }
    return buf;
}

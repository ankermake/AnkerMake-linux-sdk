#ifndef _MD_CONN_H_
#define _MD_CONN_H_

struct conn_node *conn_request(const char *name, unsigned int buf_size);

void conn_release(struct conn_node *conn);

int conn_write(struct conn_node *conn, const void *buf, unsigned int size, int timeout_ms);

int conn_read(struct conn_node *conn, void *buf, unsigned int size, int timeout_ms);

void conn_wait_conn_inited(void);

#endif
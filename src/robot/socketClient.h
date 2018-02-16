#if !defined __SOCKETCLIENT_H
#define __SOCKETCLIENT_H

#define PORT_DU_SERVEUR (32145)

extern int SocketClient_connect(void);
extern void SocketClient_write(char);
extern void SocketClient_close(void);

#endif

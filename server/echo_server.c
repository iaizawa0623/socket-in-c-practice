/**
 * 	エコーサーバ
 * 	クライアントから送られたデータを
 * 	そのままクライアントに返す
 */
#include <stdio.h>	/* printf(), fgets() */
#include <stdlib.h>	/* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdint.h>	/* int32_t, uint8_t */
#include <errno.h>	/* perror() */
#include <string.h>	/* memset(), strlen() */
#include <unistd.h>	/* close() */
#include <stdbool.h>	/* true */
#include <sys/types.h>	/* 後方互換 */
#include <sys/socket.h>	/* socket(), sockaddr{} */
#include <sys/select.h>	/* selsect() */
#include <arpa/inet.h>	/* IPPROTO_TCP, sockaddr_in{} */
#include <signal.h>	/* signal() */
#include <fcntl.h>	/* fcntl() */ 

#define PORT 5000
#define MAX_CLIT 3
#define WAITING	3

int32_t g_host_sock;	/* ホストソケット */
int32_t g_host_sock = -1;
int32_t g_clit_sock[MAX_CLIT];	/* クライアントソケット */
uint32_t g_clit_sock_size;
uint32_t g_clit_sock_size = sizeof(g_clit_sock[0]);
fd_set g_read_fds;	/* 読み取るfdのリスト */

void	exit_error(char *msg);	/* エラー終了 */
void	termination(int sig);	/* 割り込みシグナル処理関数(終了する) */
int		create_listen_socket();	/* 新しく、bindされたソケットを返す */
void	set_signal(int sig, void(*f)(int));
int		echo_proc(int clit_sock);
void	accept_proc(int sig);

int main(int argc, char *argv[])
{
	int32_t i;
	int32_t ret;
	int32_t max_clit_sock;
	struct timeval tv;

	tv.tv_sec = WAITING;
	tv.tv_usec = 0;
	g_host_sock = create_listen_socket();	/* リッスン状態のソケット */
	set_signal(SIGINT, termination);	/* SIGINTのシグナルハンドラを指定 */

	for (i = 0; i <= MAX_CLIT; i++) {	/* 初期化 */
		g_clit_sock[i] = -1;
	}

	printf("接続を待っています...\n");
	while (true) {
		FD_ZERO(&g_read_fds);
		FD_SET(g_host_sock, &g_read_fds);
		max_clit_sock = g_host_sock;
		for (i = 0; i < MAX_CLIT; i++) {	/* 使われているソケットを追加 */
			if (g_clit_sock[i] > 0) {
				FD_SET(g_clit_sock[i], &g_read_fds);
				if (g_clit_sock[i] > max_clit_sock) {
					max_clit_sock = g_clit_sock[i];
				}
			}
		}
		ret = select(max_clit_sock + 1, &g_read_fds, NULL, NULL, &tv);
		if (ret == -1) {
			exit_error("select");
		} else if (ret > 0) {
			accept_proc(1);
		} else {
			write(STDOUT_FILENO, ".\n", 2);
		}
/* 		sleep(2); */
	}

	/* ここには到達しない */
}


/**
 * 	エラー終了
 */
void exit_error(char *msg) {
	int32_t err_num;
	int32_t ret;
	int32_t i;
	err_num = errno;

	if (msg != NULL) {
		perror(msg);
	}
	if (g_host_sock > 0) {
		ret = close(g_host_sock);
		if (ret == -1) {
			perror("close g_host_sock");
		}
	}
	for (i = 0; i < MAX_CLIT; i++) {
		if (g_clit_sock[i] > 0) {
			ret = close(g_clit_sock[i]);
			if (ret == -1) {
				perror("close g_clit_sock");
			}
		}
	}
	printf("終了します\n");
	exit (err_num);
}

void termination(int sig) {
	printf("\rSIGINTを受け取りました\n");
	exit_error(NULL);
}

/**
 *	 新しく、listen状態のソケットを返す
 */
int		create_listen_socket(void) {
	int32_t host_sock;
	int32_t ret;	/* 関数の返り値。主にエラーチェック */
	struct sockaddr_in host_addr;	/* ホストアドレス構造体 */
	int32_t sock_opt_value;		/* setsockopt()で使用 */
	socklen_t	sock_opt_size;	/* setsockopt()で使用 */

	/* アドレス構造体を設定 */
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* ソケット作る */
	host_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (host_sock == -1) {
		exit_error("socket host_sock");
	}
	/* 使用中のアドレスポートへのバインドを許可 */
	sock_opt_value = 1;
	sock_opt_size = sizeof(sock_opt_value);
	setsockopt(host_sock, SOL_SOCKET, SO_REUSEADDR,
			(void *)&sock_opt_value, sock_opt_size);

	/* 名前を付ける */
	ret = bind(host_sock, (struct sockaddr *)&host_addr, sizeof(host_addr));
	if (ret == -1) {
		exit_error("bind to host_sock");
	}
	listen(host_sock, 1);
	return host_sock;
}

/**
 *  sigのシグナルハンドラにfuncを指定する
 */
void	set_signal(int sig, void (*func)(int)) {
	int32_t ret;	/* 関数の返り値。主にエラーチェック */
	struct sigaction handler;	/* シグナルハンドラを指定する構造体 */

	/* シグナルハンドラ指定 */
	handler.sa_handler = func;
	/* 全シグナルをマスク */
	ret = sigfillset(&handler.sa_mask);
	if (ret == -1) {
		exit_error("sigfillset");
	}
	/* フラグなし */
	handler.sa_flags = 0;
	/* 割り込みシグナルに対する処理を設定 */
	ret = sigaction(sig, &handler, 0);
	if (ret == -1) {
		exit_error("sigaction");
	}
}

/**
 *  受け取ったソケットからメッセージを受け取り
 *  エコーバックする
 */
int echo_proc(int clit_sock) {
	uint8_t buf[BUFSIZ + 1];	/* 受け取ったメッセージ */
	uint8_t *p_buf;
	int32_t ret;	/* 関数の返り値。主にエラーチェック */

	memset(buf, '\0', BUFSIZ);
	p_buf = buf;
	ret = recv(clit_sock, p_buf, BUFSIZ, 0);
	if (ret == -1) {
		exit_error("recv in echo_proc");
	} else if (ret == 0) { /* クライアントが終了したと判断 */
		return 1;
	} else {	/* 何らかの入力があった */
		buf[ret] = '\0';
	}
	printf("受信: %s\n", buf);
	ret = send(clit_sock, buf, strlen((char *)buf) + 1, 0);
	if (ret == -1) {
		exit_error("recv");
	}
	return 0;
}

/**
 * 	新規接続があればアセプト、
 * 	接続中のソケットにメッセージが送られた時
 * 	エコーバック関数を呼び出す
 * 	グローバルのg_read_fdsを使用
 */
void accept_proc(int sig) {
	int32_t ret;	/* 関数の返り値。主にエラーチェック */
	int32_t i;		/* ループイテレータ */
	static int32_t clit_num = 0;
	static struct sockaddr_in clit_addr[MAX_CLIT];	/* クライアントアドレス構造体(use on accept) */

	/* 新規接続の確認 */
	if (FD_ISSET(g_host_sock, &g_read_fds)) {
		i = 0;
		while (g_clit_sock[i] != -1) {
			i++;
		}
		g_clit_sock[i] = accept(g_host_sock, (struct sockaddr *)&clit_addr[i], &g_clit_sock_size);
		printf("新たなクライアントが接続しました(sock:%d ip:%s port:%d)\n",
				g_clit_sock[i], inet_ntoa(clit_addr[i].sin_addr), clit_addr[i].sin_port);
		clit_num++;	/* クライアントを増やす */
	}
	/* 接続中のソケットをチェック */
	for (i = 0; i < MAX_CLIT; i++) {
		if (FD_ISSET(g_clit_sock[i], &g_read_fds)) {
			ret = echo_proc(g_clit_sock[i]);
			if (ret == 1) {	/* クライアントが終了 */
				printf("クライアントが切断しました(sock:%d ip:%s port:%d)\n",
						g_clit_sock[i], inet_ntoa(clit_addr[i].sin_addr),clit_addr[i].sin_port );
				ret = close(g_clit_sock[i]);
				if (ret == -1) {
					perror("close sock");
				}
				g_clit_sock[i] = -1;
				clit_num--;
				if (g_host_sock == -1) {	/* 接続受付再開 */
					g_host_sock = create_listen_socket();
					printf("接続受付を再開します\n");
				}
			}
		}
	}	
	printf("現在の接続数:%d\n", clit_num);
	if (clit_num == MAX_CLIT) {	/* もうつなげない */
		if (g_host_sock != -1) {
			close(g_host_sock);
			g_host_sock = -1;
			printf("接続受付を一時停止\n");
		}
	}
	/* while(true) */
}


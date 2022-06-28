/**
 * 	エコークライアント
 * 	ユーザ入力のメッセージをエコーサーバに送り
 * 	その返信を出力する
 */
#include <stdio.h>	/* printf(), fgets() */
#include <stdlib.h>	/* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdint.h>	/* int32_t, uint8_t */
#include <errno.h>	/* perror() */
#include <string.h>	/* strstr(), strcmp(), memset() */
#include <unistd.h>	/* close() */
#include <stdbool.h>	/* true */
#include <sys/types.h>	/* 後方互換 */
#include <sys/socket.h>	/* socket(), sockaddr{} */
#include <arpa/inet.h>	/* IPPROTO_TCP, sockaddr_in{} */ 
#include <sys/select.h>	/* select() */
#include <fcntl.h>	/* fcntl() */
#include <signal.h>	/* signal() */

#define PORT 5000

int32_t host_sock;	/* ホストソケット */

int main(int argc, char *argv[])
{
	int32_t ret;		/* 関数の返り値。主にエラーチェック */
	int32_t msg_len;
	struct sockaddr_in host_addr;	/* ホストアドレス構造体 */
	uint8_t msg[BUFSIZ + 1];	/* 送信するメッセージ */
	uint8_t buf[BUFSIZ + 1];	/* サーバからの返信 */
	uint8_t *p_buf;
	fd_set	readfds;

	/* アドレス構造体を設定 */
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* ソケット作る */
	host_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* 接続 */
	printf("接続を試みます\n");
	ret = connect(host_sock, (struct sockaddr *)&host_addr, sizeof(host_addr));
	if (ret == -1) {
		if (errno == ECONNREFUSED) {
			printf("サーバに接続を拒否されました\n");
			exit(ECONNREFUSED);
		}
		perror("connect");
		exit(errno);
	} else {
		printf("接続されました\n");
	}

	/* ノンブロッキング */
	ret = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	ret = fcntl(host_sock, F_SETFL, O_NONBLOCK);

	/* 送信と受信を繰り返す */
	while (true) {
		memset(msg, '\0', BUFSIZ);
		write(STDOUT_FILENO, ">", 1);

		/* host_sockとSTDIN_FILENOを追加 */
		FD_ZERO(&readfds);
		FD_SET(host_sock, &readfds);
		FD_SET(STDIN_FILENO, &readfds);
		ret = select(host_sock + 1, &readfds, NULL, NULL, NULL);	/* いずれかから入力があるまで待つ */

		/* サーバから受け取り */
		if (FD_ISSET(host_sock, &readfds)) {
			memset(buf, '\0', BUFSIZ);
			p_buf = buf;
			do {	/* '\0'を受け取るまで */
				ret = recv(host_sock, p_buf, 1, 0);
				if (ret == -1) {
					if (errno == ENOTCONN) {
						printf("サーバが終了しました\n");
						exit(ENOTCONN);
					}
					perror("recv");
					exit(errno);
				} else if (ret == 0) {
					printf("サーバが終了しました\n");
					exit(ENOTCONN);
				}
			} while (*p_buf++ != '\0');
			if (*buf != '\0') {
				printf("\bエコーバック: %s\n", buf);
			} else {
				write(STDOUT_FILENO, "\r", 1);
			}
		}

		/* 標準入力から受け取り */
		if (FD_ISSET(STDIN_FILENO, &readfds)) {
			read(STDOUT_FILENO, (char *)msg, BUFSIZ);
			if ((p_buf = (uint8_t *)strstr((char *)msg, "\n")) != NULL) {
				*p_buf = '\0'; /* 改行を消す */
			}
			/* メッセージを送信 */
			msg_len = strlen((char *)msg);
			ret = send(host_sock, msg, msg_len + 1, 0);	/* '\0'まで送る */
			if (ret == -1) {
				perror("send");
				exit(errno);
			}
		}
	}
	close(host_sock);
    return EXIT_SUCCESS;
}

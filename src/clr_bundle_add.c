/*
 *   Software Updater - client side
 *
 *      Copyright (c) 2012-2016 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Authors:
 *         Arjan van de Ven <arjan@linux.intel.com>
 *         Tim Pepper <timothy.c.pepper@linux.intel.com>
 *         Jaime A. Garcia <jaime.garcia.naranjo@linux.intel.com>
 *
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>

#include "swupd.h"

#define MODE_RW_O (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define VERIFY_NOPICKY 0

bool list = false;
static char **bundles;

static void print_help(const char *name)
{
	printf("Usage:\n");
	printf("   swupd %s [options] [bundle1 bundle2 (...)]\n\n", basename((char *)name));
	printf("Help Options:\n");
	printf("   -h, --help              Show help options\n");
	printf("   -u, --url=[URL]         RFC-3986 encoded url for version string and content file downloads\n");
	printf("   -P, --port=[port #]        Port number to connect to at the url for version string and content file downloads\n");
	printf("   -p, --path=[PATH...]    Use [PATH...] as the path to verify (eg: a chroot or btrfs subvol\n");
	printf("   -F, --format=[staging,1,2,etc.]  the format suffix for version file downloads\n");
	printf("   -l, --list              List all available bundles for the current version of Clear Linux\n");
	printf("   -x, --force             Attempt to proceed even if non-critical errors found\n");
	printf("\n");
}

static const struct option prog_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "url", required_argument, 0, 'u' },
	{ "port", required_argument, 0, 'P' },
	{ "list", no_argument, 0, 'l' },
	{ "path", required_argument, 0, 'p' },
	{ "format", required_argument, 0, 'F' },
	{ "force", no_argument, 0, 'x' },
	{ 0, 0, 0, 0 }
};

static bool parse_options(int argc, char **argv)
{
	int opt;

	set_format_string(NULL);

	while ((opt = getopt_long(argc, argv, "hxu:P:p:F:l", prog_opts, NULL)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'u':
			if (!optarg) {
				printf("error: invalid --url argument\n\n");
				goto err;
			}
			if (version_server_urls[0]) {
				free(version_server_urls[0]);
			}
			if (content_server_urls[0]) {
				free(content_server_urls[0]);
			}
			string_or_die(&version_server_urls[0], "%s", optarg);
			string_or_die(&content_server_urls[0], "%s", optarg);
			break;
		case 'p': /* default empty path_prefix verifies the running OS */
			if (!optarg) {
				printf("Invalid --path argument\n\n");
				goto err;
			}
			if (path_prefix) { /* multiple -p options */
				free(path_prefix);
			}
			string_or_die(&path_prefix, "%s", optarg);
			break;
		case 'P':
			if (sscanf(optarg, "%ld", &update_server_port) != 1) {
				printf("Invalid --port argument\n\n");
				goto err;
			}
			break;
		case 'F':
			if (!optarg || !set_format_string(optarg)) {
				printf("Invalid --format argument\n\n");
				goto err;
			}
			break;
		case 'l':
			list = true;
			break;
		case 'x':
			force = true;
			break;
		default:
			printf("error: unrecognized option\n\n");
			goto err;
		}
	}

	if (!list) {
		if (argc <= optind) {
			printf("error: missing bundle(s) to be installed\n\n");
			goto err;
		}

		bundles = argv + optind;
	}

	return true;
err:
	print_help(argv[0]);
	return false;
}

int bundle_add_main(int argc, char **argv)
{
	copyright_header("bundle adder");

	if (!parse_options(argc, argv)) {
		return EXIT_FAILURE;
	}

	if (list) {
		return list_installable_bundles();
	} else {
		return install_bundles(bundles);
	}
}

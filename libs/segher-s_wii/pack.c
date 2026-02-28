// Copyright 2007-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#define _DEFAULT_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tools.h"

static int verbose = 3;

#define MAXFILES 1000

#define ERROR(s)                 \
	do                           \
	{                            \
		fprintf(stderr, s "\n"); \
		exit(1);                 \
	} while (0)

static u8 sd_key[16];
static u8 sd_iv[16];
static u8 md5_blanker[16];

static u32 ng_id;
static u32 ng_key_id;
static u8 ng_mac[6];
static u8 ng_priv[30];
static u8 ng_sig[60];

static FILE *fp;

static u8 header[0xf0c0];

static u32 n_files;
static u32 files_size;

static u8 files[MAXFILES][0x80];
static char src[MAXFILES][256];

static void read_image(u8 *data, u32 w, u32 h, const char *name)
{
	FILE *fp;
	u32 x, y;
	u32 ww, hh;

	fp = fopen(name, "rb");
	if (!fp)
		fatal("open %s", name);

	if (fscanf(fp, "P6 %d %d 255", &ww, &hh) != 2)
		ERROR("bad ppm");
	if (getc(fp) != '\n')
		ERROR("bad ppm");
	if (ww != w || hh != h)
		ERROR("wrong size ppm");

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++)
		{
			u8 pix[3];
			u16 raw;
			u32 x0, x1, y0, y1, off;

			x0 = x & 3;
			x1 = x >> 2;
			y0 = y & 3;
			y1 = y >> 2;
			off = x0 + 4 * y0 + 16 * x1 + 4 * w * y1;

			if (fread(pix, 3, 1, fp) != 1)
				fatal("read %s", name);

			raw = (pix[0] & 0xf8) << 7;
			raw |= (pix[1] & 0xf8) << 2;
			raw |= (pix[2] & 0xf8) >> 3;
			raw |= 0x8000;

			wbe16(data + 2 * off, raw);
		}

	fclose(fp);
}

static u8 perm_from_path(const char *path)
{
	struct stat sb;
	mode_t mode;
	u8 perm;
	u32 i;

	if (stat(path, &sb))
		fatal("stat %s", path);

	perm = 0;
	mode = sb.st_mode;
	for (i = 0; i < 3; i++)
	{
		perm <<= 2;
		if (mode & 0200)
			perm |= 2;
		if (mode & 0400)
			perm |= 1;
		mode <<= 3;
	}

	return perm;
}

static void do_main_header(u64 title_id, FILE *toc)
{

	// u8 mainHeader[0xf0c0];
	// memset(mainHeader, '\0', sizeof(mainHeader));
	memset(header, '\0', sizeof(header));

	// wbe64(mainHeader, sg.tid);
	// wbe32(mainHeader+8, bnr.second);

	wbe64(header, title_id);

	char name[256];
	FILE *in;

	if (toc)
	{
		if (!fgets(name, sizeof name, toc))
			fatal("reading banner.bin");
		name[strlen(name) - 1] = 0; // get rid of linefeed
	}
	else
		strcpy(name, "banner.bin");

	// u8 tmp8 = AttrFromSave("/banner.bin");
	// header[0xc] = (tmp8 >> 2);
	u8 perm = perm_from_path(name);
	header[0x0c] = (toc ? 0x35 : perm);
	u8 tmp8 = 0;
	header[0xd] = tmp8;

	// we need the size of banner
	struct stat sb;
	u32 banner_size = 0;
	if (stat(name, &sb))
		fatal("stat %s", name);
	banner_size = sb.st_size;

	wbe32(header + 8, banner_size);

	in = fopen(name, "rb");
	if (!in)
		fatal("open %s", name);
	if (fread(header + 0x20, banner_size, 1, in) != 1)
		fatal("read %s", name);
	fclose(in);

	if (verbose)
		printf(
			"file: size=%08x perm=%02x attr=%02x type=%02x name=%s\n",
			banner_size, perm, 0, 1, name);

	// u8 md5blanker[16] = MD5_BLANKER;
	// memcpy(header+0xe, md5blanker, 16);
	memcpy(header + 0xe, md5_blanker, 16);

	// memcpy(header+0x20, bnr.first, bnr.second);
	//  Already read

	/*
	u8 tmpHeader[0xf0c0];
	memset(tmpHeader, '\0', sizeof(tmpHeader));
	memcpy(tmpHeader, header, sizeof(tmpHeader));

	MD5 hash;
	hash.update(header, sizeof(header));
	hash.finalize();
	memcpy(tmpHeader+0x0e, hash.hexdigestChar(), 16);

	//!encrypt the header
	memset(header, '\0', sizeof(header));
	u8 iv[16] = SD_IV;
	u8 sdkey[16] = SD_KEY;
	aes_set_key(sdkey);
	aes_encrypt(iv, tmpHeader, header, 0xf0c0);
*/

	u8 md5_calc[16];
	md5(header, sizeof header, md5_calc);
	memcpy(header + 0x0e, md5_calc, 16);
	aes_cbc_enc(sd_key, sd_iv, header, sizeof header, header);

	if (fwrite(header, 0xf0c0, 1, fp) != 1)
		fatal("write main header");
}

 [[maybe_unused]] static void do_file_header(u64 title_id, FILE *toc)
{
	memset(header, 0, sizeof header);

	wbe64(header, title_id);
	header[0x0c] = (toc ? 0x35 : perm_from_path("."));
	memcpy(header + 0x0e, md5_blanker, 16);
	memcpy(header + 0x20, "WIBN", 4);
	// XXX: what about the stuff at 0x24?

	char name[256];
	FILE *in;

	if (toc)
	{
		if (!fgets(name, sizeof name, toc))
			fatal("reading title file name");
		name[strlen(name) - 1] = 0; // get rid of linefeed
	}
	else
		strcpy(name, "###title###");

	in = fopen(name, "rb");
	if (!in)
		fatal("open %s", name);
	if (fread(header + 0x40, 0x80, 1, in) != 1)
		fatal("read %s", name);
	fclose(in);

	if (toc)
	{
		if (!fgets(name, sizeof name, toc))
			fatal("reading banner file name");
		name[strlen(name) - 1] = 0; // get rid of linefeed
	}
	else
		strcpy(name, "###banner###.ppm");

	read_image(header + 0xc0, 192, 64, name);

	if (toc)
	{
		if (!fgets(name, sizeof name, toc))
			fatal("reading icon file name");
		name[strlen(name) - 1] = 0; // get rid of linefeed
	}
	else
		strcpy(name, "###icon###.ppm");

	int have_anim_icon = 0;

	if (toc == 0)
	{
		in = fopen(name, "rb");
		if (in)
			fclose(in);
		else
			have_anim_icon = 1;
	}

	if (!have_anim_icon)
	{
		wbe32(header + 8, 0x72a0);
		read_image(header + 0x60c0, 48, 48, name);
	}
	else
	{
		u32 i;
		for (i = 0; i < 8; i++)
		{
			snprintf(name, sizeof name, "###icon%d###.ppm", i);
			FILE *fp = fopen(name, "rb");
			if (fp)
			{
				fclose(fp);
				read_image(header + 0x60c0 + 0x1200 * i, 48, 48, name);
			}
			else
				break;
		}

		wbe32(header + 8, 0x60a0 + 0x1200 * i);
	}

	u8 md5_calc[16];
	md5(header, sizeof header, md5_calc);
	memcpy(header + 0x0e, md5_calc, 16);
	aes_cbc_enc(sd_key, sd_iv, header, sizeof header, header);

	if (fwrite(header, 0xf0c0, 1, fp) != 1)
		fatal("write header");
}

static void find_files_recursive(const char *path)
{
	DIR *dir;
	struct dirent *de;
	char name[53];
	u32 len;
	int is_dir;
	u8 *p;
	struct stat sb;
	u32 size;

	dir = opendir(path ? path : ".");
	if (!dir)
		fatal("opendir %s", path ? path : ".");

	while ((de = readdir(dir)))
	{
		if (strcmp(de->d_name, ".") == 0)
			continue;
		if (strcmp(de->d_name, "..") == 0)
			continue;
		if (strncmp(de->d_name, "###", 3) == 0)
			continue;
		if (strncmp(de->d_name, "banner.bin", 10) == 0)
			continue;

		if (path == 0)
			len = snprintf(name, sizeof name, "%s", de->d_name);
		else
			len = snprintf(name, sizeof name, "%s/%s", path,
						   de->d_name);

		if (len >= sizeof name)
			ERROR("path too long");

		if (de->d_type != DT_REG && de->d_type != DT_DIR)
			ERROR("not a regular file or a directory");

		is_dir = (de->d_type == DT_DIR);

		if (is_dir)
			size = 0;
		else
		{
			if (stat(name, &sb))
				fatal("stat %s", name);
			size = sb.st_size;
		}

		strcpy(src[n_files], name);

		p = files[n_files++];
		wbe32(p, 0x3adf17e);
		wbe32(p + 4, size);
		p[8] = perm_from_path(name);
		p[0x0a] = is_dir ? 2 : 1;
		strcpy((char *)p + 0x0b, name);
		// maybe fill up with dirt

		size = round_up(size, 0x40);
		files_size += 0x80 + size;

		if (de->d_type == DT_DIR)
			find_files_recursive(name);
	}

	if (closedir(dir))
		fatal("closedir");
}

static int compar_files(const void *a, const void *b)
{
	return strcmp((char *)a + 0x0b, (char *)b + 0x0b);
}

static int compar_src(const void *a, const void *b)
{
	return strcmp((char *)a , (char *)b );
}

static void find_files(void)
{
	n_files = 0;
	files_size = 0;

	memset(files, 0, sizeof files);

	find_files_recursive(0);

	qsort(files, n_files, 0x80, compar_files);
	// qsort src is also needed
	qsort(src, n_files, 0x100, compar_src);
}

static u32 wiggle_name(char *name)
{
	// XXX: encode embedded zeroes, etc.
	return strlen(name);
}

static void find_files_toc(FILE *toc)
{
	n_files = 0;
	files_size = 0;

	memset(files, 0, sizeof files);

	u32 len;
	int is_dir;
	u8 *p;
	struct stat sb;
	u32 size;

	char line[256];

	while (fgets(line, sizeof line, toc))
	{
		line[strlen(line) - 1] = 0; // get rid of linefeed

		char *name;
		for (name = line; *name; name++)
			if (*name == ' ')
				break;
		if (!*name)
			ERROR("no space in TOC line");
		*name = 0;
		name++;

		len = wiggle_name(name);
		if (len >= 53)
			ERROR("path too long");

		if (stat(line, &sb))
			fatal("stat %s", line);

		is_dir = S_ISDIR(sb.st_mode);

		size = (is_dir ? 0 : sb.st_size);

		strcpy(src[n_files], line);

		p = files[n_files++];
		wbe32(p, 0x3adf17e);
		wbe32(p + 4, size);
		p[8] = 0x35; // rwr-r-
		p[0x0a] = is_dir ? 2 : 1;
		memcpy(p + 0x0b, name, len);
		// maybe fill up with dirt

		size = round_up(size, 0x40);
		files_size += 0x80 + size;

		// if (is_dir)
		//	find_files_recursive(name);
	}

	if (ferror(toc))
		fatal("reading toc");
}

static void do_backup_header(u64 title_id)
{
	u8 header[0x80];

	memset(header, 0, sizeof header);

	wbe32(header, 0x70);
	wbe32(header + 4, 0x426b0001);
	wbe32(header + 8, ng_id);
	wbe32(header + 0x0c, n_files);
	wbe32(header + 0x10, files_size);
	wbe32(header + 0x1c, files_size + 0x3c0);

	wbe64(header + 0x60, title_id);
	memcpy(header + 0x68, ng_mac, 6);

	if (fwrite(header, sizeof header, 1, fp) != 1)
		fatal("write Bk header");
}

static void do_file(u32 file_no)
{
	u8 *header;
	u32 size;
	u32 rounded_size;
	u8 perm, attr, type;
	char *name;
	u8 *data;
	FILE *in;

	header = files[file_no];

	size = be32(header + 4);
	perm = header[8];
	attr = header[9];
	type = header[10];
	name = (char *)header + 11;

	if (verbose)
		printf(
			"file: size=%08x perm=%02x attr=%02x type=%02x name=%s\n",
			size, perm, attr, type, name);

	if (fwrite(header, 0x80, 1, fp) != 1)
		fatal("write file header %d", file_no);

	char *from = src[file_no];

	if (type == 1)
	{
		rounded_size = round_up(size, 0x40);

		data = malloc(rounded_size);
		if (!data)
			fatal("malloc data");

		in = fopen(from, "rb");
		if (!in)
			fatal("open %s", from);
		if (fread(data, size, 1, in) != 1)
			fatal("read %s", from);
		fclose(in);

		memset(data + size, 0, rounded_size - size);

		aes_cbc_enc(sd_key, header + 0x50, data, rounded_size, data);

		if (fwrite(data, rounded_size, 1, fp) != 1)
			fatal("write file %d", file_no);

		free(data);
	}
}

static void make_ec_cert(u8 *cert, u8 *sig, char *signer, char *name, u8 *priv,
						 u32 key_id)
{
	memset(cert, 0, 0x180);
	wbe32(cert, 0x10002);
	memcpy(cert + 4, sig, 60);
	strcpy((char *)cert + 0x80, signer);
	wbe32(cert + 0xc0, 2);
	strcpy((char *)cert + 0xc4, name);
	wbe32(cert + 0x104, key_id);
	ec_priv_to_pub(priv, cert + 0x108);
}

static void do_sig(void)
{
	u8 sig[0x40];
	u8 ng_cert[0x180];
	u8 ap_cert[0x180];
	u8 hash[0x14];
	u8 ap_priv[30];
	u8 ap_sig[60];
	char signer[64];
	char name[64];
	u8 *data;
	u32 data_size;

	sprintf(signer, "Root-CA00000001-MS00000002");
	sprintf(name, "NG%08x", ng_id);
	make_ec_cert(ng_cert, ng_sig, signer, name, ng_priv, ng_key_id);

	memset(ap_priv, 0, sizeof ap_priv);
	ap_priv[10] = 1;

	memset(ap_sig, 81, sizeof ap_sig); // temp

	sprintf(signer, "Root-CA00000001-MS00000002-NG%08x", ng_id);
	sprintf(name, "AP%08x%08x", 1, 2);
	make_ec_cert(ap_cert, ap_sig, signer, name, ap_priv, 0);

	sha(ap_cert + 0x80, 0x100, hash);
	generate_ecdsa(ap_sig, ap_sig + 30, ng_priv, hash);
	make_ec_cert(ap_cert, ap_sig, signer, name, ap_priv, 0);

	data_size = files_size + 0x80;

	data = malloc(data_size);
	if (!data)
		fatal("malloc");
	fseek(fp, 0xf0c0, SEEK_SET);
	if (fread(data, data_size, 1, fp) != 1)
		fatal("read data for sig check");
	sha(data, data_size, hash);
	sha(hash, 20, hash);
	free(data);

	generate_ecdsa(sig, sig + 30, ap_priv, hash);
	wbe32(sig + 60, 0x2f536969);

	if (fwrite(sig, sizeof sig, 1, fp) != 1)
		fatal("write sig");
	if (fwrite(ng_cert, sizeof ng_cert, 1, fp) != 1)
		fatal("write NG cert");
	if (fwrite(ap_cert, sizeof ap_cert, 1, fp) != 1)
		fatal("write AP cert");
}

int pack(int argc, char **argv)
{

	u64 title_id;
	u8 tmp[4];
	u32 i;

	if (argc != 3 && argc != 4)
	{
		fprintf(stderr, "Usage: %s <srcdir> <data.bin>\n", argv[0]);
		fprintf(stderr, "or: %s <srcdir> <data.bin> <toc>\n", argv[0]);
		return 1;
	}

	FILE *toc = 0;
	if (argc == 4)
	{
		toc = fopen(argv[3], "r");
		if (!toc)
			fatal("open %s", argv[3]);
	}

	get_key("sd-key", sd_key, 16);
	get_key("sd-iv", sd_iv, 16);
	get_key("md5-blanker", md5_blanker, 16);

	get_key("default/NG-id", tmp, 4);
	ng_id = be32(tmp);
	get_key("default/NG-key-id", tmp, 4);
	ng_key_id = be32(tmp);
	get_key("default/NG-mac", ng_mac, 6);
	get_key("default/NG-priv", ng_priv, 30);
	get_key("default/NG-sig", ng_sig, 60);

	if (sscanf(argv[1], "%016llx", &title_id) != 1)
		ERROR("not a correct title id");

	fp = fopen(argv[2], "wb+");
	if (!fp)
		fatal("open %s", argv[2]);

	if (!toc)
	{
		if (chdir(argv[1]))
			fatal("chdir %s", argv[1]);
	}

	do_main_header(title_id, toc);

	if (toc)
		find_files_toc(toc);
	else
		find_files();

	do_backup_header(title_id);

	for (i = 0; i < n_files; i++)
		do_file(i);

	// XXX: is this needed?
	if (!toc)
	{
		if (chdir(".."))
			fatal("chdir ..");
	}

	do_sig();

	fclose(fp);

	return 0;
}

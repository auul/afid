#include "afid.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool file_exists (char *filename)
{
	FILE *f = fopen(filename, "r");
	if (f) {
		fclose(f);
		return true;
	} else return false;
}

bool touch (char *filename)
{
	FILE *f = fopen(filename, "w");
	if (f) {
		fclose(f);
		return true;
	} else return false;
}

FILE *lock_file (l_file file, char *mode)
{
	while (file_exists(file.lock));

	if (!touch(file.lock)) {
		fprintf(ERR LOCK);
		return NULL;
	}

	if (!(file.ptr = fopen(file.name, mode))) {
		remove(file.lock);
		fprintf(ERR FILEACCESS);
		return NULL;
	}

	return file.ptr;
}

void unlock_file (l_file *file)
{
	if (file->ptr) {
		fclose(file->ptr);
		file->ptr = NULL;
		remove(file->lock);
	}
}

char *str_append (char *a, char *b)
{
	size_t len_a = a ? strlen(a) : 0;
	size_t len_b = b ? strlen(b) : 0;
	char *str = malloc(len_a + len_b + 1);
	if (!str) {
		fprintf(ERR ALLOC);
		return NULL;
	}

	for (size_t i = 0; i < len_a; i++)
		str[i] = a[i];
	for (size_t i = 0; i < len_b; i++)
		str[len_a + i] = b[i];
	str[len_a + len_b] = 0;

	return str;
}

#define str_clone(str) str_append(str, NULL)

bool destroy_l_file (l_file *file)
{
	if (file->ptr) {
		fclose(file->ptr);
		file->ptr = NULL;
	}

	if (file->name) {
		remove(file->name);
		free(file->name);
		file->name = NULL;
	}

	if (file->lock) {
		remove(file->lock);
		free(file->lock);
		file->lock = NULL;
	}

	return false;
}

bool create_l_file (l_file *file, char *name)
{
	file->name = NULL;
	file->lock = NULL;
	file->ptr  = NULL;

	if (!name && !(name = tmpnam(NULL))) {
		fprintf(ERR TMPFS);
		return destroy_l_file(file);
	}

	if (!(file->name = str_clone(name)))
		return destroy_l_file(file);
	if (!(file->lock = str_append(name, ".lock")))
		return destroy_l_file(file);
	if (!touch(file->name))
		return destroy_l_file(file);
	if (!touch(file->lock))
		return destroy_l_file(file);

	return true;
}

afid_hub *afid_destroy (afid_hub *hub)
{
	if (hub) {
		destroy_l_file(&hub->file);
		destroy_l_file(&hub->out);
		destroy_l_file(&hub->in);
		free(hub);
	}

	return NULL;
}

bool write_hub (afid_hub *hub)
{
	FILE *f = fopen(hub->file.name, "w");
	if (!f) {
		fprintf(ERR FILEACCESS);
		return false;
	}

	fprintf(f, "afid-hub\nhub-lock %s\n",    hub->file.lock);
	fprintf(f, "out-file %s\nout-lock %s\n", hub->out.name, hub->out.lock);
	fprintf(f, "in-file  %s\nin-lock  %s",   hub->in.name,  hub->in.lock);

	fclose(f);

	return true;
}

afid_hub *afid_create (char *hubfile, char *outfile, char *infile)
{
	afid_hub *hub = calloc(1, sizeof(afid_hub));
	if (!hub) {
		fprintf(ERR ALLOC);
		return afid_destroy(hub);
	}

	if (!create_l_file(&hub->file, hubfile))
		return afid_destroy(hub);
	if (!create_l_file(&hub->out,  outfile))
		return afid_destroy(hub);
	if (!create_l_file(&hub->in,   infile))
		return afid_destroy(hub);

	remove(hub->file.name);
	remove(hub->file.lock);
	remove(hub->out.name);
	remove(hub->out.lock);
	remove(hub->in.name);
	remove(hub->in.lock);

	if (!write_hub(hub))
		return afid_destroy(hub);

	return hub;
}

void find_token (FILE *f)
{
	while (isspace(fgetc(f)));
	if (!feof(f)) fseek(f, -1, SEEK_CUR);
}

void skip_token (FILE *f)
{
	while (!feof(f) && !isspace(fgetc(f)));
}

void skip_line (FILE *f)
{
	while (!feof(f) && fgetc(f) != '\n');
}

bool match_token (FILE *f, char *token)
{
	size_t hold = ftell(f);

	while (fgetc(f) == *token)
		token++;
	fseek(f, hold, SEEK_SET);
	if (*token) {
		return false;
	} else return true;
}

char *grab_line (FILE *f)
{
	size_t start = ftell(f);

	while (!feof(f) && fgetc(f) != '\n');

	size_t len = ftell(f) - start;
	char *str = malloc(len + 1);
	if (!str) {
		fprintf(ERR ALLOC);
		return NULL;
	}

	fseek(f, start, SEEK_SET);
	for (size_t i = 0; i < len; i++)
		str[i] = fgetc(f);
	str[len] = 0;

	return str;
}

afid_hub *afid_disconnect (afid_hub *hub)
{
	if (hub) {
		if (hub->file.name)
			free(hub->file.name);
		if (hub->file.ptr) {
			fclose(hub->file.ptr);
			if (hub->file.lock) {
				remove(hub->file.lock);
				free(hub->file.lock);
			}
		}
		if (hub->out.name)
			free(hub->out.name);
		if (hub->out.ptr) {
			fclose(hub->out.ptr);
			if (hub->out.lock) {
				remove(hub->out.lock);
				free(hub->out.lock);
			}
		}
		if (hub->in.name)
			free(hub->in.name);
		if (hub->in.ptr) {
			fclose(hub->in.ptr);
			if (hub->in.lock) {
				remove(hub->in.lock);
				free(hub->in.lock);
			}
		}
		free(hub);
	}

	return NULL;
}

char *lookup (FILE *f, char *key)
{
	fseek(f, 0, SEEK_SET);

	while (!feof(f)) {
		find_token(f);
		if (match_token(f, key)) {
			skip_token(f);
			find_token(f);
			return grab_line(f);
		} else skip_line(f);
	}

	return NULL;
}

afid_hub *afid_connect (char *hubfile)
{
	FILE *f = fopen(hubfile, "r");
	if (!f) {
		fprintf(ERR FILEACCESS);
		return NULL;
	}

	find_token(f);
	if (!match_token(f, "afid-hub")) {
		fprintf(ERR HUBFILE);
		fclose(f);
		return NULL;
	}

	afid_hub *hub = calloc(1, sizeof(afid_hub));
	if (!hub) {
		fprintf(ERR ALLOC);
		return afid_disconnect(hub);;
	}

	if (!(hub->file.name = str_clone(hubfile)))
		return afid_disconnect(hub);
	hub->file.name = str_clone(hubfile);
	hub->file.lock = lookup(f, "hub-lock");
	hub->out.name = lookup(f, "out-file");
	hub->out.lock = lookup(f, "out-lock");
	hub->in.name = lookup(f, "in-file");
	hub->in.lock = lookup(f, "in-lock");

	if (!hub->file.name || !hub->file.lock ||
	    !hub->out.name  || !hub->out.lock  ||
	    !hub->in.name   || !hub->in.lock) {
		fprintf(ERR INC_HUBFILE);
		return afid_disconnect(hub);
	}

	return hub;
}

bool afid_msg (afid_hub *hub, size_t *size)
{
	FILE *f;

	if (file_exists(hub->in.name) &&
	    !file_exists(hub->in.lock)) {
		if (size) {
			if (!(f = fopen(hub->in.name, "rb"))) {
				fprintf(ERR FILEACCESS);
				return false;
			}
			fseek(f, 0, SEEK_END);
			*size = ftell(f);
			fclose(f);
			return true;
		} else return true;
	} else return false;
}

FILE *afid_lock (afid_hub *hub, char *mode)
{
	FILE *f;

	switch (mode[0]) {
		case 'r':
			return lock_file(hub->in, mode);
		case 'w':
		case 'a':
			return lock_file(hub->out, mode);
		default:
			fprintf(ERR MODE);
			return NULL;
	}
}

void afid_unlock (afid_hub *hub)
{
	unlock_file(&hub->file);
	unlock_file(&hub->out);
	unlock_file(&hub->in);
}

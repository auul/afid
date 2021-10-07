#ifndef AFID_H
#define AFID_H

#include <stdio.h>
#include <stdbool.h>

#define ERR         stderr, "afid error: "
#define ALLOC       "unable to allocate memory.\n"
#define LOCK        "unable to create lock file.\n"
#define FILEACCESS  "unable to access file.\n"
#define TMPFS       "unable to access /tmp/.\n"
#define HUBFILE     "improperly formatted hub file.\n"
#define INC_HUBFILE "hub file is incomplete.\n"
#define MODE        "unknown file access mode.\n"

typedef struct l_file {
	char *name;
	char *lock;
	FILE *ptr;
} l_file;

typedef struct afid_hub {
	l_file file;
	l_file out;
	l_file in;
} afid_hub;

afid_hub *afid_create (char *hubfile, char *outfile, char *infile);
afid_hub *afid_destroy (afid_hub *hub);
afid_hub *afid_connect (char *hubfile);
afid_hub *afid_disconnect (afid_hub *hub);
bool afid_msg (afid_hub *hub, size_t *size);
FILE *afid_lock (afid_hub *hub, char *mode);
void afid_unlock (afid_hub *hub);

#endif

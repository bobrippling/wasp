#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fatal.h"

#include WASP_ARCH_FNAME

static void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-o output] [files...]\n", argv0);
	exit(1);
}

static void assemble_fname(const char *fname, FILE *fout)
{
	FILE *f = fopen(fname, "r");
	if(!f)
		fatal("open %s:", fname);

	assemble_file(f, fout);

	fclose(f);
}

int main(int argc, const char *argv[])
{
	const char *output = "a.out";
	FILE *fout;
	int i;

	for(i = 1; i < argc && argv[i][0] == '-'; i++){
		if(!strcmp(argv[i], "-o")){
			i++;
			if(i == argc){
				fprintf(stderr, "%s: '%s' needs an argument\n", argv[i-1], argv[0]);
				usage(argv[0]);
			}
			output = argv[i];
		}else{
			usage(argv[0]);
		}
	}

	fout = fopen(output, "w");
	if(!fout)
		fatal("open %s:", output);

	if(i < argc){
		for(; i < argc; i++)
			assemble_fname(argv[i], fout);
	}else{
		assemble_file(stdin, fout);
	}

	return 0;
}

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "snarf.h"

int opterr;
int optopt;
int optind;
char *optarg;

char *url_to_snarf;
char *output_file;

void
parse_args(int argc, char *argv[])
{
  int i;
  char option;
  int fileSet = 0;

  for (i = 0; optind < argc; i++) {
    info("%d opterr: %d\n", i, opterr);
    info("%d optind: %d\n", i, optind);
    info("%d optopt: %d\n", i, optopt);
    info("%d argv[optind]: %s\n", i, argv[optind]);
    if ((option = getopt(argc, argv, "+q:o:")) != -1) {
        info("option: %c\n", option);
      switch (option) {
        case 'q': {
          info("Query header: %s\n", optarg);
          break;
        }
        case 'o': {
            if (fileSet != 0){
                //printf("too many -o inputs\n");
                exit(-1);
            }
          info("Output file: %s\n", optarg);
	       output_file = optarg;
           fileSet = 1;
          break;
        }
        case '?': {
          if (optopt != 'h'){
            fprintf(stderr, KRED "-%c is not a supported argument\n" KNRM,
                    optopt);
            USAGE(argv[0]);
            exit(-1);
          }
          USAGE(argv[0]);
          exit(0);
          break;
        }
        default: {
          break;
        }
      }
    } else if(argv[optind] != NULL) {
	info("URL to snarf: %s\n", argv[optind]);
	url_to_snarf = argv[optind];
	optind++;

    //maybe???????????????????????????????????????????????????????
    if (optind < (argc-1)){
        USAGE(argv[0]);
        //printf("wrong format\n");
        exit(-1);
    }
    }
  }
}

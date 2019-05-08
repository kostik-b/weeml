// Copyright QUB 2019

#ifndef optoptopget_h
#define optoptopget_h

#include <unistd.h>
#include <stdbool.h>

extern char*  optarg;
extern int    optind;
extern int    opterr;

static void print_usage ()
{
  printf (" -p -> peer's ip\n"
          " -r -> peer's port\n"
          " -o -> own ip\n"
          " -t -> own port\n"
          " -c -> use callback (only supported in peer_test)\n");
}

static bool get_options (
  int          argc,
  char**       argv,
  const char** peer_ip,
  int*         peer_port,
  int*         own_port,
  const char** own_ip)
{
  // opterr = 0; // don't print the error param to the stdout
  optind = 1; // reset getopt
 
  int c = '\0';
  while ((c = getopt(argc, argv, "p:r:o:t:hc")) != -1)
    switch (c) {
      case 'p':
        *peer_ip = optarg;
        break;
      case 'r':
        *peer_port = atoi (optarg);
        break;
      case 'o':
        *own_ip = optarg;
        break;
      case 't':
        *own_port = atoi (optarg);
        break;
      case 'h':
        print_usage ();
        exit (0);
        break;
      case '?':
        print_usage ();
        exit (0);
        break;
      default:
        break;
    }

  return true;
}

static bool get_use_cb (
  int          argc,
  char**       argv,
  bool*        use_cb)
{
  opterr = 0; // don't print the error param to the stdout
  optind = 1; // reset getopt
 
  int c = '\0';
  while ((c = getopt(argc, argv, "c")) != -1)
    switch (c) {
      case 'c':
        *use_cb = true;
        break;
      default:
        break;
    }

  return true;
}


#endif

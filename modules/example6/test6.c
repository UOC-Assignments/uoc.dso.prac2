#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NAME "/dev/ex6"

#define error(miss) { fprintf(stderr, "ERROR (line %d): %s\n", __LINE__, miss); exit(1); }

#define TESTING(msg) fprintf(stderr, "\n\n%s...", msg);
#define TESTINGOK() fprintf(stderr, "OK\n");

int
main ()
{
  int fd, r, n_read = 0;
  char buf[80];

  TESTING ("Checking open");
  fd = open (NAME, O_WRONLY);
  if (fd > 0)
    error ("It shouldnt be opened O_WRONLY");
  if (errno != EACCES)
    error ("Error code must be EACCES");

  fd = open (NAME, O_RDWR);
  if (fd > 0)
    error ("It shouldnt be opened O_RDWR");
  if (errno != EACCES)
    error ("Error code must be EACCES");

  fd = open (NAME, O_RDONLY);
  if (fd == -1)
    error ("It should be able to open device");
  TESTINGOK ();


  TESTING ("Checking read");
  if (read (fd, (char *) 0xd0000000, 3) >= 0)
    error
      ("It shouldnt be able to read into a buffer placed at (char*)0xd0000000");
  if (errno != EFAULT)
    error ("Error code must be EFAULT");

  while ((r = read (fd, buf, sizeof (buf))) > 0)
    {
      if (r > 3)
        error ("It shouldnt be able to read more than 3 bytes");
      write (1, buf, r);
      n_read += r;
    }

  if (n_read != 26)
    error ("Inconsistency: total number read characters");
  TESTINGOK ();

  TESTING ("Checking lseek");
  r = lseek (fd, 0, SEEK_SET);
  if (r != 0)
    error ("It must be pointing 0");

  r = lseek (fd, 0, SEEK_END);
  if (r != 26)
    error ("It must be pointing 26");

  r = lseek (fd, -10, SEEK_CUR);
  if (r != 16)
    error ("It must be pointing 16");
  TESTINGOK ();

  fprintf (stderr, "All correct\n");

  return (0);
}

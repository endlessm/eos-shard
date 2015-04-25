
#ifndef EPAK_UTILS_H
#define EPAK_UTILS_H

#define ALIGN(n) (((n) + 0x3f) & ~0x3f)

static off_t lalign(int fd)
{
  off_t off = lseek (fd, 0, SEEK_CUR);
  off = ALIGN (off);
  lseek (fd, off, SEEK_SET);
  return off;
}

#endif /* EPAK_UTILS_H */

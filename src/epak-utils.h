/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of epak.
 *
 * epak is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * epak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with epak.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

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

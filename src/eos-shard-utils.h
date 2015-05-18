/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of eos-shard.
 *
 * eos-shard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * eos-shard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with eos-shard.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef EOS_SHARD_UTILS_H
#define EOS_SHARD_UTILS_H

#define ALIGN(n) (((n) + 0x3f) & ~0x3f)

static off_t lalign(int fd)
{
  off_t off = lseek (fd, 0, SEEK_CUR);
  off = ALIGN (off);
  lseek (fd, off, SEEK_SET);
  return off;
}

#endif /* EOS_SHARD_UTILS_H */

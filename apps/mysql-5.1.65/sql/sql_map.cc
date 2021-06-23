/* Copyright (C) 2000-2001, 2004-2005 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation				// gcc: Class implementation
#endif

#include "mysql_priv.h"
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

mapped_files::mapped_files(const char * filename,uchar *magic,uint magic_length)
{
#ifdef HAVE_MMAP
  name=my_strdup(filename,MYF(0));
  use_count=1;
  error=0;
  map=0;
  size=0;
  if ((file=my_open(name,O_RDONLY,MYF(MY_WME))) >= 0)
  {
    struct stat stat_buf;
    if (!fstat(file,&stat_buf))
    {
      if (!(map=(uchar*) my_mmap(0,(size_t)(size= stat_buf.st_size),PROT_READ,
			     MAP_SHARED | MAP_NORESERVE,file,
			     0L)))
      {
	error=errno;
	my_error(ER_NO_FILE_MAPPING, MYF(0), (char *) name, error);
      }
    }
    if (map && memcmp(map,magic,magic_length))
    {
      my_error(ER_WRONG_MAGIC, MYF(0), name);
      VOID(my_munmap((char*) map,(size_t)size));
      map=0;
    }
    if (!map)
    {
      VOID(my_close(file,MYF(0)));
      file= -1;
    }
  }
#endif
}


mapped_files::~mapped_files()
{
#ifdef HAVE_MMAP
  if (file >= 0)
  {
    VOID(my_munmap((char*) map,(size_t)size));
    VOID(my_close(file,MYF(0)));
    file= -1; map=0;
  }
  my_free(name,MYF(0));
#endif
}


static I_List<mapped_files> maps_in_use;

/*
**  Check if a file is mapped. If it is, then return pointer to old map,
**  else alloc new object
*/

mapped_files *map_file(const char * name,uchar *magic,uint magic_length)
{
#ifdef HAVE_MMAP
  VOID(pthread_mutex_lock(&LOCK_mapped_file));
  I_List_iterator<mapped_files> list(maps_in_use);
  mapped_files *map;
  char path[FN_REFLEN];
  sprintf(path,"%s/%s/%s.uniq",mysql_data_home,current_thd->db,name);
  (void) unpack_filename(path,path);

  while ((map=list++))
  {
    if (!strcmp(path,map->name))
      break;
  }
  if (!map)
  {
    map=new mapped_files(path,magic,magic_length);
    maps_in_use.append(map);
  }
  else
  {
    map->use_count++;
    if (!map->map)
      my_error(ER_NO_FILE_MAPPING, MYF(0), path, map->error);
  }
  VOID(pthread_mutex_unlock(&LOCK_mapped_file));
  return map;
#else
  return NULL;
#endif
}

/*
** free the map if there are no more users for it
*/

void unmap_file(mapped_files *map)
{
#ifdef HAVE_MMAP
  VOID(pthread_mutex_lock(&LOCK_mapped_file));
  if (!map->use_count--)
    delete map;
  VOID(pthread_mutex_unlock(&LOCK_mapped_file));
#endif
}

/*****************************************************************************
** Instansiate templates
*****************************************************************************/

#ifdef HAVE_EXPLICIT_TEMPLATE_INSTANTIATION
/* Used templates */
template class I_List<mapped_files>;
template class I_List_iterator<mapped_files>;
#endif

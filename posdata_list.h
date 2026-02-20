//
// JK_Botti - be more human!
//
// posdata_list.h
//

#ifndef POSDATA_LIST_H
#define POSDATA_LIST_H

#include <string.h>

#ifndef JKASSERT
#define JKASSERT(x)
#endif

typedef struct posdata_s
{
   qboolean inuse;

   float time;
   qboolean was_alive;
   qboolean ducking;
   Vector origin;
   Vector velocity;
   //Vector accel;
   posdata_s * older;
   posdata_s * newer;

} posdata_t;

// try store max 4sec of position data, server max fps is 100 -> 10ms/frame, 4000ms/10ms = 400
#define POSDATA_SIZE (4000/10)


static inline void posdata_free_list(posdata_t *pool, int pool_size, posdata_t **p_latest, posdata_t **p_oldest)
{
   memset(pool, 0, pool_size * sizeof(pool[0]));

   *p_oldest = 0;
   *p_latest = 0;
}


static inline posdata_t *posdata_get_slot(posdata_t *pool, int pool_size, float current_time,
                                          posdata_t **p_latest, posdata_t **p_oldest)
{
   int i, oldest_idx = -1;
   float oldest_time = current_time;

   for(i = 0; i < pool_size; i++)
   {
      if(!pool[i].inuse)
         break;

      /* ugly fix */
      /* we cannot have future time, there has been some error. */
      if (pool[i].time > current_time)
          pool[i].time = current_time;

      if (pool[i].time <= oldest_time)
      {
         oldest_time = pool[i].time;
         oldest_idx = i;
      }
   }

   if(i >= pool_size)
   {
      if(oldest_idx == -1)
         return(NULL);

      i = oldest_idx;

      // Unlink the stolen node from the doubly-linked list before reuse
      posdata_t *stolen = &pool[i];
      if (stolen->older)
         stolen->older->newer = stolen->newer;
      else if (*p_oldest == stolen)
         *p_oldest = stolen->newer;
      if (stolen->newer)
         stolen->newer->older = stolen->older;
      else if (*p_latest == stolen)
         *p_latest = stolen->older;
   }

   memset(&pool[i], 0, sizeof(pool[i]));
   pool[i].time = current_time;
   pool[i].inuse = TRUE;

   return(&pool[i]);
}


static inline void posdata_link_as_latest(posdata_t **p_latest, posdata_t **p_oldest, posdata_t *node)
{
   posdata_t *curr_latest = *p_latest;
   *p_latest = node;

   if(curr_latest != NULL)
   {
      curr_latest->newer = *p_latest;
   }

   (*p_latest)->older = curr_latest;
   (*p_latest)->newer = NULL;

   if(!*p_oldest)
      *p_oldest = *p_latest;
}


// remove data older than cutoff_time
static inline void posdata_timetrim(posdata_t **p_latest, posdata_t **p_oldest, float cutoff_time)
{
   posdata_t * list;

   if(!(list = *p_oldest))
      return;

   while(list)
   {
      if(list->time <= cutoff_time)
      {
         posdata_t * next = list->newer;

         //mark slot free
         list->inuse = FALSE;

         list = next;
         *p_oldest = list;
         if (!list) break;
         list->older = 0;
      }
      else
      {
         // new enough.. so are all the rest
         break;
      }
   }

   if(!*p_oldest)
   {
      JKASSERT(*p_latest != 0);

      *p_oldest = 0;
      *p_latest = 0;
   }
}


#endif

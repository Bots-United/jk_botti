#include <stdio.h>

/* here's what the graph looks like (numbers in parenthesis are distance)...

        (50)      (40)
     0 ------> 1 ------> 2 ---\
               |        ^ |    \
               |        | |     \(50)
               |        | |(45)  \
                \   (45)| |       \
             (30)\      | |        |
                  \     | V        V
                   \-->  3 ------> 4 ------> 5
                            (55)      (35)

Note that the only 2-way path is from 2 to 3 (and 3 to 2).
All other paths are one-way paths.

Here's the paths and distances:

 0 -----> 1  (distance = 50 units)
 1 -----> 2  (distance = 40 units)
 1 -----> 3  (distance = 30 units)
 2 -----> 3  (distance = 45 units)
 2 -----> 4  (distance = 50 units)
 3 -----> 2  (distance = 45 units)
 3 -----> 4  (distance = 55 units)
 4 -----> 5  (distance = 35 units)

All paths which aren't possible are indicated by -1 in the shortest_path table

The shortest_path array is a 2 dimensional array of all waypoints (i.e. if
there are 50 waypoints then the table will be 50 X 50 in size.  The rows
(running down the table) indicate the starting index (0 to N) and the columns
(running across the table) indicate the ending index.  So if you wanted to 
know the distance between waypoint 37 and waypoint 42 you would look at
shortest_path[37][42].  If you wanted to know the distance between waypoints
42 and 37, you would look at shortest_path[42][37].  NOTE!, these 2 DO NOT
HAVE TO BE THE SAME!!!  One way paths may mean that shortest_path[37][42]
is 150 units and shortest_path[42][37] is -1 (indicating you can't go that
way!!!)
                             
*/

#define MAX 6

int shortest_path[MAX][MAX] = {
   { 0, 50, -1, -1, -1, -1},
   {-1,  0, 40, 30, -1, -1},
   {-1, -1,  0, 45, 50, -1},
   {-1, -1, 45,  0, 55, -1},
   {-1, -1, -1, -1,  0, 35},
   {-1, -1, -1, -1, -1,  0}
};


int from_to[MAX][MAX];

void floyd(void)
{
   int x, y, z;
   int changed = 1;
   int distance;

   for (y=0; y < MAX; y++)
   {
      for (z=0; z < MAX; z++)
      {
         from_to[y][z] = z;
      }
   }

   while (changed)
   {
      changed = 0;

      for (x=0; x < MAX; x++)
      {
         for (y=0; y < MAX; y++)
         {
            for (z=0; z < MAX; z++)
            {
               if ((shortest_path[y][x] == -1) || (shortest_path[x][z] == -1))
                  continue;

               distance = shortest_path[y][x] + shortest_path[x][z];

               if ((distance < shortest_path[y][z]) ||
                   (shortest_path[y][z] == -1))
               {
                  shortest_path[y][z] = distance;
                  from_to[y][z] = from_to[y][x];
                  changed = 1;
               }
            }
         }
      }
   }
}

void main(void)
{
   int a, b;
   char buffer[20];

   // run Floyd-Warshall algorithm on the shortest_path table...
   floyd();

   // initialize any unreachable paths based on shortest_path table...
   for (a=0; a < MAX; a++)
   {
      for (b=0; b < MAX; b++)
      {
         if (shortest_path[a][b] == -1)
            from_to[a][b] = -1;
      }
   }

   printf("shortest_path:\n");
   for (a=0; a < MAX; a++)
   {
      for (b=0; b < MAX; b++)
        printf("%5d,", shortest_path[a][b]);
      printf("\n");
   }
   printf("\n");

   printf("from_to:\n");
   for (a=0; a < MAX; a++)
   {
      for (b=0; b < MAX; b++)
        printf("%2d,", from_to[a][b]);
      printf("\n");
   }
   printf("\n");

   printf("enter start node (0-5):");
   gets(buffer);
   a = atoi(buffer);

   printf("\nenter end node (0-5):");
   gets(buffer);
   b = atoi(buffer);

   if (shortest_path[a][b] != -1)
   {
      printf("\n\nShortest path from %d to %d is:\n", a, b);

      while (a != b)
      {
         printf("from %d goto %d (total distance remaining=%d)\n",
            a, from_to[a][b], shortest_path[a][b]);
         a = from_to[a][b];
      }
   }
   else
      printf("You can't get from %d to %d!\n", a, b);
}

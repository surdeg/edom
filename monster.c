/*                               -*- Mode: C -*- 
 * monster.c -- 
 * ITIID           : $ITI$ $Header $__Header$
 * Author          : Thomas Biskup
 * Created On      : Mon Dec 30 18:11:48 1996
 * Last Modified By: Thomas Biskup
 * Last Modified On: Thu Jan  9 20:16:01 1997
 * Update Count    : 56
 * Status          : Unknown, Use with caution!
 *
 * (C) Copyright 1996, 1997 by Thomas Biskup.
 * All Rights Reserved.
 *
 * This software may be distributed only for educational, research and
 * other non-proft purposes provided that this copyright notice is left
 * intact.  If you derive a new game from these sources you also are
 * required to give credit to Thomas Biskup for creating them in the first
 * place.  These sources must not be distributed for any fees in excess of
 * $3 (as of January, 1997).
 */

/*
 * Design notes: QHack assumes a maximum that the initial level will have
 * 4 different monster types.  Per level descended another two new monsters
 * become available.
 */


/*
 * Includes.
 */

#include "main.h"


/*
 * Local constants.
 */

/* The total number of monsters. */
#define MAX_MONSTER 4

/*
 * Global variables.
 */

struct monster_struct m;


/*
 * Local variables.
 */


/* The complete monster list for the game. */
struct monster_def md[MAX_MONSTER] =
{
  {"hydra.png", 24, 28, "hydra", 14, "1d4", 1, 0, "1d6", COMMON},
  {"gargoyle.png", 24, 32, "gargoyle", 12, "1d3", 1, 0, "1d3", COMMON},
  {"reaper.png", 32, 32, "reaper", 13, "1d8", 1, 0, "1d6", COMMON},
  {"samurai.png", 31, 32, "samurai", 18, "2d3", 1, +1, "1d4", RARE}
};

/* The dynamic index map for one monster level. */
static byte midx[MAP_W][MAP_H];

/* The total rarity for monsters; dependent on the current level. */
static uint32 total_rarity;



/*
 * Local prototypes.
 */

void get_monster_coordinates(coord *, coord *);
byte get_monster_index(void);
byte monster_level(byte);
int16 monster_rarity(byte);
void create_monster_in(byte midx);
int16 mhits(byte);



/*
 * Initialize the monster structures.  Basically we have to notify
 * all slots that they are empty.  Also the general index map needs
 * to be initialized.
 */

void init_monsters(void)
{
  coord i, j;

  for (i = 0; i < MAX_DUNGEON_LEVEL; i++)
  {
    /* The first empty monster slot. */
    m.eidx[i] = 0;

    /* Initially all slots are empty. */
    for (j = 0; j < MONSTERS_PER_LEVEL - 1; j++)
    {
      m.m[i][j].used = FALSE;
      m.m[i][j].midx = j + 1;
    }

    /* The last one points to 'no more slots'. */
    m.m[i][MONSTERS_PER_LEVEL - 1].midx = -1;
    m.m[i][MONSTERS_PER_LEVEL - 1].used = FALSE;
  }

  /* Initialize the monster index map as 'empty'. */
  for (i = 0; i < MAP_W; i++)
    for (j = 0; j < MAP_H; j++)
      midx[i][j] = -1;
}



/*
 * Create the monster map for a given dungeon level.
 */

void build_monster_map(void)
{
  coord x, y;

  /* Initialize the monster index map as 'empty'. */
  for (x = 0; x < MAP_W; x++)
    for (y = 0; y < MAP_H; y++)
      midx[x][y] = -1;

  /* Setup all monster indices. */
  for (x = 0; x < MONSTERS_PER_LEVEL; x++)
    if (m.m[d.dl][x].used)
      midx[m.m[d.dl][x].x][m.m[d.dl][x].y] = x;
}



/*
 * Create an initial monster population for a given level.
 */

void create_population(void)
{
  byte m;

  /* Initialize the basic monster data. */
  initialize_monsters();

  for (m = 0; m < INITIAL_MONSTER_NUMBER; m++)
  {
    byte midx;

    /* Find a monster index. */
    midx = get_monster_index();

    /* Paranoia. */
    if (midx == -1)
      die("Could not create the initial monster population");

    /* Create a new monster. */
    create_monster_in(midx);
  }
}



/*
 * Return the maximum monster number for the current dungeon level.
 *
 * XXXX: Since the current monster list is somewhat limited only four
 *       monsters are available.
 */

byte max_monster(void)
{
  return imin(MAX_MONSTER, ((d.dl << 1) + 4));
}



/*
 * Determine the frequency for a given monster.
 *
 * This value is level-dependent.  If the monster is out-of-depth (for
 * QHack this means 'has a lower minimum level than the current dungeon
 * level) it's frequency will be reduced.
 */

static int lmod[14] =
{
  100, 90, 80, 72, 64, 56, 50, 42, 35, 28, 20, 12, 4, 1
};

int16 monster_rarity(byte midx)
{
  int16 rarity = md[midx].rarity;
  byte level_diff = d.dl - monster_level(midx);

  return imax(1, (rarity * lmod[imin(13, level_diff)]) / 100);
}


/*
 * Determine the minimum level for a given monster number.
 */

byte monster_level(byte midx)
{
  if (midx < 4)
    return 0;

  return (midx - 2) >> 1;
}


/*
 * Calculate the frequencies for all available monsters based upon the current
 * dungeon level.
 */

void initialize_monsters(void)
{
  byte i;
  
  total_rarity = 0;

  for (i = 0; i < max_monster(); i++)
    total_rarity += monster_rarity(i);
}



/*
 * Determine the index number for a random monster on the current dungeon
 * level.
 */

byte random_monster_type(void)
{
  int32 roll;
  byte i;

  roll = rand_long(total_rarity) + 1;
  i = 0;

  while (roll > monster_rarity(i))
  {
    roll -= monster_rarity(i);
    i++;
  }
  
  return i;
}



/*
 * Create a new monster in a given slot.
 */

void create_monster_in(byte midx)
{
  byte type;

  type = random_monster_type();

  /* Initialize actor based on monster type */
  init_actor(&m.m[d.dl][midx].a,
             md[type].filename, md[type].w, md[type].h,
             &common_anim);

  /* Adjust the 'empty' index. */
  if (m.eidx[d.dl] == midx)
    m.eidx[d.dl] = m.m[d.dl][midx].midx; 

  /* Create the actual monster. */
  m.m[d.dl][midx].used = TRUE;
  m.m[d.dl][midx].midx = type;
  get_monster_coordinates(&m.m[d.dl][midx].x, &m.m[d.dl][midx].y);
  m.m[d.dl][midx].hp = m.m[d.dl][midx].max_hp = mhits(m.m[d.dl][midx].midx);
  m.m[d.dl][midx].state = ASLEEP;

  /* Fill in in actor field */
  m.m[d.dl][midx].a.x = m.m[d.dl][midx].x * TILE_WIDTH;
  m.m[d.dl][midx].a.y = m.m[d.dl][midx].y * TILE_HEIGHT;
}



/*
 * Find coordinates for a new monster.  Some things need to be considered:
 *  1. Monsters should only be created on 'floor' tiles.
 *  2. New monsters should not be in LOS of the PC.
 *  3. New monsters should not be created in spots where another monster is
 *     standing.
 */

void get_monster_coordinates(coord *x, coord *y)
{
  do
  {
    *x = rand_int(MAP_W);
    *y = rand_int(MAP_H);
  }
  while (tile_at(*x, *y) != FLOOR ||
	 los(*x, *y) ||
	 midx[*x][*y] != -1);
}



/*
 * Return an initial hitpoint number for a monster of a given type.
 */

int16 mhits(byte midx)
{
  return dice(md[midx].hits);
}



/*
 * Return the first potentially empty monster slot.
 */

byte get_monster_index(void)
{
  return m.eidx[d.dl];
}



/*
 * Check whether a PC is able to see a position.
 */

BOOL los(coord x, coord y)
{
  coord sx, sy, psx, psy;

  /* Adjacent to the PC? */
  if (abs(x - d.px) <= 1 && abs(y - d.py) <= 1)
    return TRUE;

  /* Get the section for the given position. */
  get_current_section(x, y, &sx, &sy);

  /* Get the section for the player. */
  get_current_section(d.px, d.py, &psx, &psy);

  /* In the same room section? */
  return (sx == psx && sy == psy && sx != -1);
}



/*
 * Get a monster at a specific position.
 */

struct monster *get_monster_at(coord x, coord y)
{
  /* Paranoia. */
  if (midx[x][y] == -1)
    die("No monster to retrieve");

  /* Return the requested monster. */
  return &m.m[d.dl][midx[x][y]];
}



void attack_monster_at(coord x, coord y)
{
  struct monster *mi = get_monster_at(x, y);

  /* TODO */
  int damage = 1;

  mi->hp -= damage;
  if (mi->hp <= 0)
  {
    you("%s perrished.", md[mi->midx].name);
    remove_monster_at(x, y);
  }
  else
  {
    set_counter_actor(&mi->a, &d.pa);
    you("Attacked %s and caused %d damage.", md[mi->midx].name, damage);
  }
}



void remove_monster_at(coord x, coord y)
{
  m.m[d.dl][midx[x][y]].midx = -1;
  m.m[d.dl][midx[x][y]].used = FALSE;
  midx[x][y] = -1;
}



/*
 * Determine whether a monster holds a given position.
 */

BOOL is_monster_at(coord x, coord y)
{
  return (midx[x][y] != -1);
}



static BOOL is_clear(struct monster *m, enum facing dir)
{
  BOOL result = FALSE;

  switch(dir)
  {
    case DOWN:
      if (is_floor(m->x, m->y + 1) &&
          !is_monster_at(m->x, m->y + 1) &&
          !(m->x == d.px && m->y + 1 == d.py))
        result = TRUE;
      break;

    case LEFT:
      if (is_floor(m->x - 1, m->y) &&
          !is_monster_at(m->x - 1, m->y) &&
          !(m->x - 1 == d.px && m->y == d.py))
        result = TRUE;
      break;

    case RIGHT:
        if (is_floor(m->x + 1, m->y) &&
            !is_monster_at(m->x + 1, m->y) &&
            !(m->x + 1 == d.px && m->y == d.py))
        result = TRUE;
      break;

    case UP:
      if (is_floor(m->x, m->y - 1) &&
          !is_monster_at(m->x, m->y - 1) &&
          !(m->x == d.px && m->y - 1 == d.py))
        result = TRUE;
      break;
  }

  return result;
}

void move_monster(struct monster *m, enum facing dir)
{
  byte i;

  if (m->a.act == IDLE)
  {
    set_dir_actor(&m->a, dir);
    move_actor(&m->a, dir);

    /* Store mondest index in slot at current position */
    i = midx[m->x][m->y];

    /* Clear slot */
    midx[m->x][m->y] = -1;

    /* Update monster position */
    m->x += m->a.dx;
    m->y += m->a.dy;

    /* Set index in slot at new position */
    midx[m->x][m->y] = i;
  }
}


/*
 * Handle the monster turn: movement, combat, etc.
 */

void move_monsters(void)
{
  coord x, y;
  int sx = d.map_x / TILE_WIDTH;
  int sy = d.map_y / TILE_HEIGHT;

  for (y = 0; y < screen_height / TILE_HEIGHT; y++)
    for (x = 0; x < screen_width / TILE_WIDTH; x++)
      if (is_monster_at(sx + x, sy + y) && los(sx + x, sy + y))
      {
        struct monster *mi = get_monster_at(sx + x, sy + y);

        if (mi->a.act == COUNTER)
        {
          move_counter_actor(&mi->a);
        }
        else if (mi->a.act == ATTACK)
        {
          message("%s attacked you.", md[mi->midx].name);
          mi->a.act = IDLE;
        }
        else if (mi->a.act == MOVE)
        {
          animate_move_actor(&mi->a);
        }
        else if (mi->a.act == IDLE)
        {
          if (mi->state == ASLEEP)
          {
            if (abs(mi->x - d.px) <= 2 && abs(mi->y - d.py) <= 2)
              mi->state = NEUTRAL;
          }
          else if (mi->state == NEUTRAL)
          {
            if (abs(mi->x - d.px) <= 1 && abs(mi->y - d.py) <= 1)
            {
              mi->state = ANGRY;
            }
            else
            {
              enum facing dir = rand_long(4);
              if (is_clear(mi, dir))
                move_monster(mi, dir);
            }
          }
          else if (mi->state == ANGRY)
          {
            if (d.px < mi->x && is_clear(mi, LEFT))
              move_monster(mi, LEFT);
            else if (d.px > mi->x && is_clear(mi, RIGHT))
              move_monster(mi, RIGHT);
            else if (d.py < mi->y && is_clear(mi, UP))
              move_monster(mi, UP);
            else if (d.py > mi->y && is_clear(mi, DOWN))
              move_monster(mi, DOWN);
            else
              face_target_actor(&mi->a, &d.pa);
          }
        }
      }
}



void draw_monsters(void)
{
  coord x, y;
  int sx = d.map_x / TILE_WIDTH;
  int sy = d.map_y / TILE_HEIGHT;

  for (y = 0; y < screen_height / TILE_HEIGHT; y++)
    for (x = 0; x < screen_width / TILE_WIDTH; x++)
      if (is_monster_at(sx + x, sy + y) && los(sx + x, sy + y))
      {
        struct monster *m = get_monster_at(sx + x, sy + y);

        draw_actor(&m->a);
      }
}

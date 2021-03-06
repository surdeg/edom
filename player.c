/*                               -*- Mode: C -*- 
 * player.c -- 
 * ITIID           : $ITI$ $Header $__Header$
 * Author          : Thomas Biskup
 * Created On      : Mon Jan  6 11:42:38 1997
 * Last Modified By: Thomas Biskup
 * Last Modified On: Thu Jan  9 21:22:26 1997
 * Update Count    : 60
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
 * Includes.
 */

#include <stdlib.h>
#include <string.h>
#include "main.h"


/*
 * Global variables.
 */

/* Update the player status line? */
BOOL update_necessary = TRUE;

/* String constants for the training skills. */
static char *tskill_s[MAX_T_SKILL] =
{
  "Strength", "Intelligence", "Dexterity", "Toughness", "Mana",
  "Hitpoints", "Magical Power", "To-Hit Bonus", "To-Damage Bonus",
  "Searching"
};


static struct anim_info player_anim =
{
  22, 22,
  5, 10, 15, 0,
  3,
  2,
  4,
  8, 13, 18, 3,
  2
};

/*
 * Local prototypes.
 */

long required_exp(byte);
int current_level(byte);
void increase_training_skill(byte);


/*
 * Set up all the data for the player.
 */

void init_player(void)
{
  byte i;

  init_actor(&d.pa, "viking.png", 76, 76, &player_anim);

  /* Initial attributes. */
  for (i = 0; i < MAX_ATTRIBUTE; i++)
    set_attribute(i, dice("6d3"));

  /* Initial hitpoints. */
  d.pc.hits = d.pc.max_hits = (get_attribute(TOUGHNESS) +
			       (get_attribute(STRENGTH) >> 1) +
			       dice("1d6"));

  /* Initial magical power. */
  d.pc.power = d.pc.max_power = (get_attribute(MANA) +
				 (get_attribute(INTELLIGENCE) >> 2) +
				 dice("1d6"));

  /* Initial experience. */
  d.pc.experience = 0;
  for (i = 0; i < MAX_T_SKILL; i++)
    d.pc.tskill_exp[i] = 0;

  /* The number of training units initially used. */
  for (i = T_MANA + 1; i < MAX_T_SKILL; i++)
    d.pc.tskill_training[i] = TUNITS / (MAX_T_SKILL - T_MANA - 1);

  /* Searching skill. */
  d.pc.searching = get_attribute(INTELLIGENCE) +
    (get_attribute(MANA) / 5);

  /* Combat bonusses. */
  d.pc.to_hit = d.pc.to_damage = 0;
  
  /* Default name. */
  strcpy(d.pc.name, "Brak");
}



/*
 * Set a PC attribute.
 */

void set_attribute(byte attribute, byte value)
{
  d.pc.attribute[attribute] = d.pc.max_attribute[attribute] = value;
}



/*
 * Get the effective value of an attribute.
 */

byte get_attribute(byte attribute)
{
  return d.pc.attribute[attribute];
}



/*
 * Draw the status line.
 */

void draw_player_status(void)
{
  static char str[80];

  if (update_necessary)
  {
    sprintf(str,
           "%s   St:%d  In:%d  Dx:%d  To:%d  Ma:%d  H:%d(%d)  P:%d(%d)  X:%ld"
            , d.pc.name
            , (int) d.pc.attribute[STRENGTH]
            , (int) d.pc.attribute[INTELLIGENCE]
            , (int) d.pc.attribute[DEXTERITY]
            , (int) d.pc.attribute[TOUGHNESS]
            , (int) d.pc.attribute[MANA]
            , (int) d.pc.hits
            , (int) d.pc.max_hits
            , (int) d.pc.power
            , (int) d.pc.max_power
            , (long) d.pc.experience);

    draw_fixed_text(0, screen_height + MSG_H, screen_width - 2 * FNT_W, str,
                    font);

    update_necessary = FALSE;
  }
}



/*
 * This function provides the main menu for adjusting the available
 * training levels.  Everything important happens here.
 */

void adjust_training(void)
{
#if 0
  char c;
  byte i, length, exp_length, unit_length, training_length;
  int16 remaining_units;
  BOOL do_redraw;
  static byte pos = 0;  /* Initial menu position. */
  
  /*
   * Determine the maximum training skill length.  This could be a hard-coded
   * constants but by doing this dynamically it's a lot simpler and less
   * error-prone to change the specific training skills.
   *
   * In the same run we count the number of training units spent.
   */
  length = exp_length = unit_length = 0;
  remaining_units = TUNITS;
  for (i = 0; i < MAX_T_SKILL; i++)
  {
    length = imax(length, strlen(tskill_s[i]));
    exp_length = imax(exp_length
		      , strlen(string("%d"
				      , (int) d.pc.tskill_exp[i] / TUNITS)));
    unit_length = imax(unit_length
		       , strlen(string("%ld"
				       , (long) required_exp(i) / TUNITS)));
    remaining_units -= d.pc.tskill_training[i];
  }

  /* Main loop.  Draw the menu and react on commands. */
  do_redraw = TRUE;

  do
  {
    /* Draw the menu. */
    if (do_redraw)
    {
      training_length = 0;
      for (i = 0; i < MAX_T_SKILL; i++)
	training_length = imax(training_length
			       , strlen(string("%d"
					       , (int)
					       d.pc.tskill_training[i])));
      for (i = 0; i < MAX_T_SKILL; i++)
      {
	cursor(3, i);
	prtstr("    %*s: %*ld of %*ld [%*d]: %d   "
	       , length
	       , tskill_s[i]
	       , exp_length
	       , (long) d.pc.tskill_exp[i] / TUNITS
	       , unit_length
	       , (long) required_exp(i) / TUNITS
	       , training_length
	       , (int) d.pc.tskill_training[i]
	       , (int) current_level(i));
      }
      cursor(0, 24);
      prtstr(" [iI] Up -- [kK] Down -- [jJ] Decrease -- [lL] Increase");
      prtstr(" -- Units: %d", (int) remaining_units);
      clear_to_eol();
      do_redraw = FALSE;
    }
      
    cursor(4, pos);
    prtstr("->");
    update();
    c = getkey();
    cursor(4, pos);
    prtstr("  ");
    update();
      
    switch (c)
    {
      case 'L':
	if (remaining_units)
	{
	  d.pc.tskill_training[pos] += remaining_units;
	  remaining_units = 0;
	  do_redraw = TRUE;
	}
	break;
	
      case 'l':
	if (remaining_units)
	{
	  remaining_units--;
	  d.pc.tskill_training[pos]++;
	  do_redraw = TRUE;
	}
	break;
	
      case 'J':
	if (d.pc.tskill_training[pos])
	{
	  remaining_units += d.pc.tskill_training[pos];
	  d.pc.tskill_training[pos] = 0;
	  do_redraw = TRUE;
	}
	break;
	
      case 'j':
	if (d.pc.tskill_training[pos])
	{
	  d.pc.tskill_training[pos]--;
	  remaining_units++;
	  do_redraw = TRUE;
	}
	break;

      case 'I':
	pos = 0;
	break;
	
      case 'i':
	if (pos)
	  pos--;
	else
	  pos = MAX_T_SKILL - 1;
	break;

      case 'K':
	pos = MAX_T_SKILL - 1;
	break;
	
      case 'k':
	if (pos < MAX_T_SKILL - 1)
	  pos++;
	else
	  pos = 0;
	break;
	
      default:
	break;
    }
  }
  while (c != 27 && c != 'Q' && c != 32);

  /* Clean up. */
  redraw();
#endif
}




/*
 * Determine the required amount of experience for a specific skill.
 */

long required_exp(byte i)
{
  switch (i)
  {
    case T_STRENGTH:
      return (d.pc.attribute[STRENGTH] + 1) * 65 * TUNITS;
      
    case T_INTELLIGENCE:
      return (d.pc.attribute[INTELLIGENCE] + 1) * 60 * TUNITS;

    case T_DEXTERITY:
      return (d.pc.attribute[DEXTERITY] + 1) * 60 * TUNITS;

    case T_TOUGHNESS:
      return (d.pc.attribute[TOUGHNESS] + 1) * 60 * TUNITS;
      
    case T_MANA:
      return (d.pc.attribute[MANA] + 1) * 55 * TUNITS;
      
    case T_HITS:
      return (d.pc.max_hits + 1) * TUNITS;
      
    case T_POWER:
      return (d.pc.max_power + 1) * TUNITS;
      
    case T_2HIT:
      return (((d.pc.to_hit + 1) * (d.pc.to_hit + 2)) >> 1) * 5 * TUNITS;
      
    case T_2DAMAGE:
      return (((d.pc.to_damage + 1) * (d.pc.to_damage + 2)) >> 1)
	* 25 * TUNITS;
      
    case T_SEARCHING:
      return (d.pc.searching + 1) * TUNITS;
  }

  return 0;
}



/*
 * Determine the current value of a training skill.
 */

int current_level(byte i)
{
  switch (i)
  {
    case T_STRENGTH:
    case T_INTELLIGENCE:
    case T_DEXTERITY:
    case T_TOUGHNESS:
    case T_MANA:
      return d.pc.attribute[i];
      
    case T_HITS:
      return d.pc.max_hits;
      
    case T_POWER:
      return d.pc.max_power;
      
    case T_2HIT:
      return d.pc.to_hit;
      
    case T_2DAMAGE:
      return d.pc.to_damage;
      
    case T_SEARCHING:
      return d.pc.searching;
  }

  return 0;
}



/*
 * Increase a specified training skill by +1.
 */

void increase_training_skill(byte i)
{
  switch (i)
  {
    case T_STRENGTH:
    case T_INTELLIGENCE:
    case T_DEXTERITY:
    case T_TOUGHNESS:
    case T_MANA:
      d.pc.attribute[i]++;
      break;
      
    case T_HITS:
      d.pc.max_hits++;
      d.pc.hits++;
      break;
      
    case T_POWER:
      d.pc.max_power++;
      d.pc.power++;
      break;
      
    case T_2HIT:
      d.pc.to_hit++;
      break;
      
    case T_2DAMAGE:
      d.pc.to_damage++;
      break;
      
    case T_SEARCHING:
      d.pc.searching++;
      break;
  }
  update_necessary = TRUE;
}



/*
 * Score a specified number of experience points.
 */

void score_exp(int32 x)
{
  byte i;

  /* Overall adjustment. */
  d.pc.experience += x;

  /* Divided adjustment. */
  for (i = 0; i < MAX_T_SKILL; i++)
  {
    d.pc.tskill_exp[i] += (x * d.pc.tskill_training[i]);

    /* Check advancement. */
    while (d.pc.tskill_exp[i] >= required_exp(i))
    {
      d.pc.tskill_exp[i] -= required_exp(i);
      increase_training_skill(i);
      message("Your %s increases to %d."
	      , tskill_s[i], current_level(i));
    }
  }

  /* Update the changes. */
  update_necessary = TRUE;
}

void place_player(byte px, byte py)
{
  d.px = px;
  d.opx = px;
  d.pa.x = (int16) px * TILE_WIDTH;

  d.py = py;
  d.opy = py;
  d.pa.y = (int16) py * TILE_HEIGHT;

  d.pa.act = IDLE;
  d.pa.dx = 0;
  d.pa.dy = 0;

  set_dir_actor(&d.pa, DOWN);
}

void move_player(enum facing dir)
{
  if (d.pa.act == IDLE)
  {
    move_actor(&d.pa, dir);
    d.px += d.pa.dx;
    d.py += d.pa.dy;
  }
}


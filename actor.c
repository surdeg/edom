#include <stdlib.h>
#include <string.h>
#include "sprite.h"
#include "main.h"

void init_actor(struct actor *a, const char *fn, int w, int h, const struct anim_info *info)
{
  a->spr = load_sprite(fn, w, h);
  if (a->spr == NULL)
  {
    printf("Fatal Error -- Unable to load sprite: %s\n", fn);
    exit(1);
  }

  a->act = IDLE;
  a->dx = 0;
  a->dy = 0;

  a->base_frame = 0;
  a->delta_frame = 0;
  a->rev_anim = FALSE;
  a->counter = 0;

  memcpy(&a->anim_info, info, sizeof(struct anim_info));
}

void set_dir_actor(struct actor *a, enum facing dir)
{
  a->counter = 0;
  a->dir = dir;
  a->delta_frame = 1;

  switch (dir)
  {
    case LEFT:
      a->base_frame = a->anim_info.left;
      break;

    case RIGHT:
      a->base_frame = a->anim_info.right;
      break;

    case UP:
      a->base_frame = a->anim_info.up;
      break;

    case DOWN:
      a->base_frame = a->anim_info.down;
      break;

    default:
      a->base_frame = 0;
      break;
  }
}

void animate_walk_actor(struct actor *a)
{
  if (++a->counter == a->anim_info.treshold)
  {
    if (!a->rev_anim)
    {
      if (++a->delta_frame > a->anim_info.num_walk_frames - 1)
      {
        a->delta_frame = a->anim_info.num_walk_frames - 1;
        a->rev_anim = TRUE;
      }
    }
    else
    {
      if (--a->delta_frame < 0)
      {
        a->delta_frame = 0;
        a->rev_anim = FALSE;
      }
    }

    a->counter = 0;
  }
}

void move_actor(struct actor *a, enum facing dir)
{
  switch(dir)
  {
    case DOWN:
      a->dx = 0;
      a->dy = 1;
      break;

    case LEFT:
      a->dx = -1;
      a->dy = 0;
      break;

    case RIGHT:
      a->dx = 1;
      a->dy = 0;
      break;

    case UP:
      a->dx = 0;
      a->dy = -1;
      break;
  }

  a->act = MOVE;
}

void animate_move_actor(struct actor *a)
{
  if (a->act == MOVE)
  {
    animate_walk_actor(a);

    if (a->dx)
    {
      a->x += a->dx * a->anim_info.speed;
      if ((a->x % TILE_WIDTH) == 0)
      {
        a->dx = 0;
        a->delta_frame = 1;
        a->act = IDLE;
      }
    }

    if (a->dy)
    {
      a->y += a->dy * a->anim_info.speed;
      if ((a->y % TILE_HEIGHT) == 0)
      {
        a->dy = 0;
        a->delta_frame = 1;
        a->act = IDLE;
      }
    }
  }
}

void set_counter_actor(struct actor *a, struct actor *target)
{
  a->counter = 0;
  a->delta_frame = 0;
  a->act = COUNTER;
  face_target_actor(a, target);
}

void move_counter_actor(struct actor *a)
{
  /* Replace with configurable time */
  if (++a->counter >= 10)
  {
    a->counter = 0;
    a->act = ATTACK;
  }
}

void set_attack_actor(struct actor *a, enum facing dir)
{
  a->counter = 0;
  a->delta_frame = 0;
  a->dir = dir;
  a->act = ATTACK;

  switch (dir)
  {
    case LEFT:
      a->base_frame = a->anim_info.attack_left;
      break;

    case RIGHT:
      a->base_frame = a->anim_info.attack_right;
      break;

    case UP:
      a->base_frame = a->anim_info.attack_up;
      break;

    case DOWN:
      a->base_frame = a->anim_info.attack_down;
      break;

    default:
      a->base_frame = 0;
      break;
  }
}

void animate_attack_actor(struct actor *a)
{
  if (a->act == ATTACK)
  {
    if (++a->counter == a->anim_info.treshold)
    {
      if (++a->delta_frame > a->anim_info.num_attack_frames - 1)
      {
        set_dir_actor(a, a->dir);
        a->act = IDLE;
      }

      a->counter = 0;
    }
  }
}

void face_target_actor(struct actor *a, struct actor *target)
{
  if (target->x < a->x)
    set_dir_actor(a, LEFT);
  else if (target->x > a->x)
    set_dir_actor(a, RIGHT);
  else if (target->y < a->y)
    set_dir_actor(a, UP);
  else if (target->y > a->y)
    set_dir_actor(a, DOWN);
}

void draw_actor(struct actor *a)
{
  draw_sprite(a->x - a->anim_info.anchor_x - d.map_x,
              a->y - a->anim_info.anchor_y - d.map_y,
              a->base_frame + a->delta_frame, a->spr,
              0, 0, screen_width, screen_height);
}


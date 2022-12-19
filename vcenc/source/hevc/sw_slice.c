/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#include "vsi_string.h"
#include "sw_slice.h"
#include "tools.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
  get_slice
------------------------------------------------------------------------------*/
struct slice *sw_get_slice(struct queue *q, i32 num)
{
    struct node *n;
    struct slice *s;

    for (n = q->tail; n; n = n->next) {
        s = (struct slice *)n;
        if (s->num == num)
            return s;
    }
    return NULL;
}

/*------------------------------------------------------------------------------
  create_slice
------------------------------------------------------------------------------*/
struct slice *sw_create_slice(i32 num)
{
    struct slice *s;

    if (!(s = EWLmalloc(sizeof(struct slice))))
        return NULL;

    memset(s, 0, sizeof(struct slice));
    s->num = num;

    return s;
}

/*------------------------------------------------------------------------------
  free_slices
------------------------------------------------------------------------------*/
void sw_free_slices(struct queue *q)
{
    struct node *n;

    while ((n = queue_get(q))) {
        EWLfree(n);
    }
}

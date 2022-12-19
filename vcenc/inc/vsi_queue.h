/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef QUEUE_H_
#define QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct node {
    struct node *next;
};

struct queue {
    struct node *head;
    struct node *tail;
};

/* rename functions with prefix. */
#define queue_init VSIAPI(queue_init)
#define queue_put VSIAPI(queue_put)
#define queue_put_tail VSIAPI(queue_put_tail)
#define queue_remove VSIAPI(queue_remove)
#define free_nodes VSIAPI(free_nodes)
#define queue_get VSIAPI(queue_get)
#define queue_tail VSIAPI(queue_tail)
#define queue_head VSIAPI(queue_head)

void queue_init(struct queue *);
void queue_put(struct queue *, struct node *);
void queue_put_tail(struct queue *, struct node *);
void queue_remove(struct queue *, struct node *);
void free_nodes(struct node *tail);
struct node *queue_get(struct queue *);
struct node *queue_tail(struct queue *queue);
struct node *queue_head(struct queue *queue);

#ifdef __cplusplus
}
#endif

#endif

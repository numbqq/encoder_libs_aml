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

#include "base_type.h"
#include "vsi_queue.h"
#include "osal.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
  queue_init
------------------------------------------------------------------------------*/
void queue_init(struct queue *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

/*------------------------------------------------------------------------------
  queue_put
------------------------------------------------------------------------------*/
void queue_put(struct queue *queue, struct node *node)
{
    node->next = NULL;
    if (queue->head) {
        queue->head->next = node;
        queue->head = node;
    } else {
        queue->tail = node;
        queue->head = node;
    }
}

/*------------------------------------------------------------------------------
  queue_put_tail
------------------------------------------------------------------------------*/
void queue_put_tail(struct queue *queue, struct node *node)
{
    if (queue->tail) {
        node->next = queue->tail;
        queue->tail = node;
    } else {
        node->next = NULL;
        queue->tail = node;
        queue->head = node;
    }
}

/*------------------------------------------------------------------------------
  queue_get
------------------------------------------------------------------------------*/
struct node *queue_get(struct queue *queue)
{
    struct node *node = queue->tail;

    if (!node)
        return NULL; /* Empty queue */

    if (queue->head == queue->tail) /* The last node */
    {
        queue->head = NULL;
    }
    queue->tail = node->next;

    return node;
}

/*------------------------------------------------------------------------------
  queue_tail
------------------------------------------------------------------------------*/
struct node *queue_tail(struct queue *queue)
{
    return queue->tail;
}

/*------------------------------------------------------------------------------
  queue_head
------------------------------------------------------------------------------*/
struct node *queue_head(struct queue *queue)
{
    return queue->head;
}

/*------------------------------------------------------------------------------
  queue_remove
------------------------------------------------------------------------------*/
void queue_remove(struct queue *queue, struct node *node)
{
    struct node *prev, *p;

    /* Less than one node left is special case */
    if (queue->tail == queue->head) {
        if (queue->head == node) {
            queue_init(queue);
        }
        return;
    }

    for (prev = p = queue->tail; p; p = p->next) {
        if (p == node) {
            prev->next = p->next;
            if (queue->head == node)
                queue->head = prev;
            if (queue->tail == node)
                queue->tail = p->next;
            break;
        }
        prev = p;
    }
}

/*------------------------------------------------------------------------------
  free_nodes
------------------------------------------------------------------------------*/
void free_nodes(struct node *tail)
{
    struct node *node;

    while (tail) {
        node = tail;
        tail = tail->next;
        EWLfree(node);
    }
}

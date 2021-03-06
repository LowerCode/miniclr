#include "list.h"
#include "platform.h"
#include <stdint.h>
#include <string.h>

#define Spinlock uint8_t *
#define CreateSpinlock() (NULL)
#define LockSpinlock(x) ((void *)1)
#define UnlockSpinlock(x)
#define FinalLockSpinlock(x)
#define FreeSpinlock(x)

typedef struct ListNode {
  void *value;
  struct ListNode *next;
  struct ListNode *prev;
} ListNode;

typedef struct List {
  ListNode *nodes;
  ListNode *last;
  ListNode *last_accessed_node;
  uint64_t last_accessed_index;
  uint64_t entry_count;
  Spinlock spin;
} List;

List *List_Create(void) {
  List *t = p_malloc(sizeof(List));

  Spinlock spin = CreateSpinlock();
  if (LockSpinlock(spin) == NULL)
    return NULL;

  t->nodes = NULL;
  t->last = NULL;
  t->last_accessed_node = NULL;
  t->last_accessed_index = 0;
  t->entry_count = 0;
  t->spin = spin;
  UnlockSpinlock(spin);
  return t;
}

ListError List_AddEntry(List *a, void *value) {
  ListNode *l = p_malloc(sizeof(ListNode));
  if (l == NULL)
    return ListError_AllocationFailed;

  if (LockSpinlock(a->spin) == NULL)
    return ListError_Deleting;

  l->prev = a->last;
  l->value = value;
  l->next = NULL;
  if (a->last != NULL)
    a->last->next = l;
  a->last = l;
  if (a->nodes == NULL)
    a->nodes = a->last;
  if (a->last_accessed_node == NULL) {
    a->last_accessed_node = a->nodes;
    a->last_accessed_index = 0;
  }
  a->entry_count++;
  UnlockSpinlock(a->spin);
  return ListError_None;
}

uint64_t List_Length(List *a) {
  if (LockSpinlock(a->spin) == NULL)
    return 0;

  uint64_t val = a->entry_count;
  UnlockSpinlock(a->spin);
  return val;
}

void List_Remove(List *a, uint64_t index) {
  if (LockSpinlock(a->spin) == NULL)
    return;

  if (a->entry_count == 0 | index >= a->entry_count) {
    UnlockSpinlock(a->spin);
    return;
  }

  if (a->last_accessed_index >= a->entry_count) {
    a->last_accessed_index = 0;
    a->last_accessed_node = a->nodes;
  }

  while (a->last_accessed_index > index) {
    a->last_accessed_node = a->last_accessed_node->prev;
    a->last_accessed_index--;
  }

  while (a->last_accessed_index < index) {
    a->last_accessed_node = a->last_accessed_node->next;
    a->last_accessed_index++;
  }

  void *tmp = a->last_accessed_node;

  if (a->last_accessed_node->prev != NULL)
    a->last_accessed_node->prev->next = a->last_accessed_node->next;

  if (a->last_accessed_node->next != NULL)
    a->last_accessed_node->next->prev = a->last_accessed_node->prev;

  if (a->nodes == a->last_accessed_node)
    a->nodes = a->last_accessed_node->next;

  if (a->last == a->last_accessed_node)
    a->last = a->last_accessed_node->prev;

  a->last_accessed_node = a->nodes;
  a->last_accessed_index = 0;
  p_free(tmp);

  a->entry_count--;
  UnlockSpinlock(a->spin);
}

void List_Free(List *a) {
  FinalLockSpinlock(a->spin);
  while (List_Length(a) > 0) {
    List_Remove(a, 0);
  }
  Spinlock p = a->spin;
  p_free(a);

  UnlockSpinlock(p);
  FreeSpinlock(p);
}

void *List_EntryAt(List *a, uint64_t index) {

  if (LockSpinlock(a->spin) == NULL)
    return NULL;

  if (a->last_accessed_index >= a->entry_count) {
    a->last_accessed_index = 0;
    a->last_accessed_node = a->nodes;
  }

  if (index >= a->entry_count) {
    UnlockSpinlock(a->spin);
    return NULL;
  }

  // Accelarate first and last element accesses
  if (index == 0) {
    a->last_accessed_index = 0;
    a->last_accessed_node = a->nodes;
  }

  // Accelerate first and last element accesses
  if (index == a->entry_count - 1) {
    a->last_accessed_index = a->entry_count - 1;
    a->last_accessed_node = a->last;
  }

  while (a->last_accessed_index > index) {
    a->last_accessed_node = a->last_accessed_node->prev;
    a->last_accessed_index--;
  }

  while (a->last_accessed_index < index) {
    a->last_accessed_node = a->last_accessed_node->next;
    a->last_accessed_index++;
  }

  void *val = a->last_accessed_node->value;
  UnlockSpinlock(a->spin);
  return val;
}

void *List_RotNext(List *a) {
  if (LockSpinlock(a->spin) == NULL)
    return NULL;

  void *val = NULL;
  if (a->entry_count != 0)
    val = List_EntryAt(a, (a->last_accessed_index + 1) % a->entry_count);
  UnlockSpinlock(a->spin);
  return val;
}

void *List_RotPrev(List *a) {
  if (LockSpinlock(a->spin) == NULL)
    return NULL;

  void *val = NULL;
  if (a->entry_count != 0)
    val = List_EntryAt(a, (a->last_accessed_index - 1) % a->entry_count);
  UnlockSpinlock(a->spin);
  return val;
}

uint64_t List_GetLastIndex(List *a) {
  if (LockSpinlock(a->spin) == NULL)
    return -1;

  uint64_t i = a->last_accessed_index;
  UnlockSpinlock(a->spin);
  return i;
}

void List_Lock(List *a) {

}

void List_Unlock(List *a) {
  
}
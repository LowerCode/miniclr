#ifndef _CARDINAL_LIST_H_
#define _CARDINAL_LIST_H_

#include "types.h"

typedef struct ListNode ListNode;
typedef struct List List;

typedef enum ListError {
  ListError_None = 0,
  ListError_AllocationFailed = (1 << 0),
  ListError_EndOfList = (1 << 1),
  ListError_Deleting = (1 << 2),
} ListError;

List *List_Create(void);

ListError List_AddEntry(List *a, void *value);

uint64_t List_Length(List *a);

void List_Lock(List *a);

void List_Unlock(List *a);

void List_Remove(List *a, uint64_t index);

void List_Free(List *a);

void *List_EntryAt(List *a, uint64_t index);

void *List_RotNext(List *a);

void *List_RotPrev(List *a);

uint64_t List_GetLastIndex(List *a);

#endif
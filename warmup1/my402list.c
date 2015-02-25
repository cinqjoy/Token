#include "my402list.h"

#include <stdio.h>
#include <stdlib.h>

int My402ListLength(My402List *list){
    return list->num_members;
}

int My402ListEmpty(My402List *list){
    if(list->num_members == 0) return TRUE;
    else return FALSE;
}

int My402ListAppend(My402List *list, void *nobj){
    My402ListElem *new = (My402ListElem*)malloc(sizeof(My402ListElem));
    if(new == NULL) return FALSE;

    new->obj = nobj;
    new->next = &list->anchor;
    My402ListLast(list)->next = new;
    new->prev = My402ListLast(list);
    list->anchor.prev = new;
    list->num_members++;
    return TRUE;
}

int My402ListPrepend(My402List *list, void *nobj){
    My402ListElem *new = (My402ListElem*)malloc(sizeof(My402ListElem));
    if(new == NULL) return FALSE;

    new->obj = nobj;
    new->next = My402ListFirst(list);
    new->prev = &list->anchor;
    My402ListFirst(list)->prev = new;
    list->anchor.next = new;
    list->num_members++;
    return TRUE;
}

void My402ListUnlink(My402List *list, My402ListElem *elem){
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    free(elem);
    list->num_members--;
}

void My402ListUnlinkAll(My402List *list){
    My402ListElem *e, *f;
    for(e = My402ListFirst(list); e != NULL;){
        f = e;
        e = My402ListNext(list, e);
        My402ListUnlink(list, f);
        }
    list->num_members = 0;
}

int My402ListInsertBefore(My402List *list, void *nobj, My402ListElem *elem){
    if(!elem) return My402ListPrepend(list, nobj);

    My402ListElem *new = (My402ListElem*)malloc(sizeof(My402ListElem));   
    if(new == NULL) return FALSE;

    new->obj = nobj;
    new->prev = elem->prev;
    new->next = elem;
    elem->prev->next = new;
    elem->prev = new;
    list->num_members++;
    return TRUE;
}

int My402ListInsertAfter(My402List *list, void *nobj, My402ListElem *elem){
    if(!elem) return My402ListAppend(list, nobj);

    My402ListElem *new = (My402ListElem*)malloc(sizeof(My402ListElem));   
    if(new == NULL) return FALSE;

    new->obj = nobj;
    new->next = elem->next;   
    new->prev = elem;
    elem->next->prev = new;
    elem->next = new;
    list->num_members++;
    return TRUE;
}

My402ListElem* My402ListFirst(My402List *list){
    if(list->anchor.next) return list->anchor.next;
    else return NULL; 
}

My402ListElem* My402ListLast(My402List *list){
    if(list->anchor.prev) return list->anchor.prev;
    else return NULL; 
}

My402ListElem* My402ListNext(My402List *list, My402ListElem *elem){
    if(elem != My402ListLast(list)) return elem->next;
    else return NULL;
}

My402ListElem* My402ListPrev(My402List *list, My402ListElem *elem){
    if(elem != My402ListFirst(list)) return elem->prev;
    else return NULL;
}

My402ListElem *My402ListFind(My402List *list, void *nobj){
    My402ListElem *e;
    for(e = My402ListFirst(list); e != NULL; e = My402ListNext(list, e)){
        if(e->obj == nobj) return e;
        }
    return NULL;
}

int My402ListInit(My402List *list){
    if(list){
       list->anchor.next = &list->anchor;
       list->anchor.prev = &list->anchor;
       list->num_members = 0;
       return TRUE;
       }  
    else return FALSE; 
}

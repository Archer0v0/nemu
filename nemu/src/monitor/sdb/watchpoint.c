#include "sdb.h"

#define NR_WP 32



static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* wp_new(char *exp){
  if(free_ == NULL){
    Log("No free watchpoint available");
    return NULL;
  }

  bool success = true;
  word_t exprVal = expr(exp, &success);
  if (!success) return NULL;

  WP *tmpP = free_;
  free_ = free_->next;
  strcpy(tmpP->exp, exp);
  tmpP->value = exprVal;
  
  if(head == NULL){
    head = tmpP;
    tmpP->NO = 0;
    tmpP->next = NULL;
    return head;
  }
  tmpP->NO = head->NO+1;
  tmpP->next = head;
  head = tmpP;
  return head;
}

void wp_free(WP* wp){
  if(wp == NULL) Log("Invalid pointer wp");
  if(wp == head) 
    head = head->next;
  else{
    WP* tmp = head;
    while(tmp->next != wp)
      tmp = tmp->next;
    tmp->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

void wp_delete(int num){
  WP *tmp = head;
  while(tmp != NULL && tmp->NO != num) {
    tmp = tmp->next;
  }
  if( tmp == NULL){
    Log("Delete watch point %d filed: Invalid NO", num);
  }else{
    wp_free(tmp);
    Log("Delete watch point: %d", num); 
  }
}

void wp_display(){
  WP *tmp = head;
  while(tmp != NULL){
    printf("watch point %d: %s\n", tmp->NO,tmp->exp);
    printf("\tThe value now is: %d\n", tmp->value);
    tmp = tmp->next;
  }
  // tmp = free_;
  // int sum = 0;
  // while(tmp != NULL){
  //   sum+=1;
  //   tmp = tmp->next;
  // }
  // printf("free_ point number: %d\n", sum);
}

bool wp_check(){
  WP *tmp = head;
  while(tmp != NULL){
    bool success = true;
    word_t exprVal = expr(tmp->exp, &success);
    if(!success){Assert(0,"expr error");}
    if(exprVal != tmp->value) {
      printf("Get watch point %d: %s = %d\n", tmp->NO,tmp->exp, tmp->value);
      printf("The value now is: %d\n", exprVal);
      return false;
    }   
    tmp = tmp->next;
  }
  return true;
}

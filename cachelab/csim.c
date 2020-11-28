#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cachelab.h"
#include <stdbool.h>
#include <assert.h>
#define CHARNUMBER 20
typedef unsigned long Tag;

/*double linkedlist simulate LRU replacement policy*/
typedef struct line{
    struct line * next;
    struct line * prev;
    Tag key;
    bool valid;
} Line;

typedef struct queue{
    Line * head;
    Line * tail;
    int count;
    int capacity;
} Queue;

typedef struct cache{
    Queue * sets;
} CacheSimulator;

/*initialize cache simulator*/
void InitializeCache(CacheSimulator * cache, int numOfSets, int numOfLines, int numOfBlocks);

/*initialize queue for each set*/
void InitializeQueue(Queue * queue, int numOfLines);

/*flush cache return value 1:hit; 0:miss; -1:eviction*/
int FlushCache(unsigned int set_no, unsigned long tag, CacheSimulator * cache);

int FlushSet(unsigned long tag, Queue * set);

/*if the given tag is found in the set, remove the line to the tail*/
bool FindTagInSet(unsigned long tag, Queue * set);

/*LRU replacement strategy*/
void MoveToTail(Queue * set, Line * line);

/*capacity of LRU is full*/
void RemoveHead(Queue * set);

/*add to tail*/
void EnQueue(unsigned long tag, Queue * set);

/*remove line*/
void RemoveLine(Queue * set, Line * line);
/**/

int main(int argc, char * argv[]){
    FILE* fp;
    char buf[CHARNUMBER];
    char * ret_ptr;
    CacheSimulator cache;
    int i;
    int s, E, b, hits, misses, evictions;
    hits = misses = evictions = 0;

    if(argc != 9 || strcmp(argv[1],"-s") || strcmp(argv[3], "-E") || strcmp(argv[5], "-b") || strcmp(argv[7], "-t")){
        printf("invalid input of arguments.\n");
        exit(EXIT_FAILURE);
    }
    // read the arguments of s, E, b
    s = atoi(argv[2]);
    E = atoi(argv[4]);
    b = atoi(argv[6]);

    //initialize cache simulator -- allocate space 
    InitializeCache(&cache, (1 << s), E, b);

    fp = fopen(argv[8], "r");
    if(fp == NULL){
        printf("invalid file name %s \n", argv[8]);
        printf("system exit.\n");
        exit(EXIT_FAILURE);
    }
    
    /*parse input file, remove I command*/
    while(ret_ptr = fgets(buf, 20, fp)){
        i = 0;
        while(buf[i] != '\n')
            i++;
        buf[i] = '\0';
        if(buf[0] == 'I')
            continue;
        // /* a space char inserted before M L S*/
        // if(buf[1] == 'M' || buf[1] == 'L')
        //     continue;
        char mode = buf[1];
        /*string literal in C has static storage duration*/
        char addr[10] = "";
        int addr_len = strchr(buf, ',') - buf - 3;
        char * hex_addr = strncat(addr, buf + 3, addr_len);
        unsigned long address = strtoul(hex_addr, NULL, 16);
        unsigned int set_no = (unsigned int) address % (0x1 << (s + b));
        set_no = set_no / (0x1 << b);
        unsigned long tag = address / (0x1 << (s + b)); 
        // puts(&buf[1]);
        // printf("addres: %s | hex_adress: %lx | set number: %d | tag: %lx \n", hex_addr, address, set_no, tag);

        /* hit miss eviction*/
        int ret_flag = FlushCache(set_no, tag, &cache);
        if(mode == 'M'){
            /*1:hit; 0:miss; -1:eviction*/
            /*调用两次*/
            /*如果是1 hit *2 ｜ 如果是0 hit*1 miss * 1 ｜ 如果是-1 hit*1 eviction *1 hit * 1*/
            if(ret_flag == 1)
                hits += 2;
            else if(ret_flag == 0){
                misses++;
                hits++;
            }else{
                evictions++;
                misses++;
                hits++;
            }
        }else{
            if(ret_flag == 1)
                hits++;
            else if(ret_flag == 0)
                misses++;
            else{
                misses++;
                evictions++;
            }
        }
    }
    // printf("Hits: %d | Misses: %d | Evictions: %d \n", hits, misses, evictions);
    fclose(fp);
    printSummary(hits, misses, evictions);

    return 0;
}

void InitializeCache(CacheSimulator * cache, int numOfSets, int numOfLines, int numOfBlocks){
    cache->sets = (Queue *) malloc(numOfSets * sizeof(Queue));
    for(int i = 0; i < numOfSets; i++){
        InitializeQueue(cache->sets + i, numOfLines);
    }
}

void InitializeQueue(Queue * queue, int numOfLines){
    /*set two dummy nodes(head and tail) for convenience */
    queue->capacity = numOfLines;
    queue->count = 0;
    queue->head = (Line *) malloc(sizeof(Line));
    queue->tail = (Line *) malloc(sizeof(Line));
    queue->head->next = queue->tail;
    queue->tail->prev = queue->head; 
}

int FlushCache(unsigned int set_no, unsigned long tag, CacheSimulator * cache){
    Queue * queue = cache->sets + set_no;
    /*进入set，1.查找tag找到返回1 hit 2.如果set已经满了 eviction， 如果没满 miss*/
    return FlushSet(tag, queue);
}

int FlushSet(unsigned long tag, Queue * set){
    bool find_tag = FindTagInSet(tag, set);

    if(find_tag){
        /*hit*/
        return 1;
    }
    else{
        if(set->capacity == set->count){
            /*eviction*/
            RemoveLine(set, set->head->next);
            EnQueue(tag, set);
            return -1;
        }else{
            /*miss*/
            EnQueue(tag, set);
            return 0;
        }
    }

}

bool FindTagInSet(unsigned long tag, Queue * set){
    /* there are two dummy nodes in each queue for the convience of LRU*/
    Line * start = set->head->next;
    while(start->next){
        if(start->key == tag){
            /*LRU replacement strategy*/
            MoveToTail(set, start);
            return true;
        }
        start = start->next;
    }
    return false;
}

void MoveToTail(Queue * set, Line * line){
    unsigned long tag = line->key;
    RemoveLine(set, line);
    EnQueue(tag, set);
}

void RemoveHead(Queue * set){
    set->count--;
    Line * line = set->head->next;
    set->head->next = set->head->next->next;
    set->head->next->prev = set->head;
    free(line);
}

void EnQueue(unsigned long tag, Queue * set){
    assert(set->capacity != set->count);
    Line * line = (Line *) malloc(sizeof(Line));
    line->key = tag;
    Line * old_tail = set->tail->prev;
    set->tail->prev = line;
    line->prev = old_tail;
    line->next = set->tail;
    old_tail->next = line;

    set->count++;
}

void RemoveLine(Queue * set, Line * line){
    Line * prevLine = line->prev;
    Line * nextLine = line->next;
    prevLine->next = nextLine;
    nextLine->prev = prevLine;
    free(line);
    set->count--;
}
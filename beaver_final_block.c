#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <locale.h>
#define TURING_CARD_DATA_TYPE unsigned char
struct turing_card {
	TURING_CARD_DATA_TYPE state[2][3];
};

#define TURING_TAPE_TYPE  unsigned char
#define TURING_TAPE_INFO_TYPE int
struct turing_tape {
	TURING_TAPE_TYPE *array;//the array for the tape
	TURING_TAPE_TYPE *array_cent;//the center of the array for the tape
	unsigned TURING_TAPE_INFO_TYPE len;//the length of the array
	TURING_TAPE_INFO_TYPE min,max;//the minimum and maximum 
};
#define TURING_MACHINE_NUM_TYPE unsigned long int
TURING_MACHINE_NUM_TYPE int_pow(TURING_MACHINE_NUM_TYPE base, TURING_MACHINE_NUM_TYPE exp) {
	TURING_MACHINE_NUM_TYPE result=1;
	while(exp) {
		if(exp&1) {
			result*=base;
		}
		exp/=2;
		base*=base;
	}
	return result;
}
#define TURING_MACHINE_ACTION_TYPE int
#define TURING_MACHINE_ACTION_LEN_TYPE int
#define TURING_MACHINE_ACTION_VAR_TYPE int
int det_cycle_brent(TURING_MACHINE_ACTION_TYPE *actions,TURING_MACHINE_ACTION_LEN_TYPE len) {
	TURING_MACHINE_ACTION_VAR_TYPE power=1,mu=0,lam=1,t=1,h=2;
	if(len==0) {
		return 0;
	}
	while (actions[t]!=actions[h]) {
		if(power==lam) {
			t=h;
			power*=2;
			lam=0;
		}
		h++;
		lam++;
		if(h>=len) {
			return 0;
		}
	}
	t=0;
	h=lam;
	while(actions[t]!=actions[h]) {
		t++;
		h++;
		mu++;
	}
	//printf("mu=%d, lam=%d\n",mu,lam);
	return 1;
}
struct turing_machine {
	TURING_MACHINE_NUM_TYPE num;//the number corresponding to the configuration of cards
	TURING_MACHINE_ACTION_TYPE *actions;
	int step;//the current step of the turing machine
	int max_steps;//the maximum number of steps for the turing machine
	int ncards;//the number of cards
	struct turing_card *cur_card;//the current card
	struct turing_card *cards;//the cards for the turing machine
	TURING_TAPE_TYPE read_val;
	int cur_index;
	struct turing_tape *tape;//the tape the machine operates on
	int n1s;
};
struct turing_card *turing_n_to_cards(TURING_MACHINE_NUM_TYPE num, int ncards) {
	struct turing_card *cards;
	TURING_CARD_DATA_TYPE nums[2][3]={0};
	int i,j;
	cards=calloc(ncards,sizeof(struct turing_card));
	for(i=0;i<ncards;++i) {
		for(j=0;j<=1;++j) {
			nums[j][2]=num%(ncards+1);
			num/=ncards+1;
			nums[j][1]=num%2;
			num/=2;
			nums[j][0]=num%2;
			num/=2;
			memcpy(cards[i].state[j],nums[j],3*sizeof(TURING_CARD_DATA_TYPE));
		}
	}
	return cards;
}
struct turing_tape *turing_tape_init() {
	struct turing_tape *tape;
	tape=calloc(1,sizeof(struct turing_tape));
	tape->array=calloc(1,sizeof(TURING_TAPE_TYPE));
	tape->array_cent=&tape->array[0];
	tape->len=1;
	tape->min=0;
	tape->max=0;
	return tape;
}
void turing_tape_append_0(struct turing_tape *tape) {
	int i;
	tape->array=realloc(tape->array,(tape->len+1)*sizeof(TURING_TAPE_TYPE));//allocate more memory
	tape->array_cent=&tape->array[-tape->min];//relocate the center of the array
	tape->array[tape->len]=0;
	tape->len++;
	tape->max++;
}
void turing_tape_prepend_0(struct turing_tape *tape) {
	int i;
	tape->array=realloc(tape->array,(tape->len+1)*sizeof(TURING_TAPE_TYPE));//allocate more memory
	for(i=tape->len-1;i>=0;i--) {//shift everything forwards 1
		tape->array[i+1]=tape->array[i];
	}
	tape->array[0]=0;
	tape->array_cent=&tape->array[-tape->min+1];//relocate the center of the array
	tape->len++;
	tape->min--;
}
void turing_tape_check_bounds(int index, struct turing_tape *tape) {
	if(index>tape->max) {
		turing_tape_append_0(tape);
	}
	else if(index<tape->min){
		turing_tape_prepend_0(tape);
	}
}
void turing_tape_free(struct turing_tape *tape) {
	if(tape!=NULL) {
		free(tape->array);
	}
}
int turing_machine_check_halting(struct turing_card *cards, int ncards) {//checks to see if there is a change to halt command present
	int num,card,state,count;
	for(num=0;num<ncards+1;++num) {
		count=0;
		for(card=0;card<ncards;++card) {
			for(state=0;state<=1;++state) {
				if(cards[card].state[state][2]==num) {
					count++;
				}
			}
		}
		if(count==0) {
			return 0;
		}
	}
	return 1;
}
void turing_machine_free(struct turing_machine *machine) {
	free(machine->cards);
	turing_tape_free(machine->tape);
	free(machine->tape);
	free(machine);
}
struct turing_machine *turing_machine_init(int ncards, TURING_MACHINE_NUM_TYPE num, int max_steps) {
	struct turing_machine *machine;
	machine=calloc(1,sizeof(struct turing_machine));
	machine->cards=turing_n_to_cards(num,ncards);
	if(!turing_machine_check_halting(machine->cards,ncards)) {
		turing_machine_free(machine);
		return NULL;
	}
	machine->n1s=0;
	machine->num=num;
	machine->ncards=ncards;
	machine->max_steps=max_steps;
	machine->tape=turing_tape_init();
	machine->cur_card=&machine->cards[0];
	//read_val does not need to be initialized
	machine->cur_index=0;
	return machine;
}
int turing_run(struct turing_machine *machine) {
	//declare the variables
	TURING_TAPE_TYPE *read_val;
	int nactions=0;
	int *step;
	int *cur_index;
	TURING_TAPE_TYPE *array_cent;
	struct turing_card *cur_card;
	int i,n1s;
	int ccindex=0;
	//assign the variables
	read_val=&machine->read_val;
	step=&machine->step;
	cur_index=&machine->cur_index;
	//array_cent=machine->tape->array_cent;
	cur_card=machine->cur_card;
	//printf("%p\n",*array_cent);
	for(*step=0;*step<machine->max_steps;++*step) {
		array_cent=machine->tape->array_cent;
		//printf("%d %d %p %p %p\n",*read_val,*cur_index,machine->tape->array,array_cent,machine->tape->array_cent);
		*read_val=array_cent[*cur_index];
		array_cent[*cur_index]=cur_card->state[*read_val][0];
		*cur_index+=2*cur_card->state[*read_val][1]-1;
		turing_tape_check_bounds(*cur_index,machine->tape);
		if(det_cycle_brent(machine->actions,nactions)) {
			if(((*cur_index<=machine->tape->min)+(!cur_card->state[*read_val][1]))+((*cur_index>=machine->tape->max)+(cur_card->state[*read_val][1]))+(*read_val==cur_card->state[*read_val][0])) {
				machine->n1s=0;
				//printf("num=%'lu\n",machine->num);
				return -1;
			}
			nactions=0;
		}
		machine->actions[nactions]=*read_val;
		nactions++;
		machine->actions[nactions]=cur_card->state[*read_val][0]+2;
		nactions++;
		machine->actions[nactions]=cur_card->state[*read_val][1]+2+2;
		nactions++;
		machine->actions[nactions]=ccindex+2+2+2;
		nactions++;
		/*for(i=0;i<nactions;++i) {
			printf("%d ",actions[i]);
		}
		printf("\n");*/
		if(cur_card->state[*read_val][2]==0) {
			break;
		}
		ccindex=cur_card->state[*read_val][2]-1;
		cur_card=&machine->cards[cur_card->state[*read_val][2]-1];
	}
	if(machine->step!=machine->max_steps) {
		n1s=0;
		for(i=0;i<machine->tape->len;++i) {
			if(machine->tape->array[i]) {
				n1s++;
			}
		}
		machine->n1s=n1s;
		return 0;
	}
	else {
		//printf("num=%'lu\n",machine->num);
		machine->n1s=0;
		return -2;
	}
}
struct turing_thread_args {
	int blocksize;
	pthread_mutex_t *lock;
	TURING_MACHINE_NUM_TYPE *nextnum;
	int ncards;
	struct turing_machine *best;
	int max_steps;
	time_t start;
};
void *turing_thread(struct turing_thread_args *args) {
	static __thread TURING_MACHINE_NUM_TYPE cnum;
	static __thread TURING_MACHINE_NUM_TYPE num;
	static __thread struct turing_machine *machine;
	static __thread struct turing_machine *best;
	static __thread TURING_MACHINE_ACTION_TYPE *actions;
	actions=calloc(args->max_steps*4,sizeof(TURING_MACHINE_ACTION_TYPE));
	best=calloc(1,sizeof(struct turing_machine));
	best->n1s=1;
	pthread_mutex_lock(args->lock);
	num=*args->nextnum;
	cnum=num;
	*args->nextnum=*args->nextnum+args->blocksize;
	pthread_mutex_unlock(args->lock);
	while(num<int_pow(4*args->ncards+4,args->ncards*2)) {
		pthread_mutex_lock(args->lock);
		num=*args->nextnum;
		cnum=num;
		*args->nextnum=*args->nextnum+args->blocksize;
		pthread_mutex_unlock(args->lock);
		
		for(;num<cnum+args->blocksize;num++) {
			//mutex_lock causes a huge slowdown in windows
			machine=turing_machine_init(args->ncards,num,args->max_steps);
			if(machine!=NULL) {
				machine->actions=actions;
				if(turing_run(machine)==-2) {
					//printf("%lu\n",num);
				}
				
				if(machine->n1s>=best->n1s) {
					//turing_tape_free(args->best->tape);
					//turing_machine_free(args->best);
					best=machine;
					printf("num=%'lu, n1s=%d, step=%d, machines/s=%'f\n",machine->num,machine->n1s,machine->step,(float)machine->num/(float)(time(NULL)-args->start));
				}
				else {
					//turing_tape_free(machine->tape);
					turing_machine_free(machine);
				}
				
			}
		}
	}
	args->best=best;
	return NULL;
}
void turing_run_all(int ncards, int max_steps, int nthreads, int blocksize) {
	printf("initializing...\n");
	time_t start=time(NULL);
	struct turing_machine *best;
	best=calloc(1,sizeof(struct turing_machine));
	best->n1s=0;
	pthread_mutex_t lock;
	pthread_mutex_init(&lock,NULL);
	pthread_t *tthreads=calloc(nthreads,sizeof(pthread_t));
	struct turing_thread_args *args=calloc(nthreads,sizeof(struct turing_thread_args));
	TURING_MACHINE_NUM_TYPE nextnum=0;
	//printf("%p\n",&nextnum);
	printf("running...\n");
	int i;
	for(i=0;i<nthreads;++i) {
		args[i].blocksize=blocksize;
		args[i].lock=&lock;
		args[i].ncards=ncards;
		args[i].max_steps=max_steps;
		args[i].nextnum=&nextnum;
		args[i].start=start;
		pthread_create(&tthreads[i],NULL,(void*)&turing_thread,&args[i]);
	}
	//printf("%d\n",nextnum);
	for(i=0;i<nthreads;++i) {
		pthread_join(tthreads[i],NULL);
	}
	printf("finding best...\n");
	for(i=0;i<nthreads;++i) {
		if(args[i].best->n1s>=best->n1s) {
			best=args[i].best;
		}
	}
	printf("winner: num=%lu steps=%d, n1s=%d\n",best->num,best->step, best->n1s);
	for(i=0;i<best->tape->len;++i) {
		printf("%d ",best->tape->array[i]);
	}
	printf("\n");
	//printf("%d\n",nextnum);
}
int main(int argc, char *argv[]) {
	//TURING_MACHINE_ACTION_VAR_TYPE actions[6]={0,1,2,0,1,2};
	setlocale(LC_NUMERIC,"");
	time_t then=time(NULL);
	turing_run_all(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[4]));
	time_t now=time(NULL);
	//printf("%d\n",det_cycle_brent((int *)actions,6));
	printf("took %d seconds\n",now-then);
	return 0;
}

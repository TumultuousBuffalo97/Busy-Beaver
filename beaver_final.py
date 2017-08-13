import time
start=time.time()
class turing_card:
	def __init__(self,state):
		self.state=state
def det_cycle_brent(array):#this appears to work
	power=1
	mu=0
	lam=1
	t=1
	h=2
	if len(array)==0:
		return 0
	while array[t]!=array[h]:
		if power==lam:
			t=h
			power*=2
			lam=0
		h+=1
		lam+=1
		#print(t,h,len(array))
		if h>=len(array):
			return 0;
	t=0
	h=lam
	while array[t]!=array[h]:
		t+=1
		h+=1
		mu+=1
	#print(mu,lam)
	return 1
def turing_n_to_cards(num,ncards):
	cards=[]
	nums=[[0,0,0],[0,0,0]]
	for i in range(0,ncards):
		cards.append(turing_card([[0,0,0],[0,0,0]]))
		nums=[[0,0,0],[0,0,0]]
		for j in range(0,2):
			nums[j][2]=num%(ncards+1)
			num//=ncards+1
			nums[j][1]=num%2;
			num//=2;
			nums[j][0]=num%2;
			num//=2;
		cards[i].state=nums
	return cards;
class turing_machine:
	def check_halting(self):
		for i in range(0,self.ncards+1):
			num=0
			for j in range(0,self.ncards):
				for k in range(0,2):
					if self.cards[j].state[k][2]==i:
						num+=1
			if num==0:
				return 0
		return 1
	def __init__(self,num,ncards,max_steps):
		self.n1s=0
		self.num=num
		self.step=0
		self.max_steps=max_steps
		self.ncards=ncards
		self.cur_card=0
		self.cards=turing_n_to_cards(num,ncards)
		self.tape=[0]
		self.tape_min=0
		self.tape_max=0
		self.tape_len=1
		self.cur_index=0
		if self.check_halting()!=1:
			self.cards=[]
	def run(self):
		if len(self.cards)==0:
			return -1
		actions=[]
		read_val=0;
		write=0
		move=0
		ccard=0
		cindex=0
		for self.step in range(0,self.max_steps):
			read_val=self.tape[self.cur_index-self.tape_min]#read from the tape into read_val
			write=self.cards[self.cur_card].state[read_val][0]
			move=self.cards[self.cur_card].state[read_val][1]
			ccard=self.cur_card
			cindex=self.cur_index
			self.tape[self.cur_index-self.tape_min]=write#write to the tape
			'''print(self.tape)
			for i in range(0,-3*self.tape_min+1):
				print(" ",end="")
			print("^")
			'''
			
			self.cur_index+=2*move-1#move'
			
			if self.cur_index>self.tape_max:#fix the tape if the machine moved out of bounds
				self.tape.append(0)
				self.tape_max+=1
				self.tape_len+=1
			if self.cur_index<self.tape_min:
				self.tape=[0]+self.tape
				self.tape_min-=1
				self.tape_len+=1
			if det_cycle_brent(actions):#if the same actions are taken
				#print(actions)
				#print(read_val,move)
				#if the 
				if ((((self.cur_index<=self.tape_min)+(move==0))+((self.cur_index>=self.tape_max)+(move==1)))+(read_val==write)):
					return -2
				actions=[]
			actions.append(read_val)
			actions.append(write+2)
			actions.append(move+2+2)
			actions.append(ccard+2+2+2)
			if self.cards[self.cur_card].state[read_val][2]==0:
				break
			self.cur_card=self.cards[self.cur_card].state[read_val][2]-1#change state
		
		if self.step==self.max_steps-1:
			self.n1s=0
			return -3
		for i in range(0,self.tape_len):
			if self.tape[i]==1:
				self.n1s+=1
	def print(self):
		print("num="+str(self.num)+" n1s="+str(self.n1s)+" steps="+str(self.step)+" cur_index="+str(self.cur_index)+" tape_min="+str(self.tape_min)+" tape_max="+str(self.tape_max))
		for i in self.cards:
			print(i.state)
		print("tape_cent=",str(-self.tape_min))
		print(self.tape)
def turing_machine_run_all(ncards,max_steps):
	best=turing_machine(0,ncards,max_steps)
	best.n1s=1
	for i in range(0,(4*(ncards+1))**(2*ncards)):
		current=turing_machine(i,ncards,max_steps)
		if current.run()==-3:
			print(str(current.num)+" reached max steps ("+str(current.max_steps)+")")
			#current.print()
			#pass
		if current.n1s>=best.n1s:
			best=current
			print("num="+str(best.num)+" n1s="+str(best.n1s)+" steps="+str(best.step),end=" ")
			if time.time()-start>0:
				print("machines/s="+str(current.num/(time.time()-start)))
			else:
				print("")

turing_machine_run_all(3,400000)
print("took "+str(time.time()-start)+" seconds")
'''
a=turing_machine(70383,3,40)
print(a.run())
a.print()
'''

import string
import random
import sys

def getCode(length = 10, char = string.ascii_uppercase + string.ascii_lowercase + string.digits):
	return ''.join(random.choice( char) for x in range(length))


print("Enter the number of requests to generate: ", file=sys.stderr)
n = int(input())

req_read = list() # 1
req_put = list() # 2
req_del = list() # 3
req = list()

for i in range(int(n*0.2)):
	k,v = getCode(10), getCode(10)
	req_put.append(("2",k,v))
	req.append(req_put[-1])

for i in range(int(n*0.2), n):
	ch = random.choice([1,2,3])
	if ch == 1:
		# READ: only key
		rchoice = ("1", random.choice(req_put)[1])
		req_read.append(rchoice)
	elif ch == 2:
		# PUT: key and value
		if random.choice([0,1]) == 1:  # add new value
			k,v = getCode(10), getCode(10)
			rchoice = ("2",k,v)
		else:
			rchoice = random.choice(req_put)
		req_put.append(rchoice)
	else:
		# DEL: only key
		rchoice = ("3", random.choice(req_put)[1])
		req_del.append(rchoice)
	req.append(rchoice)

print(n)
for i in req:
	print(" ".join(i))

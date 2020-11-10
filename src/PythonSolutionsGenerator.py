n = int(input())
kv = dict()
print(n)
for i in range(n):
	vals = input().split()
	# print(vals)
	if vals[0] == "1":  # read/GET
		if vals[1] in kv:
			print(kv[vals[1]])
		else:
			print("-ERROR-")
	elif vals[0] == "2":  # write/PUT
		kv[vals[1]] = vals[2]
		print("-SUCCESS-")
	elif vals[0] == "3":  # DELETE
		if vals[1] in kv:
			kv.pop(vals[1])
			print("-SUCCESS-")
		else:
			print("-ERROR-")

'''

print(kv, file=open("temp.dictionary", "w+"))

import random
kv = eval(open('temp.dictionary', 'r').read())
fff = open('client_request_009.txt', "w+")
fffsol = open('client_request_009_sol.txt', "w+")
kvkeylist = list(kv.keys()); random.shuffle(kvkeylist)
print(len(kv),file=fff)
print(len(kv),file=fffsol)
for i in kvkeylist:
    key=i#random.choice(kvkeylist)
    val1 = kv[key]
    print(f"1 {key}", file=fff,flush=True)
    print(f"{val1}", file=fffsol,flush=True)

'''
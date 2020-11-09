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

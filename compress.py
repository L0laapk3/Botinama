# unrelated experiment


import operator as op
from functools import reduce

def ncr(n, r):
    r = min(r, n-r)
    numer = reduce(op.mul, range(n, n-r, -1), 1)
    denom = reduce(op.mul, range(1, r+1), 1)
    return numer // denom



def boardToBits(s):
	# converts a board string to a 41 bit number
	res = s.index('2')
	s = s.replace('2', '')

	res = 24 * res + s.index('4')
	s = s.replace('4', '')
	
	res *= ncr(23, 4) + ncr(23, 3) + ncr(23, 2) + ncr(23, 1) + ncr(23, 0)
	ix = [i for i, x in enumerate(s) if x == '1']
	for i in range(4, 0, -1):
		if (len(ix) < i):
			res += ncr(23, i)
		else:
			pass
			
	


boardToBits("1121100000000000000033433")
boardToBits("0000000000000001111333324")





from numpy import random

X0 = 35000
N0 = 100
X1 = 18000
N1 = 200

n0 = random.poisson(lam = X0, size = N0)
n1 = random.poisson(lam = X1, size = N1)

for i in range(1, len(n0)):
    n0[i] += n0[i-1]

for i in range(1, len(n1)):
    n1[i] += n1[i-1]

n = list()

for i in n0:
    n.append((i, 0))

for i in n1:
    n.append((i, 1))

n.sort()

for i in n:
    print(i[0], i[1])


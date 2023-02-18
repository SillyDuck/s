import random
f = open('data1', 'w')
for i in range(10000000):
    f.write(str(random.randint(0,1001))+'\n')
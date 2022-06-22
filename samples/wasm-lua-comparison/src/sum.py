def sum (start, length):
    for x in range(0,10000000):
        sum(x for x in range (start,length))
sum(2,3)
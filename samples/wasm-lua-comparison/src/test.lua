-- function fib(n)
--     if n == 1 or n == 2 then
--        return 1,1
--     end
--     prev, prevPrev = fib(n-1)
--     return prev+prevPrev, prev
--  end

--  print(fib(5))
--  print(fib(10))

 function sum(start, length)
    local sum =0
    print(start)
for x=0,10000000 do 
    for x=start,(start+length) do
        sum = sum + x;
    end
    
end
return sum
end
--  print(sum(1))
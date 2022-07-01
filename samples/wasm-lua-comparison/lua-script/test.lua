function power(n)
    local start=7
    for x=0,(n-1) do
        start=start*(start+1)
    end
    return start
end

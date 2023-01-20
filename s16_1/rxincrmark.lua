
--local posix = require'posix'




test  = function(endpoint)
	i = 0
	while(i ~= endpoint)
	do
		i = i + 1
	end
end

ti = os.time()

test(0xffFFffFF)

ti = os.time() - ti

print(ti)

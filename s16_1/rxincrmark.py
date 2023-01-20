from time import time

def get_ms():
    return time()*1000

def busy(iterations):
    it = 0
    while it < iterations:
        it = it + 1
    return

ti = get_ms()
busy(0xffFFffFF)
print( (get_ms() - ti)  )

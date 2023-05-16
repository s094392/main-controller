first = -1
last = -1
comp_time0 = 0
comp_time1 = 0
resp_time0 = 0
resp_time1 = 0
model0 = 0
model1 = 0

while True:
    try:
        arrival, start, end, comp_time, model_id = tuple(map(int, input().split()))

        if first == -1:
            first = arrival
        else:
            first = min((first, arrival))

        if last == -1:
            last = end
        else:
            last = max((last, end))

        if model_id == 0:
            model0 += 1
            comp_time0 += end - start
            resp_time0 += end - arrival
        else:
            model1 += 1
            comp_time1 += end - start
            resp_time1 += end - arrival

    except EOFError:
        break;

print("Tail Latency: ",last - first)
if model0:
    print("Model 0: ")
    print("Comp Time: ", comp_time0 / model0)
    print("Resp Time: ", resp_time0 / model0)
    print("Num: ", model0)

if model1:
    print("Model 1: ")
    print("Comp Time: ", comp_time1 / model1)
    print("Resp Time: ", resp_time1 / model1)
    print("Num: ", model1)

print(comp_time0 / model0, comp_time1 / model1, resp_time0 / model0, resp_time1 / model1, last - first, model0, model1)

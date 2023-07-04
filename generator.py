import sys
from numpy import random


def main():
    N = 1024

    N0 = 10000
    N1 = 10000

    original = {
        "vgg16": 44943,
        "tf": 24488,
        "yolo": 17621,
        "alexnet": 6972,
        "lstm": 745
    }

    latencies = {
        "vgg16": [49622, 53909, 59382, 67626, 79759, 106498, 150111],
        "tf": [26339, 30145, 33294, 39003, 44858, 57650, 81442],
        "yolo": [18310, 19474, 20506, 22739, 26154, 31986, 44006],
        "alexnet": [7595, 7736, 8011, 8544, 9375, 10834, 14496],
        "lstm": [798, 845, 852, 857, 968, 1285, 3202]
    }

    latencies = {
        "vgg16": [89937, 98626, 59382, 67626, 79759, 106498, 150111],
        "tf": [26339, 30145, 33294, 39003, 44858, 57650, 81442],
        "yolo": [18310, 19474, 20506, 22739, 26154, 31986, 44006],
        "alexnet": [7595, 7736, 8011, 8544, 9375, 10834, 14496],
        "lstm": [798, 845, 852, 857, 968, 1285, 3202]
    }

    model0, model1 = sys.argv[1], sys.argv[2]
    part = int(sys.argv[3])

    # X0 = int(original[model0] * 1.1)
    # X1 = int(original[model1] * 7)

    X0 = int(latencies[model0][part] * 1.02)
    X1 = int(latencies[model1][-1 - part] * 1.5)

    # X0 = 1

    print(X0, X1)

    n0 = random.poisson(lam=X0, size=N0)
    n1 = random.poisson(lam=X1, size=N1)

    for i in range(1, len(n0)):
        n0[i] += n0[i - 1]

    for i in range(1, len(n1)):
        n1[i] += n1[i - 1]

    n = list()

    for i in n0:
        n.append((i, 0))

    for i in n1:
        n.append((i, 1))

    n.sort()

    for i in n[:N]:
        print(i[0], i[1])


if __name__ == "__main__":
    main()

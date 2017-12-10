#!/usr/bin/env python3

import grpc
import simple_pb2_grpc
import simple_pb2
from concurrent.futures import ThreadPoolExecutor


def gen_file():
    with open('C:/home/data/input.txt', 'w') as tmp_file:
        for i in range(2, 5000):
            tmp_file.write(str(i) + '\n')
    quit()


def read_file():
    with open('C:/home/data/input.txt', 'r') as tmp_file:
        return tmp_file.readlines()


def thread_func(in_number, in_file, in_stub):
    resp = in_stub.Factorize(simple_pb2.FactorRequest(number=in_number))
    factors = list(resp.factors)
    str_to_write = str(in_number) + '\t'
    for num in factors:
        str_to_write += str(num) + ','
    if str_to_write.endswith(','):
        str_to_write = str_to_write[:-1]
    str_to_write += '\n'
    in_file.write(str_to_write)


if __name__ == '__main__':
    # gen_file()
    lines = read_file()
    numbers = [int(x.split()[0]) for x in lines]

    channel = grpc.insecure_channel('localhost:5757')
    stub = simple_pb2_grpc.SampleServiceStub(channel)

    hFile = open('C:/home/data/output.txt', 'w')

    with ThreadPoolExecutor(max_workers=5) as executor:
        for number in numbers:
            executor.submit(thread_func, number, hFile, stub)
    hFile.close()
    print('ok')
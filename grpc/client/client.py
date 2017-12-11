#!/usr/bin/env python3

import grpc
import simple_pb2_grpc
import simple_pb2
from concurrent.futures import ThreadPoolExecutor, as_completed
import threading
from multiprocessing import Queue

locker = threading.Lock()


def gen_file():
    with open('C:/home/data/input.txt', 'w') as tmp_file:
        for i in range(2, 5000):
            tmp_file.write(str(i) + '\n')
    quit()


def get_string_to_write(in_number, in_stub):
    resp = in_stub.Factorize(simple_pb2.FactorRequest(number=in_number))
    factors = [str(factor) for factor in resp.factors]
    str_to_write = str(in_number) + '\t' + (','.join(factors)) + '\n'
    return str_to_write


def thread_func_queue(in_number, in_queue, in_stub):
    in_queue.put(get_string_to_write(in_number, in_stub))
    return 4


def thread_func_lock(in_number, in_file, in_stub):
    locker.acquire()
    in_file.write(get_string_to_write(in_number, in_stub))
    locker.release()


def thread_func_future(in_number, in_stub):
    return get_string_to_write(in_number, in_stub)


def lock_version(numbers, stub):
    with open('C:/home/data/output.txt', 'w') as file:
        with ThreadPoolExecutor(max_workers=5) as executor:
            for number in numbers:
                executor.submit(thread_func_lock, number, file, stub)


def queue_version(numbers, stub):
    with open('C:/home/data/output.txt', 'w') as file:
        queue = Queue()
        with ThreadPoolExecutor(max_workers=5) as executor:
            for number in numbers:
                executor.submit(thread_func_queue, number, queue, stub)
        file.write(queue.get())


def future_version(numbers, stub):
    with open('C:/home/data/output.txt', 'w') as file:
        with ThreadPoolExecutor(max_workers=5) as executor:
            wait_for = [executor.submit(thread_func_future, number, stub) for number in numbers]
            for fut in as_completed(wait_for):
                file.write(fut.result())


if __name__ == '__main__':
    # gen_file()

    lines = []
    with open('C:/home/data/input.txt', 'r') as tmp_file:
        lines = tmp_file.readlines()

    numbers = [int(x.split()[0]) for x in lines]

    channel = grpc.insecure_channel('localhost:5757')
    stub = simple_pb2_grpc.SampleServiceStub(channel)

    # lock_version(numbers, stub)
    # queue_version(numbers, stub)
    future_version(numbers, stub)

    print('ok')

#!/usr/bin/env python3

import os
import grpc_tools.protoc as pb

if __name__ == '__main__':
    pb.main(['-I=.',
            '--python_out=.',  
            '--grpc_python_out=.', 
            'simple.proto'])
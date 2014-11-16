#!/bin/env python # -*- mode: python -*- #
###########################################################################
# You set all command line args for "scons" in this file, eg:             #
#                                                                         #
#  CXX = "distcc g++" #set c++ compiler to "distcc g++"                   #
#                                                                         #
# invoke "scons -h" to see all available args                             #
###########################################################################
def getUpperPath():
    import os
    path = os.getcwd()
    ls = path.split("/")
    n = len(ls)
    if n < 1:
        return None
    n = n -1
    path = '/'
    dirCount = n
    n = 1
    while n < dirCount :
        path = path + ls[n] + '/'
        n = n+1
    return path
root = getUpperPath()

engine_commonheaderdir = '/home/w/include/'
engine_commonlibdir='/home/w/lib64/'

librarytype = 'shared'
heapchecktype = 'none'
mode = 'debug'


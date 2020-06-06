gcc -pg decoder.c demod/cicFilterCplxStep.c demod/pttA2Demod.c demod/sampler.c detect/detect_loop.c main.c service.c -lm -fopenmp -o main

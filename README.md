# SpongeBob

## How to use

```
mkdir build && cd build
cmake ..
make -j
```

## How to test

1. Change `conf/conf.xml`
2. Run `metaserver`
3. Run `storageserver` at all storage servers

### FUSE mount

```
./spfs -o direct_io -f spfs/
```

This will mount SpongeBob to `spfs/``

### MPI Test

```
mpirun -np 4 mpibw 10240 1
```

### Simple Test

Library-based:

```
./rw_direct
```

FUSE-based(chdir to mount directory):

```
../rw_bw
```

### FIO

```
fio --name=sequential_write --rw=write --bs=128k --numjobs=1 --size=128m --time_based --runtime=60s --filename=./spfs/testfile1
fio --name=sequential_read --rw=read --bs=128k --numjobs=1 --size=128m --time_based --runtime=60s --filename=./spfs/testfile1
```

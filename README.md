# Out-component measurement

## Compiling
A recent (C++17 compatible) version of GCC is required. To compile the
executables just use `make`.

A general purpose executable with double-precision floating-point time type and
32-bit unsigned integer vertex names:
```
make largest_out_component
make network_stats
```

Executables specifically tuned to mobile data-set with 32-bit unsigned integer
times and vertex names:
```
make largest_out_component_mobile
make network_stats_mobile
```

Executables specifically tuned to transport data-set with 32-bit unsigned
integer times, vertex names and delayes:
```
make largest_out_component_transport
make network_stats_transport
```


A random temporal network generator for use with the first executable:
```
make random_network
```

## Using the implementation

You can give the executable an event file (`--network path/to/file`) with each
line containing one event formatted as `v1 v2 time` or generate a random network
with given parameters for you. You can check the available options and output
options using:


```
./largest_out_component{,_mobile,_transport} -h
./network_stats{,_mobile,_transport} -h
./random_network{,_mobile,_transport} -h
```

### Input format
Event list files (via `--network` argument) should be a list of events
formatted as `v1 v2 time` or `v1 v2 time delay` separated by newlines and with
no header row. Here's the first few lines of an example random network:

```
73 0 0.143728
114 0 0.143728
311 0 0.143728
648 0 0.143728
784 0 0.143728
958 0 0.143728
9 1 0.143728
105 1 0.143728
553 1 0.143728
48 2 0.143728
```

You can also generate random networks using the provided `random_network` tool:

```
./random_network --seed 1 --average-degree 9 \
    --node 10 --max-t 128 --bursty --barabasi > random.events
```

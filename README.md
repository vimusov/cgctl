# What?

A bunch of utilities which makes daemons management better.
They made to be used with SysV init scripts or with upstart configs.

# How?

The utilities are built over cgroup v1 and allows to control system resources consumption.
Also they watch over child processes and help to kill all of them when a daemon is going to stop.

When a daemon is going to run, a new cgroup is created. All child processes will be put into it
and limited as needed. Limits can be set on CPU usage and RAM+SWAP.

**WARNING!** Limits are calculating as a percent of total system capabilities!

# Build

- CentOS >= 6.6;
- gcc & glibc-devel required;

`/cgroup` must be the mountpoint of cgroupfs. You can change it in `CGROUP_ROOT_DIR`:`conf.h`.

# Usage

See the [manual](manual.md) for the details. Also look at the `examples` subdirectory.

# Status

Production ready.

# License
GPL.

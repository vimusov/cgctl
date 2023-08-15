# cgctl

Intended to be used in SysV init scripts as a shebang.

```
#!/usr/bin/cgctl --options=cpu_usage=50,mem_usage=25
# Set CPU limit to 50% and RAM+SWAP limit to 25%.

... # the content of the script
```

# cgctl-start

Intended to be used with upstart as a start action.

```
description "Some program"
expect stop
# Create a cgroup named 'some_program', limit CPU to 10% and RAM+SWAP to 5%.
exec cgctl-start --cpu-usage=10 --mem-usage=5 -- some_program --option=value
```

# cgctl-stop

Intended to be used with upstart as a stop action. Will also kill all the children processes if any.

```
post-stop exec cgctl-stop some_program
```

# cgctl-append

Intended to be used with upstart. Runs a process and adds it into an existing cgroup instead of creating a new one.

```
description "Some prog which runs under some backend"
exec cgctl-append some_backend -- some_prog
```

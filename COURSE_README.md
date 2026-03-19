# Managing build configurations

Passing extra configuration files

```
west build -b native_sim  -- -DCONF_FILE=debug.conf
```

Running target

```
west build -b native_sim  -- -DCONF_FILE=debug.conf -t run
```

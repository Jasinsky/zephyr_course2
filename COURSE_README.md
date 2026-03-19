# Managing build configurations

Passing extra configuration files

```
west build -b native_sim  -- -DCONF_FILE=debug.conf
```

Running target

```
west build -b native_sim  -- -DCONF_FILE=debug.conf -t run
```

This will add debug.conf and keep prj.conf:

```
west build -b native_sim/native/64 -p -t run -- -DEXTRA_CONF_FILE=debug.conf
```
EXT4/JBD2 Buffer Debug
===========================
Buffer-layer tracing support for ext4/jbd2(linux kernel 2.6.32.67).

>It records, within the bh, the inputs from the last 32 `BUFFER_TRACE()`s which were run against this bh. If a `J_ASSERT` fails then you get a nice trace of the last 32 "things" which happened to this bh, including the bh's state at that transition. It basically tells you everything you need to know to find the bug.


CONFIGURATION the kernel:

`BUFFER_DEBUG` depends on JBD2. Here is an example:
```
CONFIG_EXT4_FS=m
CONFIG_EXT4_DEBUG=y
CONFIG_JBD2=m
CONFIG_JBD2_DEBUG=y
CONFIG_BUFFER_DEBUG=y
```

You can also find ext3 buffer debug patch from [HERE](http://lkml.iu.edu/hypermail/linux/kernel/0609.0/1543.html).

Related patch for jbd and jbd2: https://gist.github.com/huwan/b03a16c49e61b69cfa67.

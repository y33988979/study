cmd_/root/wangfuqiang/kernel_test/kprobe/kprobe_test.ko := ld -r  -EL -maarch64linux  -T ./scripts/module-common.lds -T ./arch/arm64/kernel/module.lds  --build-id  -o /root/wangfuqiang/kernel_test/kprobe/kprobe_test.ko /root/wangfuqiang/kernel_test/kprobe/kprobe_test.o /root/wangfuqiang/kernel_test/kprobe/kprobe_test.mod.o ;  true

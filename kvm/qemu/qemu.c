
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/kvm.h>
#include <sys/ioctl.h>

#define KVM_API_VERSION 12

struct kvm {
	int kvmfd;
	int vmfd;
	int vcpu_fd;
};


struct kvm* kvm_init(void)
{
	struct kvm *kvm;

	kvm = (struct kvm *)malloc(sizeof(struct kvm));
	if (!kvm) {
		perror("malloc");
		return NULL;
	}

	kvm->kvmfd = open("/dev/kvm", O_RDWR);
	if (kvm->kvmfd < 0) {
		free(kvm);
		perror("open");
		return NULL;
	}

	return kvm;
}

void kvm_deinit(struct kvm *kvm)
{
	if (kvm) {
		close(kvm->kvmfd);
		free(kvm);
	}
}

int get_kvm_api_version(struct kvm *kvm)
{
	int ret;

	ret = ioctl(kvm->kvmfd, KVM_GET_API_VERSION, 0);
	if (ret != KVM_API_VERSION)
		perror("KVM_API_VERSION ioctl");

	printf("KVM_API_VERSION: %d\n", ret);
	return ret;
}

int vm_create(struct kvm *kvm)
{
	int ret;

	ret = ioctl(kvm->kvmfd, KVM_CREATE_VM, 0);
	if (ret < 0)
		perror("KVM_CREATE_VM ioctl");
	
	/*
	 * call kvm_dev_ioctl_create_vm(arg) 
	 * 
	 * kvm_vm_ioctl:
	 * #define KVM_CREATE_VCPU           _IO(KVMIO,   0x41)
	   #define KVM_GET_DIRTY_LOG         _IOW(KVMIO,  0x42, struct kvm_dirty_log)
	   #define KVM_SET_MEMORY_ALIAS      _IOW(KVMIO,  0x43, struct kvm_memory_alias)
	   #define KVM_SET_NR_MMU_PAGES      _IO(KVMIO,   0x44)
	   #define KVM_GET_NR_MMU_PAGES      _IO(KVMIO,   0x45)
	   #define KVM_SET_USER_MEMORY_REGION _IOW(KVMIO, 0x46, \
	                                        struct kvm_userspace_memory_region)
	   #define KVM_SET_TSS_ADDR          _IO(KVMIO,   0x47)
	   #define KVM_SET_IDENTITY_MAP_ADDR _IOW(KVMIO,  0x48, __u64)
	 */
	kvm->vmfd = ret;
	printf("vm_create: ret=%d\n", ret);
	return ret;
}

int vm_vcpu_create(struct kvm *kvm, int cpuid)
{
	int ret;

	ret = ioctl(kvm->vmfd, KVM_CREATE_VCPU, cpuid);
	if (ret < 0)
		perror("KVM_CREATE_VCPU ioctl");
	
	/*
	 * kvm_vm_ioctl_create_vcpu(kvm, arg);
	 * kvm_vcpu_ioctl:
	 * #define KVM_RUN                   _IO(KVMIO,   0x80)
	   #define KVM_GET_REGS              _IOR(KVMIO,  0x81, struct kvm_regs)
	   #define KVM_SET_REGS              _IOW(KVMIO,  0x82, struct kvm_regs)
	   #define KVM_GET_SREGS             _IOR(KVMIO,  0x83, struct kvm_sregs)
	   #define KVM_SET_SREGS             _IOW(KVMIO,  0x84, struct kvm_sregs)
	   #define KVM_TRANSLATE             _IOWR(KVMIO, 0x85, struct kvm_translation)
	   #define KVM_INTERRUPT             _IOW(KVMIO,  0x86, struct kvm_interrupt)
	 */
	
	kvm->vcpu_fd = ret;
	printf("vm_vcpu_create: ret=%d\n", ret);
	return ret;
}

int vm_vcpu_run(struct kvm *kvm)
{
	int ret;

	ret = ioctl(kvm->vcpu_fd, KVM_RUN, 0);
	if (ret < 0)
		perror("KVM_RUN ioctl");
	
	printf("vm_vcpu_run: ret=%d\n", ret);
	return ret;
}

int main(int argc, char **argv)
{
	struct kvm *kvm;

	kvm = kvm_init();

	get_kvm_api_version(kvm);
	vm_create(kvm);

	vm_vcpu_create(kvm, 0);
	vm_vcpu_run(kvm);

	kvm_deinit(kvm);
	
	return 0;
}

